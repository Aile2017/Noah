# Copilot instructions for Noah

## Project summary

Noah is a lightweight Windows drag-and-drop archiver frontend. Dropping files or folders onto the app compresses them, while dropping an archive extracts it. Noah does not implement archive formats itself: all archive operations are delegated to external tools through B2E scripts.

## Communication and documentation

- Reply to repository users in Japanese.
- Write the body of any repository documentation file (`*.md`, `*.txt`, `*.html`) in English.
- Read the relevant existing files before proposing or making changes.
- Before making a change larger than about 200 lines, present a plan and get confirmation.

## Architecture and key components

- `Noah.cpp` / `NoahApp.h`: application entry and main behavior
- `NoahAM.cpp` / `NoahAM.h`: archive manager and format dispatch
- `NoahCM.cpp` / `NoahCM.h`: configuration manager and settings dialogs
- `SubDlg.cpp` / `SubDlg.h`: archive viewer, password, and progress dialogs
- `Archiver.cpp` / `Archiver.h`: common archiver interface
- `ArcB2e.cpp` / `ArcB2e.h`: B2E integration
- `kilib\`: project-specific Win32 utility library, including the Rythp VM in `kilib\kl_rythp.cpp`
- `Release\b2e\`: practical sample B2E scripts

## B2E and runtime behavior

- B2E means "Bridge To Executables".
- Each `.b2e` file defines `load:`, `encode:`, `decode:`, `sfx:`, and `list:` sections using a Lisp-like S-expression syntax.
- Format support is extended by adding one `.b2e` file per archiver format under `Release\b2e\`.
- Settings are stored in `Noah.ini` beside the executable, using the Windows user name as the INI section name.

## Coding conventions

- Do not introduce STL, exceptions, or RTTI. The project is intentionally built without them.
- The application entry point is `kilib_startUp`, not `WinMain`.
- Treat Noah's `kilib` as a project-specific library. Do not try to merge it with `GreenPad`'s separate `kilib`.
- Keep changes surgical and consistent with the existing Win32 style.

## Important implementation details

- `NoahAM.cpp` intentionally converts paths to short 8.3 form with `kiPath::beShortPath()` before invoking archivers for compatibility with older tools.
- When showing paths to users, prefer long paths via `GetLongPathName()`.
- If `GetLongPathName()` fails because the final component does not exist yet, convert the parent directory first and then reattach the leaf name.
- In `SubDlg.cpp`, `arcfile` contains `union { bool selected; bool isfile; }`. Writing one member changes the other because they share storage.
- `CArcViewDlg` uses `m_fileIndices` to track file entries safely. Before calling `setSelection()`, clear `selected` through `m_fileIndices` rather than touching `isfile`.
- Archive viewer dialog state is persisted through `CNoahConfigManager::getInt()` and `putInt()` using `ArcViewW`, `ArcViewH`, and `ArcViewSplit`.
- The archive viewer top row is dynamically laid out in `CArcViewDlg::LayoutTopRow()`: keep the `Extract to:` caption, destination edit box, and browse button on the left, while keeping the action buttons right-aligned.
- The tree/list splitter width in the archive viewer is intentionally narrow and centralized through `ARCVIEW_SPLITTER_WIDTH`.

## Build guidance

- Preferred MSVC build command: `msbuild .\Noah.sln /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /m`
- `Makefile` is for MSVC (`nmake`), and `Makefile.msys2` is for MSYS2 toolchains.
- When driving MSVC from an MSYS2 shell, keep `/usr/bin` out of `PATH` so MSYS2 `link` does not override MSVC `link.exe`.
- `INCLUDE` and `LIB` must use Windows-style backslash paths for `cl.exe`.
- Avoid `cmd /c` for MSVC builds in this environment; use `bash.exe`-driven setup instead.

## Makefile dependency rule

- If you change a header that is not pulled in through `stdafx.h`, do a clean rebuild.
- Keep both `Makefile` and `Makefile.msys2` dependency lists in sync when new non-PCH header dependencies are introduced.
- Known non-PCH header dependencies:
  - `Archiver.h` -> `ArcB2e.cpp`, `Archiver.cpp`, `NoahAM.cpp`, `SubDlg.cpp`
  - `SubDlg.h` -> `SubDlg.cpp`, `NoahAM.cpp`
  - `NoahAM.h` -> `NoahAM.cpp`
  - `NoahCM.h` -> `NoahCM.cpp`, `Noah.cpp`, `NoahAM.cpp`, `SubDlg.cpp`

## Documentation policy

- `README.md` is the main developer-facing overview in English.
- `Release\Noah.html` is the user-facing help in English.
- `docs\ReadMe.txt` and `docs\aboutb2e.txt` are original Shift-JIS source documents and should usually not be edited.
