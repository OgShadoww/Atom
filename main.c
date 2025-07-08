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
#define MAX_LINES 1000

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

void ansi_emit(enum AnsiCode code) {
  write(STDOUT_FILENO, ansi_codes[code], strlen(ansi_codes[code]));
}

struct termios OriginalTermios;

typedef struct Line {
  int size;
  char *line;
} Line;

typedef struct Cursor {
  int x;
  int y;
} Cursor;

typedef struct Window {
  int width;
  int height;
  Line *screen;
} Window;

typedef struct Buffer {
  int mode;
  Line document[MAX_LINES];
  int document_size;
  Cursor cursor;
} Buffer;

Buffer Buff;

// ----------
// MAIN CODE 
// ----------


void open_file(char *filen) {
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

Window *create_window(Line *doc) {
  Window *win = malloc(sizeof(Window));

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); 
  win->height = w.ws_col;
  win->width = w.ws_row;

  win->screen = malloc(win->height);
  for(int i = 0; i < win->height; i++) {
    win->screen[i] = doc[i];
  }

  return win;
}

void inti_editor(char *file) {
  open_file(file);  
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;



  enable_raw_mode();
}

int main(int arg, char **file) {
  if (arg < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", file[0]);
    exit(1);
  }  

  inti_editor(file[1]);

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
