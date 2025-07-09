#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <wctype.h>
#include <sys/ioctl.h>

// ---------------
// MAIN STRUCTRES
// ---------------

// Max amount of editors' lines
#define MAX_LINES 1000

// Ansi codes for control terminal
enum AnsiCode {
  ANSI_EXIT,
  ANSI_CLEAR,
  ANSI_CLEAR_LINE,
  ANSI_CURSOR_HIDE,
  ANSI_CURSOR_SHOW,
  ANSI_CURSOR_LEFT,
  ANSI_CURSOR_RIGHT,
  ANSI_CURSOR_TOP,
  ANSI_CURSOR_DOWN
};

//Array with ansi codes
const char *ansi_codes[] = {  
  [ANSI_EXIT] = "\b \b",
  [ANSI_CLEAR] = "\033[2J",
  [ANSI_CLEAR_LINE] = "\033[2K",
  [ANSI_CURSOR_HIDE] = "\033[?25l",
  [ANSI_CURSOR_SHOW] = "\033[?25h",
  [ANSI_CURSOR_LEFT] = "\033[1D",
  [ANSI_CURSOR_RIGHT] = "\033[1C",
  [ANSI_CURSOR_TOP] = "\033[1A",
  [ANSI_CURSOR_DOWN] = "\033[1B",
};

// Fucntion to emit ansi codes and run it
void ansi_emit(enum AnsiCode code) {
  write(STDOUT_FILENO, ansi_codes[code], strlen(ansi_codes[code]));
}

struct termios OriginalTermios;

// Document line
typedef struct Line {
  int size;
  char *line;
} Line;

// Cursor 
typedef struct Cursor {
  int x;
  int y;
} Cursor;

// Window that keep sizes of terminal and screen that include text from doucument that can be placed in screen
typedef struct Window {
  int width;
  int height;
  Line *screen;
} Window;

// Buffer to keep all importante things in one place
typedef struct Buffer {
  int mode;
  Line document[MAX_LINES];
  int document_size;
  Cursor cursor;
} Buffer;

// Initialization
Buffer Buff;
Window Win;

// ----------
// MAIN CODE 
// ----------


// Open file and write data to Buffer
void open_editor(char *filen) {
  FILE *file = fopen(filen, "r");

  if (file == NULL) {
    perror("Error opening file");
    return;
  }

  char buffer[128];
  Buff.document_size = 0;
  while (fgets(buffer, sizeof(buffer), file)) {
    Buff.document[Buff.document_size].size = strlen(buffer);
    Buff.document[Buff.document_size].line = malloc(Buff.document[Buff.document_size].size + 1);
    strcpy(Buff.document[Buff.document_size].line, buffer);
    Buff.document_size++;
  }

  fclose(file);
}

// Functions to work with terminal text modes
void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &OriginalTermios);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &OriginalTermios);
  atexit(disable_raw_mode);

  struct termios raw = OriginalTermios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Set files in Window structure
void create_window(Line *doc) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); 
  Win.height = w.ws_col;
  Win.width = w.ws_row;

  Win.screen = malloc(Win.height);
  for(int i = 0; i < Win.height; i++) {
    Win.screen[i] = doc[i];
  }
}

// Initialization of Buffer 
void inti_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  Buff.mode = 1;

}

// Processing key pressing
void editor_key_press() {
  
}

int main(int arg, char **file) {
  if (arg < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", file[0]);
    exit(1);
  }  

  inti_editor();
  open_editor(file[1]);
  enable_raw_mode();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (c == 127) {
      ansi_emit(ANSI_EXIT);
    } else if (c == 'k') {
      ansi_emit(ANSI_CURSOR_TOP);
    } else if (c == 'j') {
      ansi_emit(ANSI_CURSOR_DOWN);
    } else if (c == 'l') {
      ansi_emit(ANSI_CURSOR_RIGHT);
    } else if (c == 'h') {
      ansi_emit(ANSI_CURSOR_LEFT);
    } else {
      write(STDOUT_FILENO, &c, 1);
    }  
  }


  disable_raw_mode();
  return 0;
}
