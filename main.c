#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

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
  [ANSI_CLEAR_LINE] = "^[[2K",
  [ANSI_CURSOR_LEFT] = "^[[1D",
  [ANSI_CURSOR_RIGHT] = "^[[1C",
  [ANSI_CURSOR_TOP] = "^[[1A",
  [ANSI_CURSOR_DOWN] = "^[[1B",
};

struct termios OriginalTermios;

typedef struct Line {
  int size;
  char *line;
} Line;

typedef struct Cursor {
  int x;
  int y;
} Cursor;

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

void inti_editor() {

}

int main(int arg, char **file) {
  if (arg < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", file[0]);
    exit(1);
  }
  open_file(file[1]);
  for (int i = 0; i < Buff.document_size; i++) {
    write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
  }

  enable_raw_mode();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (c == 127) {
      write(STDOUT_FILENO, ansi_codes[ANSI_EXIT], 3);
    }
    if(c == 107) {
      write(STDOUT_FILENO, ansi_codes[ANSI_CURSOR_TOP], 6);
    }
    if(c == 106) {
      write(STDOUT_FILENO, ansi_codes[ANSI_CURSOR_DOWN], 6);
    }
    if(c == 108) {
      write(STDOUT_FILENO, ansi_codes[ANSI_CURSOR_RIGHT], 6);
    }
    if(c == 104) {
      write(STDOUT_FILENO, ansi_codes[ANSI_CURSOR_LEFT], 6);
    }
    else {
      write(STDOUT_FILENO, &c, 1);
    }
  }

  return 0;
}
