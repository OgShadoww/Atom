# Atom – terminal text editor (1 Week left)

Atom is a hobby text editor implemented in C for Unix-like terminals. It draws
inspiration from modal editors such as Vim and focuses on providing a compact
codebase that can be compiled with nothing more than a C compiler and the
standard POSIX library.

This project is an exercise in building a terminal application from scratch, handling raw terminal I/O, managing file buffers, and implementing a modal user interface without external dependencies.

### Nearest goals:
    1. Finish viewing mode:   
        - Saving your x cordinate even after zero lines (Done)   
        - Fully working scrolling in y  (Done)   
        - Scale of terminal sizes to the maximum width of the line or creating x scroll or smart wrap system
    2. Creating line number in bottom and name of the file (Done)   
    3. Create main menu if run atom without file path   (Done)
    4. Inserting mode   (Half done)
    5. Highting of syntax   
    6. Atom . functoin   
    7. Command mode basics (Done)
    8. Fix the bug with last line printing (Done)
    9. Colors functions 
    10. Inserting a enter new line, tab and more cases for inserting mode

---

## ✨ Features

- **Modal Workflow**: Switch between different modes for efficient text manipulation (Viewing, Inserting, Command).
- **Terminal-Native UI**: Renders its interface using ANSI escape sequences, ensuring it runs in most POSIX-compliant terminals without needing libraries like ncurses.
- **File Manipulation**: Open, display, and edit plain-text files directly in the terminal.
- **Status Bar**: Displays the current file name, cursor line, and column number.
- **Vim-like Controls**: Uses `hjkl` keys for navigation, providing a familiar experience for users of modal editors.
- **Startup Menu**: Presents a menu when launched without a file argument.

## 🏗️ Project Structure

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

## 🚀 Getting Started

### Prerequisites

- A C11-compatible compiler (e.g., GCC or Clang)
- A POSIX-compliant environment (Linux, macOS, WSL, etc.)
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

## 🎛️ Controls

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


## 🤝 Contributing

Pull requests and issue reports are welcome. Key areas for contribution include improving buffer management, expanding the set of available commands, and enhancing the rendering loop.

## 📄 License

This project is distributed under the MIT License. See [`LICENSE`](LICENSE) for more details.
