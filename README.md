# Atom – terminal text editor (2 Weeks left)

Atom is a hobby text editor implemented in C for Unix-like terminals. It draws
inspiration from modal editors such as Vim and focuses on providing a compact
codebase that can be compiled with nothing more than a C compiler and the
standard POSIX library.

### Nearest goals:
    1. Finish viewing mode:   
        - Saving your x cordinate even after zero lines (Done)   
        - Fully working scrolling in y  (Done)   
        - Scale of terminal sizes to the maximum width of the line or creating x scroll or smart wrap system
    2. Creating line number in bottom and name of the file   
    3. Create main menu if run atom without file path   
    4. Inserting mode   
    5. Highting of syntax   
    6. Atom . functoin   
    7. Command mode basics (Done)
    8. Fix the bug with last line printing (Done)

---

## ✨ Highlights

- **Modal workflow** – switching between insert, view and command modes.
- **Terminal-native UI** – rendering and cursor control powered by ANSI escape
  sequences without external dependencies.
- **File manipulation** – open, display and edit plain-text files directly in
  the terminal.
- **Configurable foundation** – the codebase is intentionally small to make it
  approachable for experimentation and learning.

## 🏗️ Project structure

```
.
├── LICENSE
├── Makefile        # Minimal build instructions (gcc main.c -o main)
├── main.c          # Editor implementation and terminal helpers
└── output.txt        # Sample file for manual testing
```

The editor keeps its state in a central `Buffer` structure that holds the
document, cursor position and active mode. Terminal interaction relies on
raw-mode configuration via `termios` and a small set of ANSI escape sequences.

## 🚀 Getting started

### Prerequisites

- GCC or another C11-compatible compiler
- POSIX environment (Linux, macOS, WSL, etc.)

### Build

```bash
make
```

This produces an executable named `main` in the project root. Clean builds can
be enforced with `make clean` (add this target if you need it).

### Run

```bash
./main test.txt
```

Replace `test.txt` with any text file you want to open. Running the executable
without arguments creates an empty buffer.

## 🎛️ Controls (current state)

| Key(s) | Mode            | Action                          |
|--------|-----------------|---------------------------------|
| `i`    | Viewing         | Switch to insert mode           |
| `Esc`  | Insert / Command| Return to viewing mode          |
| Arrow keys | Any         | Move cursor                     |
| `:q` + `Enter` | Command | Quit editor                     |

> ℹ️ The control scheme is still evolving. Check the source for the most
> up-to-date bindings while the project is in active development.

## 🧭 Roadmap

- [ ] Undo/redo support
- [ ] Syntax highlighting for C and other languages
- [ ] Status bar with file information
- [ ] Persistent configuration options
- [ ] Menu/command palette for discoverability

## 🤝 Contributing

Pull requests and issue reports are welcome. Focus areas include improving the
buffer management, expanding modal commands, and polishing the rendering loop.

## 📄 License

This project is distributed under the MIT License. See [`LICENSE`](LICENSE) for
details.

## 💡 Portfolio considerations

This project demonstrates systems-level programming, terminal UI handling, and
knowledge of editor internals. Enhancing the feature set (undo history, syntax
highlighting, tests, documentation) and polishing the user experience will make
it a compelling addition to a portfolio. Highlight the aspects you engineered
from scratch—especially raw terminal handling and data structures—to emphasize
your understanding of low-level development.
