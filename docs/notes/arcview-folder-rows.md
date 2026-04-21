# Archive Viewer: Folder Rows in the File List

## Goal

The archive viewer dialog (`CArcViewDlg`) previously showed only the files
directly in the folder selected in the tree. This work adds subfolder rows to
the list so that the user can:

1. See both files and immediate child folders of the selected tree node.
2. Double-click a folder row (or press **View** with a folder selected) to
   navigate into that folder — the tree selects the corresponding node and the
   list repopulates via the existing `TVN_SELCHANGED` → `FilterListByFolder`
   flow.
3. Have folder selection be treated as recursive for Extract Each, drag &
   drop, and the right-click **SendTo** menu — i.e. selecting a folder row
   implicitly selects every file under that folder path.

## Files Touched

- `SubDlg.h`
- `SubDlg.cpp`

## Design Overview

### Row Descriptors (`listrow`)

Every row in the ListView now has its `LVITEM.lParam` pointing into a
member-owned array `kiArray<listrow> m_rows`:

```cpp
struct listrow { bool isFolder; int idx; };
```

- `isFolder == false` — `idx` is an index into `m_fileIndices` (so the file is
  `m_files[ m_fileIndices[idx] ]`).
- `isFolder == true`  — `idx` is an index into `m_folderPaths` /
  `m_treeNodes` (so the folder path is `m_folderPaths[idx]` and the
  corresponding tree node is `m_treeNodes[idx]`).

`m_rows` is pre-allocated in `onInit` to
`m_fileIndices.len() + m_folderPaths.len() + 1` via `kiArray::alloc` so that
subsequent `add()` calls never reallocate the backing storage. This keeps
pointers into `m_rows` stable for the lifetime of the dialog, even across
repeated `FilterListByFolder` rebuilds (which call `m_rows.empty()` and refill).

**Why not tagged pointers?** `arcfile` embeds `INDIVIDUALINFO`, which is
defined under `#pragma pack(1)` and is 558 bytes (odd). `sizeof(arcfile)` is
815 bytes, so elements of `kiArray<arcfile>` sit on arbitrary byte boundaries.
The low bit of `&m_files[i]` is therefore not guaranteed to be 0, ruling out
the usual "low-bit tag" trick.

### `FilterListByFolder(folderIdx)`

Rebuilds the ListView:

1. `clearSelections()` — reset the `selected` flag on every file entry.
2. `LVM_DELETEALLITEMS` and `m_rows.empty()`.
3. Determine `prefix` and `prefixLen` from `folderIdx`
   (`-1` means the root, i.e. top-level view).
4. Collect *immediate* child folders: a `m_folderPaths` entry whose `tail`
   (the part after `prefix`) contains exactly one separator, placed at the end.
5. Sort the collected folder indices by leaf name (case-insensitive,
   insertion sort).
6. Insert folder rows first (icon = `m_folderIconIdx`, cached in `onInit`
   from `SHGetFileInfo("folder", FILE_ATTRIBUTE_DIRECTORY, ...)`).
7. Insert file rows that are directly in the current folder.

### `setSelection()` (moved from header to `SubDlg.cpp`)

For each selected ListView row:

- **File row** — mark the corresponding `arcfile.selected = true`.
- **Folder row** — recursively mark every file whose full path starts with
  the folder's prefix (`m_folderPaths[idx]`, which ends in a separator). This
  means Extract Each / D&D / SendTo automatically include all descendants.

`clearSelections()` is already invoked by every caller before `setSelection()`,
so the pass is purely additive.

### Sort Callback (`lv_compare`)

The callback is `static`, so it uses a file-scope `s_sortCtx` pointer set by
`DoSort` for the duration of `LVM_SORTITEMS` to resolve row descriptors back
to `arcfile*` / folder paths.

Rules:

- Folder rows always sort above file rows, regardless of ascending/descending.
- Folder-vs-folder compares leaf names case-insensitively, ignoring the
  column type (folders have no meaningful size/date/ratio/method).
- File-vs-file follows the original per-column logic.

### Navigation (double-click / View button)

- `NM_DBLCLK` on the ListView: if the clicked row is a folder,
  `TVM_SELECTITEM` on `m_treeNodes[r->idx]`. The synchronous
  `TVN_SELCHANGED` triggers `FilterListByFolder`, which repopulates the list.
- `IDC_SHOW` (View button): if any selected row is a folder, navigate to the
  first one (same `TVM_SELECTITEM` path). Otherwise, the existing
  extract-to-tempdir + `ShellExecute` path runs.

### Initial Population

The original code inserted every file into the ListView in `onInit`, then
relied on the `TVM_SELECTITEM(hAll)` side-effect to reshape it via
`TVN_SELCHANGED`. The new code skips the initial per-file insertion (only
builds `m_fileIndices`) and calls `FilterListByFolder(-1)` explicitly after
the tree's root is selected, so that the initial list content does not depend
on whether `TVN_SELCHANGED` is delivered while the dialog is still
initializing.

## Known Issues / To Investigate

The user reported that "things look off" after the first end-to-end run, but
the conversation context was nearly exhausted before we could enumerate them.
Pick up here:

- Verify initial layout: does the root view show both top-level folders and
  top-level files correctly? Sort order?
- Verify that double-clicking a folder navigates exactly once (no double
  rebuild) and that focus behaviour is reasonable.
- Verify Extract Each with only folder selection behaves as expected
  (recursive contents land in destination directory with their relative
  paths preserved — this is handled by `CArchiver::melt`, not by us).
- Verify D&D dragging a folder gives all descendants.
- Verify IDC_SELECTINV (invert selection) interaction: it flips selection on
  folder rows too. The subsequent View/Extract action then either navigates
  (first selected folder wins) or recursively expands — think about whether
  this is the desired UX.
- Verify `NM_DBLCLK` behaviour on Common Controls versions older than v6:
  the code casts `lp` to `NMITEMACTIVATE*` and reads `iItem`, which relies
  on the struct layout (NMITEMACTIVATE extends NMHDR with iItem next).
- If the archive contains entries whose path separator style is mixed
  (`/` and `\` in different entries for the same logical folder),
  `BuildFolderTree` will create two separate folder nodes. Pre-existing
  limitation, but folder rows will reflect it.
- Consider whether `IDC_SELECTINV` should skip folder rows to avoid
  confusing recursive-selection side effects.

## Entry Points for Debugging

- `SubDlg.cpp::FilterListByFolder` — row enumeration and insertion.
- `SubDlg.cpp::setSelection` — selection → `arcfile.selected` mapping.
- `SubDlg.cpp::lv_compare` + `s_sortCtx` — sort ordering.
- `SubDlg.cpp::proc` — `NM_DBLCLK` and `IDC_SHOW` folder branches.
- `SubDlg.h::listrow` — descriptor layout stored in `LVITEM.lParam`.
