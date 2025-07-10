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

#define INSERTING 0
#define VIEWING 1
#define COMMAND_MODE 2

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
  int scroll_y;
} Window;

// Buffer to keep all importante things in one place
typedef struct Buffer {
  int mode;
  Line *document;
  int document_size;
  int document_capacity;
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
  for (int i = 0; i < Buff.document_size; ++i) {
    free(Buff.document[i].line);
    Buff.document[i].line = NULL;
    Buff.document[i].size = 0;
  }
  Buff.document_size = 0;


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
  Win.height = w.ws_row;
  Win.width = w.ws_col;
  Win.scroll_y = 0;
}

void move_cursor(int x, int y) {
  if(x >= 0 && x <= Win.width && y >= 0 && y <= Win.height) {
    Buff.cursor.x = x;
    Buff.cursor.y = y;
    char buff[32];
    snprintf(buff, sizeof(buff), "\033[%d;%dH", y, x);
    write(STDOUT_FILENO, buff, strlen(buff));
  }
}

void write_char(char c) {

}

// Drawing editor to screen
void draw_editor() {
  ansi_emit(ANSI_CLEAR);
  if(Buff.document_size < Win.height) {
    for(int i = 0; i < Buff.document_size; i++) {
      write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
    } 
  }
  else {
    for(int i = 0; i < Win.height; i++) {
      write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
    } 
  }
}

void editor_key_press(int mode) {
  char c;
  if(mode == VIEWING) {
    while(1) {
      read(STDIN_FILENO, &c, sizeof(c)); 
      switch (c) {
        case 'h': 
          //draw_editor();
          move_cursor(Buff.cursor.x - 1, Buff.cursor.y);
          break;
        case 'l':
          //draw_editor();
          move_cursor(Buff.cursor.x + 1, Buff.cursor.y);
          break;
        case 'j': 
          //draw_editor();
          move_cursor(Buff.cursor.x, Buff.cursor.y - 1);
          break;
        case 'k': 
          //draw_editor();
          move_cursor(Buff.cursor.x, Buff.cursor.y + 1);
          break;
        case 'q':
          return;
      }
    }
  }
}

//If we need more memory for our buffer
void ensure_document_capacity() {
  if(Buff.document_size >= Buff.document_capacity) {
    Buff.document_capacity *= 2;
    Buff.document = realloc(Buff.document, sizeof(Line) * Buff.document_capacity);
    if(Buff.document) {
      perror("Realloc failled");
      exit(1);
    }

    for(int i = Buff.document_size; i < Buff.document_capacity; i++) {
      Buff.document->line = NULL;
      Buff.document->size = 0;
    }
  }
}

// Initialization of Buffer 
void init_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  move_cursor(Buff.cursor.x, Buff.cursor.y);
  Buff.mode = VIEWING;
  Buff.document_capacity = 64;
  Buff.document_size = 0;
  Buff.document = malloc(sizeof(Line) * Buff.document_capacity);
  if(!Buff.document) {
    perror("Malloc failled");
    exit(1);
  }

  for(int i = 0; i < Buff.document_size; i++) {
    Buff.document[i].line = NULL;
    Buff.document[i].size = 0;
  }
}

int main(int arg, char **file) {
  if (arg < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", file[0]);
    exit(1);
  }  

  init_editor();
  open_editor(file[1]);
  create_window(Buff.document);
  enable_raw_mode();
  draw_editor();
  editor_key_press(Buff.mode);
 
  disable_raw_mode();
  return 0;
}
