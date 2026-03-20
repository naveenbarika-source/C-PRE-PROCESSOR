# My Pre-Processor

A modular C preprocessor that replicates the core preprocessing stages of a real C compiler frontend. Given a `.c` source file it produces an expanded `.i` file by performing **file inclusion**, **macro substitution**, and **comment removal** — in that order.

---

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Architecture](#architecture)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Examples](#examples)
- [Module Details](#module-details)
- [Known Limitations](#known-limitations)
- [Requirements](#requirements)

---

## Features

| Feature | Details |
|---|---|
| **File Inclusion** | Expands `#include <header.h>` by inlining the named system header |
| **Macro Substitution** | Collects `#define` directives and replaces all occurrences in source |
| **Object-like macros** | e.g. `#define PI 3.14` |
| **Function-like macros** | e.g. `#define SQUARE(x) ((x)*(x))` |
| **Single-line comment removal** | Strips `//` comments (with heuristic to preserve URLs) |
| **Block comment removal** | Strips `/* ... */` comments spanning one or more lines |
| **Modular design** | Each processing stage is isolated in its own `.c` / `.h` file |
| **Makefile build** | Single `make` command compiles and links the executable |

---

## Project Structure

```
mypreprocessor/
├── preprocessor.h      # Central header — shared types, constants, prototypes
├── main.c              # Entry point, file I/O, pass orchestration
├── comment_removal.c   # Single-line and block comment stripping
├── file_inclusion.c    # #include <...> directive expansion
├── macro_handler.c     # #define collection and textual substitution
├── Makefile            # Build rules
└── README.md           # This file
```

---

## Architecture

The preprocessing pipeline runs in five sequential passes over the line-buffer:

```
Source .c file
      │
      ▼
 ┌─────────────┐
 │  readfile() │  ← main.c
 │  Read all   │    Loads every line into a dynamic 2-D char array
 │  lines into │
 │  ptr[][]    │
 └──────┬──────┘
        │
        ▼
 ┌──────────────────┐
 │  Pass 1          │  ← file_inclusion.c
 │  File Inclusion  │    #include <...> lines → inlined header content
 └──────┬───────────┘
        │
        ▼
 ┌──────────────────┐
 │  Pass 2          │  ← macro_handler.c
 │  Macro Collect   │    #define lines → Macro_t records[]
 └──────┬───────────┘
        │
        ▼
 ┌──────────────────┐
 │  Pass 3          │  ← macro_handler.c
 │  Macro Expand    │    Replace macro names with bodies in all lines
 └──────┬───────────┘
        │
        ▼
 ┌──────────────────┐
 │  Pass 4          │  ← comment_removal.c
 │  Comment Strip   │    Remove // and block comments in-place
 └──────┬───────────┘
        │
        ▼
 ┌──────────────────┐
 │  Pass 5          │  ← main.c
 │  Write Output    │    Write non-directive lines to .i file
 └──────────────────┘
        │
        ▼
   Output .i file
```

---

## Build Instructions

### Prerequisites

- GCC (or any C11-compatible compiler)
- GNU Make
- A Linux/Unix system with standard headers in `/usr/include/`

### Build

```bash
git clone <your-repo-url>
cd mypreprocessor
make
```

This compiles all four `.c` modules and links them into the `mypreprocessor` executable.

### Clean

```bash
make clean
```

### Rebuild from scratch

```bash
make rebuild
```

---

## Usage

```bash
./mypreprocessor <inputfile.c>
```

The output file is automatically named by replacing the `.c` extension with `.i` and is written to the current directory.

```bash
./mypreprocessor hello.c        # produces hello.i
./mypreprocessor src/program.c  # produces src/program.i
```

---

## Examples

### Input: `example.c`

```c
#include <stdio.h>

#define PI     3.14159
#define SQUARE(x) ((x)*(x))

/* Compute the area of a circle */
int main(void)
{
    // radius
    double r = 5.0;
    double area = PI * SQUARE(r);
    printf("Area = %f\n", area);
    return 0;
}
```

### Run

```bash
./mypreprocessor example.c
```

### Result: `example.i` (excerpt)

```c
/* ... contents of stdio.h with comments stripped ... */

int main(void)
{
    double r = 5.0;
    double area = 3.14159 * ((r)*(r));
    printf("Area = %f\n", area);
    return 0;
}
```

---

## Module Details

### `preprocessor.h`

The single shared header included by every module. Defines:

- **Constants** — `MAX_LINE_LEN`, `MAX_MACRO_NAME`, `MAX_MACRO_DEF`, `MAX_MACROS`
- **`Macro_t`** — struct holding one `name` + `def` pair for a `#define`
- **Extern globals** — `count` (line count) and `store` (header line count)
- **All function prototypes** — one authoritative declaration per function

---

### `main.c`

Owns program entry, file I/O, and pass orchestration.

| Function | Purpose |
|---|---|
| `main()` | Validates args, builds output filename, calls readfile + write_on_prep |
| `readfile()` | Opens source, reads lines into a `realloc`-grown 2-D array |
| `write_on_prep()` | Runs all five passes and writes the `.i` file |

---

### `comment_removal.c`

Strips C comments from the line-buffer in-place.

| Function | Purpose |
|---|---|
| `remove_comments()` | Main entry point: iterates lines, delegates to helpers |
| `remove_line_range()` | Deletes lines `[i..j]`, preserving trailing code after `*/` |

**Single-line (`//`) logic:**  
Searches for `//`. If no `;` precedes it on the line, erases from `//` to end-of-line, keeping the newline character.

**Block (`/* */`) logic:**  
Finds `/*`. Scans forward for the matching `*/`. If on the same line, splices it out in-place. If multi-line, erases from `/*` to end-of-line i, then calls `remove_line_range()` to collapse lines `i+1..j`.

---

### `file_inclusion.c`

Expands `#include <…>` directives.

| Function | Purpose |
|---|---|
| `process_includes()` | Extracts header name, reads `/usr/include/<name>`, strips its comments, writes to output |

The included header content is written to the `.i` file **before** the source code body, matching the behaviour of a real preprocessor.

---

### `macro_handler.c`

Handles `#define` directives.

| Function | Purpose |
|---|---|
| `collect_macro()` | Parses one `#define` line into a `Macro_t` name+body record |
| `apply_macros()` | Replaces every macro name occurrence in every line |

Multi-token bodies (e.g. `#define MSG Hello World`) are joined with spaces. Trailing newlines are stripped from the body before storage. Replacement is repeated per line until no further occurrences exist, then advances past the inserted text to prevent infinite loops.

---

## Known Limitations

- **Angle-bracket includes only** — `#include "file.h"` (user headers) is not supported.
- **No recursive inclusion** — headers included by a header are not expanded.
- **Textual macro matching** — macro names are matched with `strstr`, so `MAX` would also match inside `MAXIMUM`. A word-boundary check is not implemented.
- **Line length cap** — lines longer than 200 characters are truncated.
- **No conditional compilation** — `#ifdef`, `#ifndef`, `#if`, `#endif` are not handled.
- **No `#undef`** — macros cannot be undefined once collected.

---

## Requirements

| Requirement | Version |
|---|---|
| GCC | ≥ 4.9 (C11 support) |
| GNU Make | ≥ 3.81 |
| OS | Linux (relies on `/usr/include/` for system headers) |

---

## 👨‍💻 Author
**Barika Naveen**  
🎓 EEE Student | Embedded Systems Enthusiast  
💡 Interested in ARM, Embedded C & IoT  

---

## ⭐ Support
If you like this project:
- ⭐ Star this repository  
- 🍴 Fork it  
- 🛠️ Contribute improvements  

Happy Coding! 🚀
