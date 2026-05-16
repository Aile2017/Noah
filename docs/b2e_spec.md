# B2E (Bridge To Executables) Script Specification

**Applies to:** Noah 3.x  
**Original author:** k.inaba  
**This document:** based on the current source implementation (`ArcB2e.cpp`, `kl_rythp.cpp`)

---

## 1. Overview

Noah delegates all compression and extraction work to external command-line tools through
B2E script files.  Each `.b2e` file describes one archiver tool: which executable to call,
what archive extensions it handles, and how to build the command line for each operation.

Script files are placed in the `b2e\` folder next to `Noah.exe` and are loaded
automatically at startup.

---

## 2. File Naming

```
<ext-list>.b2e
```

- The extension of the file must be `.b2e`.
- The rest of the filename encodes the archive extensions this script handles for
  **extraction**, listed in order and separated by dots.
- The last segment (just before `.b2e`) is always treated as part of the extension list,
  not as a separator.

| Filename | Handled extraction extensions |
|---|---|
| `zip.zipx.b2e` | `.zip`, `.zipx` |
| `tar.gz.bz2.xz.zst.liz.lz4.lz5.br.b2e` | `.tar`, `.gz`, `.bz2`, … |
| `rar.b2e` | `.rar` |
| `zz.b2e` | `.zz` |

### Compress-only scripts

Prefix the filename with `#` to mark a script as **compression-only**.  
Noah will not route extraction requests to it; compression works normally.

```
#rar.b2e    ← compress only; will not be used for extraction
```

This works because the leading `#` shifts the first segment's length, causing Noah's
extension-matching logic to never select it for extraction.  The `#` prefix is
**not required** for compress-only scripts — it is a convention that makes the intent
explicit.

---

## 3. Quick Example

```
load:
 (name 7zG.exe)
 (type 7z Store *LZMA2 LZMA PPMd)

encode:
 (if (method 1) (let o "-m0=Copy"))
 (if (method 2) (let o "-mx=9 -m0=LZMA2"))
 (if (method 3) (let o "-mx=9 -m0=LZMA"))
 (if (method 4) (let o "-mx=9 -m0=PPMd"))
 (cmd a -t7z %o -r0 (arc.7z) (resp@ (listr)))

sfxd:
 (cmd a -t7z %o -r0 -sfx7z.sfx (arc.exe) (resp@ (listr)))

decode:
 (cmd x (arc))

decode1:
 (cmd x -y (arc) (list))

list:
 (xscan "------------------" 1 "------------------" 1 53 7z.exe l (arc))
```

---

## 4. Script Structure

A `.b2e` file is divided into named sections.  Each section starts with a label on
its own line (the label must begin at column 0):

| Section | Purpose |
|---|---|
| `load:` | Declare executable and compression format. **Required.** |
| `encode:` | Command to create a multi-file archive. |
| `encode1:` | Command to compress a single file; Noah loops it for multiple files. |
| `decode:` | Command to extract an entire archive. |
| `decode1:` | Command to extract selected files (used by the archive viewer). |
| `sfx:` | Command to convert an existing archive to a self-extracting archive. |
| `sfxd:` | Command to create a self-extracting archive directly (skips `encode:`). |
| `list:` | Command to enumerate archive contents (used by the archive viewer). |

Sections can appear in any order.  Only `load:` is required; the presence of other
sections determines which operations Noah offers:

| Section present | Capabilities unlocked |
|---|---|
| `decode:` | Extraction |
| `decode1:` | Selective extraction + archive viewer |
| `decode1:` + `list:` | Archive viewer with file list |
| `encode:` | Compression (multi-file) |
| `encode1:` | Compression (single-file mode) |
| `encode:` or `encode1:` + `sfx:` | Two-step SFX creation |
| `sfxd:` | Direct SFX creation |

---

## 5. `load:` Section

```
load:
 (name <executable>)
 (type <format-ext> <method1> <method2> ...)
 (use <file1> <file2> ...)
```

### `(name <executable>)`

Declares the external program that this script controls.  Noah searches for the file
using `SearchPath` (PATH + exe directory).  If not found, the script is silently
disabled (no compression or extraction).

```
(name 7zG.exe)
(name WinRAR.exe)
```

### `(type <ext> <method1> <method2> ...)`

Declares the output format for compression and the available quality levels.

- The first argument is the archive extension (e.g., `7z`, `zip`, `rar`).  
  This is independent of the filename extension list that controls extraction routing.
