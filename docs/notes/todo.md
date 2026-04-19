# Noah Refactoring TODO

Items identified during code review. High-priority items have already been resolved.

---

## Completed

- [x] **D-3** `SubDlg.cpp::BuildFolderTree` — O(N×D×M) double linear search over folder paths.
  Replace with sorted index + binary search to achieve O(N×D×log M).

- [x] **A-1** `ArcB2e.cpp` — Same `switch(extnum())` block written twice in sequence
  (once for `(arc.XXX)`, once for `(arc.)`). Merged into single block.

- [x] **A-2** `NoahCM.cpp` — `@dir` encode/decode logic duplicated for
  Melt and Compress sections. Extracted `encode_dir` / `decode_dir` helpers.

- [x] **A-3** `NoahAM.cpp` — `arcname` construction with the
  `cAlternateFileName` ternary was copy-pasted in `do_listing` and `do_melting`. Simplified.

- [x] **A-6** `NoahAM.h / NoahAM.cpp` — Removed redundant `m_BasePath`; now uses
  `m_BasePathList[0]` throughout.

---

## Medium Priority — B2E Scripts (future)

---

## Medium Priority — B2E Scripts (future)

- [ ] **E-1** `7z.b2e` — `encode:` and `sfxd:` share 13 identical method-option lines;
  only the final `(cmd ...)` differs. Requires new Rythp macro/subroutine syntax to fix properly.

- [ ] **E-2** `tar.gz.bz2.xz.zst.liz.lz4.lz5.br.b2e` — 8 compression-method blocks follow
  the same pattern with only `-t<type>`, extension, and `-mx` level differing.
  Same prerequisite as E-1.

---

## Low Priority — Cleanup (Completed)

- [x] `kiPath::endwithyen()`, `kiFile::getSize64` — Confirmed unused; deleted from header and impl.
- [x] `kiRegKey` in `kl_reg.h/kl_reg.cpp` — Unused; deleted entire class.
- [x] `NoahCM.cpp::init()` `load(Melt)` pre-load — Redundant (Noah.cpp calls `load(All)` immediately after); removed.
- [x] `kiStr::isSame` → renamed to `equalsIgnoreCase` (kl_str.h, kl_str.cpp, Noah.cpp, Archiver.h).

Note: `kiStr::replaceToSlash()` is used in `kl_rythp.cpp` — kept.
Note: `kiListView` is used in `SubDlg.cpp` (3 sites) — kept.
