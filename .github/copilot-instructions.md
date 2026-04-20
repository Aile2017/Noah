# Copilot instructions for Noah

## Prerequisites

- Reply to repository users in Japanese.
- Write the body of any repository documentation file (`*.md`, `*.txt`, `*.html`) in English.
- Read the relevant existing files before proposing or making changes.
- Before making a change larger than about 200 lines, present a plan and get confirmation.

## Application overview

Noah is a lightweight Windows drag-and-drop archiver frontend (x64, Windows 10/11).
Dropping files or folders onto the app compresses them; dropping an archive extracts it.
Noah has no built-in archive engine — all compression and extraction is delegated to external
tools via **B2E scripts** placed in the `b2e\` subdirectory beside the executable.

### Operating modes (configurable)

| Mode | Behavior |
|---|---|
| 0 — Compress only | Always compresses |
| 1 — Compress preferred | Compresses unless clearly an archive |
| 2 — Extract preferred *(default)* | Extracts archives; compresses other files |
| 3 — Extract only | Always extracts |

### B2E script engine

- B2E means "Bridge To Executables".
- Each `.b2e` file defines `load:`, `encode:`, `decode:`, `sfx:`, and `list:` sections using a Lisp-like S-expression syntax evaluated by the Rythp VM (`kilib\kl_rythp.cpp`).
- Format support is extended by adding one `.b2e` file per archiver under `Release\b2e\`.
- Sample scripts for zip, 7z, lzh, rar, cab, tar/gz/bz2/xz/zst etc. are included in `Release\b2e\`.

### Settings

Settings are stored in `Noah.ini` beside the executable, using the Windows user name as the INI section name.

## Tech stack

Build environments: MSVC (VS 2022+), MSYS2 (ucrt64 / mingw64 / clang64).

| Toolchain | Makefile | Output |
|---|---|---|
| MSVC nmake | `Makefile` | `Release\Noah.exe` |
| MSVC msbuild | `Noah.sln` | `x64\Release\Noah.exe` |
| MSYS2 ucrt64 (default) | `Makefile.msys2` | `Release_ucrt64\Noah.exe` |
| MSYS2 mingw64 | `Makefile.msys2 TOOLCHAIN=mingw64` | `Release_mingw64\Noah.exe` |
| MSYS2 clang64 | `Makefile.msys2 TOOLCHAIN=clang64` | `Release_clang64\Noah.exe` |

MSVC is installed at:
`C:\Program Files\Microsoft Visual Studio\2022\Community`

MSYS2 is installed at:
`C:\usr\msys64`

## Project structure

```
Noah/
├── Noah.cpp / NoahApp.h       — Application entry point and main logic (CNoahApp)
├── NoahAM.cpp / NoahAM.h      — Archive manager: format dispatch, compress/extract routing
├── NoahCM.cpp / NoahCM.h      — Configuration manager: Noah.ini read/write, settings dialog
├── SubDlg.cpp / SubDlg.h      — Sub-dialogs: archive viewer, password input, progress bar
├── Archiver.cpp / Archiver.h  — CArchiver abstract base + CArcModule (external tool runner)
├── ArcB2e.cpp / ArcB2e.h      — B2E script engine (CArcB2e : CArchiver)
├── kilib/                     — Project-specific Win32 utility library
│   ├── kl_app.cpp/h           — kiApp: application base class
│   ├── kl_str.cpp/h           — kiStr, kiPath, StrArray
│   ├── kl_file.cpp/h          — kiFile, kiFindFile
│   ├── kl_wnd.cpp/h           — kiWindow, kiDialog, kiPropSheet
│   ├── kl_dnd.cpp/h           — Drag & drop (kiDataObject)
│   ├── kl_rythp.cpp/h         — Rythp scripting VM (kiRythpVM)
│   ├── kl_cmd.cpp/h           — Command-line parser (kiCmdParser)
│   └── kl_reg.cpp/h           — Registry / INI file access
├── Release/
│   ├── Noah.html              — User-facing help document (English)
│   └── b2e/                   — Sample B2E scripts (zip, 7z, lzh, rar, cab, tar, ...)
├── docs/
│   ├── ReadMe.txt             — Original Noah documentation (Shift-JIS, do not edit)
│   └── aboutb2e.txt           — B2E format description (Shift-JIS, do not edit)
├── Makefile                   — nmake (MSVC)
├── Makefile.msys2             — GNU make (MSYS2 toolchains)
└── Noah.sln / Noah.vcxproj    — Visual Studio solution
```

## Coding conventions

- Do not introduce STL, exceptions, or RTTI. The project is intentionally built without them
  (`/EHs-c- /GR-` for MSVC; `-fno-exceptions -fno-rtti` for MSYS2).
- The application entry point is `kilib_startUp`, not `WinMain`.
- Treat Noah's `kilib` as a project-specific library. Do not try to merge it with GreenPad's separate `kilib`.
- Keep changes surgical and consistent with the existing Win32 / raw API style.
- All source files include `stdafx.h` as a precompiled header.

## Important implementation details

- `NoahAM.cpp` intentionally converts paths to short 8.3 form with `kiPath::beShortPath()` before invoking archivers, for compatibility with older tools.
- When showing paths to users, prefer long paths via `GetLongPathName()`. If it fails because the final component does not exist yet, convert the parent directory first and reattach the leaf name.
- In `SubDlg.cpp`, `arcfile` contains `union { bool selected; bool isfile; }`. Writing one member changes the other because they share storage.
- `CArcViewDlg` uses `m_fileIndices` to track file entries safely. Before calling `setSelection()`, clear `selected` through `m_fileIndices` rather than touching `isfile`.
- Archive viewer dialog state is persisted through `CNoahConfigManager::getInt()` / `putInt()` using keys `ArcViewW`, `ArcViewH`, and `ArcViewSplit`.
- The archive viewer top row is dynamically laid out in `CArcViewDlg::LayoutTopRow()`: `Extract to:` caption, destination edit box, and browse button on the left; action buttons right-aligned.
- The tree/list splitter width in the archive viewer is intentionally narrow and centralized through `ARCVIEW_SPLITTER_WIDTH`.

## Makefile dependency rules

- If you change a header not pulled in through `stdafx.h`, do a clean rebuild.
- Keep both `Makefile` and `Makefile.msys2` dependency lists in sync when new non-PCH header dependencies are introduced.
- Known non-PCH header dependencies:

| Header | Affected sources |
|---|---|
| `Archiver.h` | `ArcB2e.cpp`, `Archiver.cpp`, `NoahAM.cpp`, `SubDlg.cpp` |
| `SubDlg.h` | `SubDlg.cpp`, `NoahAM.cpp` |
| `NoahAM.h` | `NoahAM.cpp` |
| `NoahCM.h` | `NoahCM.cpp`, `Noah.cpp`, `NoahAM.cpp`, `SubDlg.cpp` |

## Build / clean procedures

Working directory is the project root (`Noah/`).

### 1) MSVC — nmake

Prerequisites: open an **x64 Native Tools Command Prompt** for Visual Studio so that `cl`, `link`, `nmake` are on PATH.

Build:
```cmd
nmake
nmake CFG=Debug
```

Clean:
```cmd
nmake clean
nmake CFG=Debug clean
```

Output: `Release\Noah.exe`

---

### 2) MSVC — msbuild

Build:
```powershell
msbuild .\Noah.sln /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /m
```

Clean:
```powershell
msbuild .\Noah.sln /t:Clean /p:Configuration=Release /p:Platform=x64 /nologo
```

Output: `x64\Release\Noah.exe`

Notes:
- When driving MSVC from an MSYS2 shell, keep `/usr/bin` out of `PATH` so MSYS2 `link` does not shadow MSVC `link.exe`.
- `INCLUDE` and `LIB` must use Windows-style backslash paths for `cl.exe`.
- Avoid `cmd /c` wrappers; use `bash.exe`-driven setup instead when calling from MSYS2.

---

### 3) MSYS2 — ucrt64 (default)

Prerequisites: MSYS2 installed at `C:\usr\msys64` with the `mingw-w64-ucrt-x86_64-toolchain` package.

Build:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/ucrt64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2"
```

Clean:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/ucrt64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2 clean"
```

Output: `Release_ucrt64\Noah.exe`

---

### 4) MSYS2 — mingw64

Prerequisites: `mingw-w64-x86_64-toolchain` package installed.

Build:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/mingw64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2 TOOLCHAIN=mingw64"
```

Clean:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/mingw64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2 TOOLCHAIN=mingw64 clean"
```

Output: `Release_mingw64\Noah.exe`

---

### 5) MSYS2 — clang64

Prerequisites: `mingw-w64-clang-x86_64-toolchain` package installed.

Build:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/clang64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2 TOOLCHAIN=clang64"
```

Clean:
```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/clang64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefile.msys2 TOOLCHAIN=clang64 clean"
```

Output: `Release_clang64\Noah.exe`

## Documentation policy

- `README.md` — main developer-facing overview (English).
- `Release\Noah.html` — user-facing help document (English).
- `docs\ReadMe.txt` and `docs\aboutb2e.txt` — original Shift-JIS source documents; do not edit.
