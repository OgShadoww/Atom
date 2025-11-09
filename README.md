# Atom – terminal text editor

Atom is a hobby text editor implemented in C for Unix-like terminals. It draws
inspiration from modal editors such as Vim and focuses on providing a compact
codebase that can be compiled with nothing more than a C compiler and the
standard POSIX library.

This project is an exercise in building a terminal application from scratch, handling raw terminal I/O, managing file buffers, and implementing a modal user interface without external dependencies.

---

## Project Structure

```
.
├── LICENSE
├── Makefile        # Build instructions for the editor
├── README.md       # Project overview and documentation
├── include/        # Header files and additional source modules
│   ├── file_browser.c
│   ├── menu.c
│   └── syntax_highlight.c
└── main.c          # Core editor implementation and terminal helpers
```

The editor keeps its state in a central `Buffer` structure that holds the
document, cursor position and active mode. Terminal interaction relies on
raw-mode configuration via `termios` and a small set of ANSI escape sequences.

## Getting Started

### Prerequisites

- A C11-compatible compiler (GCC or Clang)
- A POSIX-compliant environment (Linux, macOS, etc)
- `make` for building the project

### Build

To compile the editor, run the following command in the project root:

```bash
make
```
his will produce an executable file named `atom`.

### Run

To edit an existing file, provide its name as a command-line argument:

```bash
./atom test.txt
```

If you run the editor without any arguments, it will start with an empty buffer and display the main menu.
```bash
./atom
```

## Controls

| Key(s)         | Mode     | Action                          |
|----------------|----------|---------------------------------|
| `h`,`j`,`k`,`l`  | Viewing  | Move cursor                   |
| `i`              | Viewing  | Switch to Insert Mode         |
| `Esc`            | Insert   | Return to Viewing Mode        |
| `:`              | Viewing  | Switch to Command Mode        |
| `Enter`          | Command  | Execute the command           |
| `Esc`            | Command  | Return to Viewing Mode        |
| `:w` + `Enter`   | Command  | Save the file                 |
| `:q` + `Enter`   | Command  | Quit the editor               |
| `:wq` + `Enter`  | Command  | Save and quit the editor      |