- Subsequent arguments are the compression method names shown to the user.
- Prefix a method name with `*` to mark it as the default.

```
(type 7z Store *LZMA2 LZMA PPMd Bzip2 Deflate)
(type zip Store *Deflate LZMA)
```

If `type` is omitted, the script can only extract (no compression).

### `(use <file1> <file2> ...)`

Lists auxiliary files that this archiver depends on (e.g., SFX stubs, DLLs).  
These files are displayed in the Noah version information dialog.  
They do not affect runtime behavior.

```
(use sfx32gui.dat)
```

---

## 6. `decode:` Section

Called when the user drops an archive onto Noah to extract it.

```
decode:
 (cmd x (arc))
```

Available substitutions:

| Expression | Expands to |
|---|---|
| `(arc ...)` | Archive path (see [Arc name expressions](#10-arc-name-expressions)) |
| `(dir)` | Destination folder (short filename, full path) |
| `(cmd ...)` | Run the declared executable |
| `(xcmd ...)` | Run any external executable |

---

## 7. `encode:` and `encode1:` Sections

Called when the user drops files onto Noah to compress them.

```
encode:
 (if (method 1) (let o "-m0=Copy"))
 (if (method 2) (let o "-mx=9"))
 (cmd a -tzip %o (arc.zip) (resp@ (listr)))
```

### Choosing between `encode:` and `encode1:`

- `encode:` — the executable receives all source files at once.
- `encode1:` — the executable accepts only one file at a time.  Noah calls the script
  once per file and merges the results into a single archive as needed.

### Available substitutions

| Expression | Expands to |
|---|---|
| `(arc.ext)` | Output archive path with extension `.ext` |
| `(dir)` | Source folder (short filename, full path) |
| `(list ...)` | Space-separated list of source files (see [List expressions](#11-list-expressions)) |
| `(listr ...)` | Same, but directories are expanded recursively by Noah |
| `(resp ...)` / `(resq ...)` | Write list to a response file (see [Response file expressions](#12-response-file-expressions)) |
| `(method)` | Current compression level number (1-based) |
| `(method N)` | `1` if current level equals N, else `0` |
| `(is_file)` | `1` if exactly one file was dropped, else `0` |
| `(is_folder)` | `1` if exactly one folder was dropped, else `0` |
| `(is_multiple)` | `1` if more than one item was dropped, else `0` |

---

## 8. `sfx:` and `sfxd:` Sections

### `sfx:` — two-step SFX creation

Noah first compresses via `encode:`, then calls `sfx:` to convert the result.

```
sfx:
 (cmd s -sfx (arc))
```

| Expression | Expands to |
|---|---|
| `(arc)` | Source archive path (name only, long filename) |
| `(arc.exe)` | Target SFX path |
| `(dir)` | Working directory |

### `sfxd:` — direct SFX creation

Replaces `encode:` entirely when creating SFX archives.  Use this when the tool
creates SFX files directly without a separate conversion step.

```
sfxd:
 (cmd a -t7z -sfx7z.sfx (arc.exe) (resp@ (listr)))
```

Available substitutions are identical to `encode:`.

---

## 9. `decode1:` and `list:` Sections

These two sections work together to enable the archive viewer.

### `decode1:`

Called by the archive viewer to extract selected files.  The special `(list)` function,
in this context, expands to the list of filenames selected by the user.

```
decode1:
 (cmd x -y (arc) (list))
```

### `list:`

Called to populate the archive viewer's file list.  Use `(scan)` or `(xscan)` to parse
the tool's output (see [scan / xscan](#13-scan-and-xscan)).

```
list:
 (xscan "------------------" 1 "------------------" 1 53 7z.exe l (arc))
```

Both sections must be present to show a file list in the viewer.  If only `decode1:` is
present (no `list:`), the viewer opens but cannot populate the list.

---

## 10. Arc Name Expressions

`(arc...)` returns the archive path.  The exact form is controlled by the suffix after
`arc` and an optional flag string.

### Syntax

```
(arc[modifier[.ext]] [flags])
```

### Modifiers

| Expression | Meaning |
|---|---|
| `(arc)` | Archive path as-is |
| `(arc.)` | Strip the last extension (controlled by Noah's extension-strip setting) |
| `(arc.ext)` | Replace the last extension with `.ext` |
| `(arc+.ext)` | Append `.ext` to the existing name |
| `(arc-.ext)` | Remove `.ext` if present; otherwise append `.decompressed` |

### Flag string (optional first argument)

| Flag | Meaning |
|---|---|
| `l` | Use long filename (default) |
| `s` | Use short (8.3) filename |
| `f` | Return full path (directory + name) |
| `n` | Return name only (no directory) |
| `d` | Return directory only |

Flags can be combined: `(arc.zip sf)` = short filename, full path, with `.zip` extension.

### Default flags by section

| Section | Default |
|---|---|
| `decode:`, `decode1:` | `lf` (long, full path) |
| `encode:`, `encode1:`, `sfxd:` | `lf` (long, full path) |
| `sfx:` | `ln` (long, name only) |

### Examples

Given archive `C:\programs\myarchive.lzh`:

| Expression | Result |
|---|---|
| `(arc d)` | `C:\programs\` |
| `(arc-.lzh sn)` | `MYARCH~1` (short, name only, `.lzh` stripped) |
| `(arc.cab lf)` | `"C:\programs\myarchive.cab"` |
| `(arc+.txt ln)` | `myarchive.lzh.txt` |

---

## 11. List Expressions

`(list...)` returns a space-separated list of filenames.

### In `encode:` / `encode1:` / `sfxd:` (compression mode)

```
(list)         ← name only, long filename
(list\*)       ← name only + wildcard suffix for directories: dir\*
(list\*.*)     ← name only + wildcard suffix for directories: dir\*.*
(listr)        ← Noah recursively expands directories itself
```

The optional flag string (second argument) controls filename format:

| Flag | Meaning |
|---|---|
| `l` | Long filename (default) |
| `s` | Short (8.3) filename |
| `f` | Full path |
| `n` | Name only (default) |

`(listr)` causes Noah to recurse into directories and list individual files; no
wildcard is appended to the command line.

**Recommended:** use `(listr)` when possible for maximum compatibility.  
Use `(list\*)` or `(list\*.*)` when the tool requires a wildcard to recurse itself.

### In `decode1:` (selective extraction mode)

`(list)` expands to the quoted filenames of the files selected in the archive viewer,
space-separated.  The flag variants and `listr` are not meaningful in this context.

---

## 12. Response File Expressions

Write the file list to a temporary file and pass the filename to the archiver.
This avoids command-line length limits.

```
(resp  (list\*))     ← pass as bare filename
(resp@ (listr))      ← pass as @filename
(resp-o (listr))     ← pass as -ofilename

(resq  (list\*))     ← same as resp, but strip double-quotes from entries
(resq@ (listr))
```

The prefix option (`@`, `-o`, or nothing) is prepended to the temporary filename
without a space.  Each entry in the response file is written on its own line.

`resp` preserves `"` characters in filenames; `resq` removes them (useful for tools
that do not accept quoted entries in response files).

---

## 13. `scan` and `xscan`

Used inside `list:` to parse a tool's text output and extract filenames.

### Syntax

```
(scan  BL BSL EL SL dx  cmd...)
(xscan BL BSL EL SL dx  EXE cmd...)
```

- `scan` runs the command using the executable declared in `(name ...)`.
- `xscan` runs the command using the explicitly named `EXE` executable.

### Parameters

| Parameter | Type | Meaning |
|---|---|---|
| `BL` | string | Begin-of-data marker: skip lines until a line starting with `BL` is found. `""` means start from line number `BSL`. |
| `BSL` | int | Number of lines to skip after the begin-marker line before reading data. |
| `EL` | string | End-of-data marker: stop before a line starting with `EL`. `""` means stop at an empty line. |
| `SL` | int | Read every `SL`-th line (e.g., `1` = every line, `2` = every other line). |
| `dx` | int | Number of characters to skip from the start of each data line. Negative value means skip that many whitespace-delimited tokens instead. |
| `cmd...` | expressions | Command arguments passed to the archiver for listing (e.g., `l (arc)`). |

### Examples

**7-Zip output** (data between `---` separator lines, filename at column 53):
```
(xscan "------------------" 1 "------------------" 1 53 7z.exe l (arc))
```

**imp.exe output** (data starts after `---` line, every 2 lines, column 0):
```
(scan "---" 1 "" 2 0 v (arc))
```

**Right-aligned filenames** (first token of each line, `dx = -1`):
```
(scan "--------" 1 "" 1 -1 l (arc))
```

---

## 14. Language Reference (Rythp)

B2E scripts are written in **Rythp**, a minimal Lisp-like scripting language.

### Syntax

Every expression is either:

- A **literal** string: `hello`, `"hello world"`, `-m5`, `0`
- A **function call**: `(name arg1 arg2 ...)`

Arguments that are themselves function calls are written as nested parentheses:
```
(cmd a -t7z (arc.7z) (resp@ (listr)))
```

### Variables

Variables are single ASCII letters `a`–`z` or `A`–`Z` (52 total).

```
(let x "some value")    ← assign
%x                      ← expand
```

Variables hold strings.  They can be used inside literal arguments: `-p%x`.

### Special character escapes

| Write | Produces |
|---|---|
| `%%` | `%` |
| `%"` | `"` |
| `%(` | `(` |
| `%)` | `)` |
| `%/` | newline |

These escapes are needed when a compression method name or command argument
literally contains `%`, `"`, `(`, or `)`.

### Control flow

#### `(exec stmt1 stmt2 ...)`
Evaluates each statement left to right.  Returns the value of the last statement.

#### `(if cond then [else])`
Evaluates `cond`.  If non-zero, evaluates and returns `then`; otherwise evaluates
and returns `else` (if provided).

#### `(while cond body)`
Evaluates `body` repeatedly while `cond` is non-zero.

### Variable assignment

#### `(let var val...)`
Concatenates all `val` arguments and assigns the result to variable `var`.
Returns the assigned value.

```
(let o "-mx=9")
(let f (arc.tar))
```

### Arithmetic and comparison

| Expression | Result |
|---|---|
| `(+ A B)` | A + B |
| `(- A B)` | A - B |
| `(* A B)` | A × B |
| `(/ A B)` | A ÷ B (integer) |
| `(mod A B)` | A mod B |
| `(= A B)` | `1` if A == B, else `0` |
| `(! A B)` | `1` if A != B, else `0` |
| `(! A)` | `1` if A == 0, else `0` (logical NOT) |
| `(< A B)` | `1` if A < B, else `0` |
| `(> A B)` | `1` if A > B, else `0` |
| `(between A B C)` | `1` if A ≤ B ≤ C, else `0` |

Comparison treats operands as integers when both look like integers, and as
strings otherwise.  `+` and `*` also serve as logical OR and AND respectively
in boolean contexts.

### Utility functions

#### `(slash A)`
Returns `A` with all `\` replaced by `/`.  Useful for tools that treat backslash
as an escape character.

#### `(find filename)`
Searches PATH for `filename`.  Returns the full path quoted if found, empty string
if not found.
```
(let s (find stubwin.sfx))
(if %s (cmd s %s (arc)))
```

#### `(size filename)`
Returns the size of `filename` in bytes as an integer string.

#### `(cd path)`
Changes the current directory to `path`.

#### `(del filename)`
Deletes `filename`.  Useful for cleaning up intermediate files.
```
(xcmd del %"%f%")
```

#### `(input MESSAGE [DEFAULT])`
Shows a dialog prompting the user for input.  `MESSAGE` is the prompt text;
`DEFAULT` is pre-filled text (optional).  Returns the entered string.
```
(let p (input "Password"))
(cmd x -p%p (arc))
```

### Execution functions

#### `(cmd arg1 arg2 ...)`
Runs the executable declared in `(name ...)` with the given arguments.
Spaces between arguments separate command-line parameters.
Returns the process exit code.
```
(if (! 0 (cmd a (arc.zip) (list))) (error-handling))
```

To pass a value that contains spaces as a single parameter, use a variable:
```
(let f (arc.tar))
(cmd a -tgzip %"%f.gz%" %"%f%")
```

#### `(xcmd exe arg1 arg2 ...)`
Runs `exe` (any executable or shell command) with the given arguments.
`exe` is looked up the same way as `(name ...)`.
```
(xcmd copy /b (find DecZipW.EXE) + (arc) (arc.exe))
(xcmd del %"%f%")
```

### `load:` section functions

These functions are only valid inside `load:`.

#### `(name <executable>)`
See [Section 5](#5-load-section).

#### `(type <ext> <method1> <method2> ...)`
See [Section 5](#5-load-section).

#### `(use <file1> <file2> ...)`
See [Section 5](#5-load-section).

### Query functions (encode / decode / sfx / list sections)

#### `(method [N])`
Available only in `encode:`, `encode1:`, and `sfxd:`.
- `(method)` — returns the current compression level (1-based integer).
- `(method N)` — returns `1` if the current level equals N, else `0`.

#### `(dir)`
Returns the working directory as a short-filename full path.
- In `decode:` / `decode1:`: the extraction destination folder.
- In `encode:` / `encode1:` / `sfxd:`: the folder containing the source files.
- In `sfx:`: the working (temp) directory.
- In `list:`: returns an empty string.

#### `(is_file)`
Returns `1` if exactly one file was dropped, else `0`.

#### `(is_folder)`
Returns `1` if exactly one folder was dropped, else `0`.

#### `(is_multiple)`
Returns `1` if more than one item was dropped, else `0`.

---

## 15. Complete Function Reference Summary

| Function | Available in | Description |
|---|---|---|
| `(exec ...)` | all | Sequence execution |
| `(if A B [C])` | all | Conditional |
| `(while A B)` | all | Loop |
| `(let var val)` | all | Variable assignment |
| `(+ A B)` | all | Add / logical OR |
| `(- A B)` | all | Subtract |
| `(* A B)` | all | Multiply / logical AND |
| `(/ A B)` | all | Divide |
| `(mod A B)` | all | Modulo |
| `(= A B)` | all | Equality |
| `(! A [B])` | all | Inequality / NOT |
| `(< A B)` | all | Less than |
| `(> A B)` | all | Greater than |
| `(between A B C)` | all | Range check |
| `(slash A)` | all | Replace `\` with `/` |
| `(name exe)` | `load:` | Declare executable |
| `(type ext m...)` | `load:` | Declare compression format |
| `(use file...)` | `load:` | Declare auxiliary files |
| `(arc...)` | encode, decode, sfx, list | Archive name |
| `(list...)` | encode, decode1 | File list |
| `(listr)` | encode | Recursively expanded file list |
| `(resp... args)` | encode, decode1 | Response file (preserve quotes) |
| `(resq... args)` | encode, decode1 | Response file (strip quotes) |
| `(cmd arg...)` | encode, decode, sfx, list | Run named executable |
| `(xcmd exe arg...)` | encode, decode, sfx, list | Run external executable |
| `(scan BL BSL EL SL dx cmd...)` | `list:` | Parse named-exe output for file list |
| `(xscan BL BSL EL SL dx EXE cmd...)` | `list:` | Parse external-exe output for file list |
| `(input MSG [DEF])` | encode, decode, sfx | Prompt user for input |
| `(dir)` | encode, decode, sfx | Working directory |
| `(cd path)` | encode, decode, sfx | Change current directory |
| `(method [N])` | encode, sfxd | Compression level query |
| `(is_file)` | encode | True if single file dropped |
| `(is_folder)` | encode | True if single folder dropped |
| `(is_multiple)` | encode | True if multiple items dropped |
| `(find filename)` | encode, decode, sfx | Search PATH for file |
| `(size filename)` | encode, decode, sfx | Get file size |
| `(del filename)` | encode, decode, sfx | Delete a file |

---

## 16. Implementation Notes

### Extension routing

Noah builds the extraction route by matching the dropped file's extension against
each loaded script's extension list (parsed from the filename).  The first matching
script that has `decode:` is used.

Compression routing uses the format extension declared in `(type ...)`, independent
of the script filename.

### `0.b2e`

A script named `0.b2e` is a convention for a helper-only entry.  The single `0`
in the extension list will never match a real archive extension, so the script is
never selected for extraction or compression.  Its only purpose is to declare
auxiliary tools via `(use ...)` so they appear in the Noah version information dialog.

### Features from the original specification that are not in the current implementation

The following features were described in the original `aboutb2e.txt` but are
**not implemented** in the current `ArcB2e.cpp`:

- **`(name EXE us)` — US locale mode.**  The `us` flag (second argument to `name`)
  was intended to run the archiver in US locale to avoid garbled output on Japanese
  Windows.  The current parser ignores the second argument.

- **DLL-based archivers and the `check:` section.**  The original specification
  described using `(name DLL名)` with a `check:` section to enable content-based
  archive detection and a richer `decode1:` for DLL APIs (Unlha32, UnZip32, etc.).
  The current `CArcModule` class supports only EXE and shell commands.

- **`Kill=` INI setting.**  The original specification mentioned disabling built-in
  DLL archiver routines via `Kill=<letters>` in `Noah.ini`.  The current Noah
  version does not use built-in DLL archivers; all archive support is via B2E scripts.

- **`OldAbout=1` INI setting.**  Deprecated; the current About dialog is unaffected
  by this setting.
