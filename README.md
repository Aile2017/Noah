# Noah

A lightweight Windows drag-and-drop archiver frontend (version 3.199).

Drop files onto Noah to compress them, or drop an archive onto Noah to extract it.  
Noah itself has no built-in archive engine — it delegates all archive operations to external tools via **B2E scripts** placed in the `b2e\` subdirectory.

---

## Features

- **Drag & Drop operation** — drop files/folders to compress, drop an archive to extract
- **Auto mode selection** — automatically detects whether to compress or extract based on what is dropped and the current mode setting
- **B2E script engine** — supports any archive format for which a B2E script and the corresponding external tool (e.g. 7-Zip, zip, tar …) are available
- **Archive viewer** — list archive contents before extracting; supports password entry
- **Configurable extraction destination** — fixed folder, same folder as source, or prompted each time
- **Subdirectory creation control** — choose when to create a folder for extracted files (never / single file / multiple files / always)
- **Multiple instance limit** — prevents too many simultaneous archive operations
- **Minimized start option** — launch in the system tray / taskbar

---

## Modes

Noah has four operating modes (configurable in Settings):

| Mode | Behavior |
|---|---|
| 0 — Compress only | Always compresses dropped items |
| 1 — Compress preferred | Compresses unless the drop target is clearly an archive |
| 2 — Extract preferred *(default)* | Extracts archives; compresses other files |
| 3 — Extract only | Always extracts dropped items |

---

## Requirements

- Windows 10 / 11 (x64)
- One or more B2E scripts and their corresponding archive tools in the `b2e\` subdirectory

---

## Building

Four toolchains are supported. All produce approximately the same binary size (~70 KB).

### MSVC (nmake)

```cmd
nmake                 # Release (default)
nmake CFG=Debug
nmake clean
```

Requires a **x64 Native Tools Command Prompt** for Visual Studio.

### MSYS2 — GCC (mingw64 / ucrt64) or Clang (clang64)

```bash
make -f Makefile.msys2                       # ucrt64 Release (default)
make -f Makefile.msys2 TOOLCHAIN=mingw64
make -f Makefile.msys2 TOOLCHAIN=clang64
make -f Makefile.msys2 CFG=Debug
make -f Makefile.msys2 clean
```

Requires [MSYS2](https://www.msys2.org/) with the corresponding toolchain package installed.

---

## Source Structure

```
Noah/
├── Noah.cpp / NoahApp.h      — Application entry, main logic
├── NoahAM.cpp / .h           — Archive manager (format dispatch)
├── NoahCM.cpp / .h           — Configuration manager and settings dialogs
├── SubDlg.cpp / .h           — Archive viewer, password, and progress dialogs
├── Archiver.cpp / .h         — Common archiver interface
├── ArcB2e.cpp / .h           — B2E script engine
├── b2e/                      — B2E scripts (not included; supply your own)
├── kilib/                    — K.I.LIB: Win32 utility library
├── Makefile                  — nmake (MSVC)
└── Makefile.msys2            — GNU make (MSYS2 toolchains)
```

---

## License

Basically free to use in any way. See `ReadMe.txt` for the original terms.

---

## Credits

### Application Icon

[Archiver - free Icon in PNG and SVG](https://icon-icons.com/icon/archiver/37045) by [icon-icons.com](https://icon-icons.com/), used under free for commercial use license.
