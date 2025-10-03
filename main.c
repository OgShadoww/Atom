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

// Edtior modes
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

void free_editor() {
  for(int i = 0; i < Buff.document_size; i++) {
    if(Buff.document[i].line != NULL) {
      free(Buff.document[i].line);
    }
  }
  free(Buff.document);
}


void ensure_document_capacity() {
  if(Buff.document_size >= Buff.document_capacity) {
    Buff.document_capacity *= 2;
    Buff.document = realloc(Buff.document, sizeof(Line) * Buff.document_capacity);
    if(!Buff.document) {
      perror("Realloc failled");
      exit(1);
    }

    for(int i = Buff.document_size; i < Buff.document_capacity; i++) {
      Buff.document[i].line = NULL;
      Buff.document[i].size = 0;
    }
  }
}

// Open file and write data to Buffer
void open_editor(char *filen) {
  for (int i = 0; i < Buff.document_size; i++) {
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
    ensure_document_capacity();
    Buff.document[Buff.document_size].size = strlen(buffer);
    Buff.document[Buff.document_size].line = malloc(Buff.document[Buff.document_size].size + 1);
    if(!Buff.document[Buff.document_size].line) {
      perror("Malloc buffer line failled");
      fclose(file);
      return;
    }
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
void create_window() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); 
  Win.height = w.ws_row;
  Win.width = w.ws_col;
  Win.scroll_y = 0;
}

void draw_editor() {
  ansi_emit(ANSI_CLEAR);
  for(int i = Win.scroll_y; i < Win.height + Win.scroll_y - 1; i++) {
    write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
  }
  write(STDOUT_FILENO, "\x1b[H", 3);
  //fflush(stdout);
}

void scroll_window(int a) {
  if(a > 0) {
    Win.scroll_y++;
    draw_editor();  
  }
  if(a < 0) {
    Win.scroll_y--;
    draw_editor();
  }
}

void move_cursor(int x, int y) {
  if (y < 1) {
    y = 1;
    if(Win.scroll_y > 0) {
      scroll_window(-1);
    }
  }
  if(y >= Win.height - 1 && Win.scroll_y <= Buff.document_size - Win.height) {
    y = Win.height - 1;
    scroll_window(1);
  }
  if(y >= Win.height - 1 && Win.scroll_y > Buff.document_size - Win.height) {
    y = Win.height - 1;
  }
  int line_len = Buff.document[Win.scroll_y + (y - 1)].size;
  if(x < 1) {
    x = 1;
  }
  if(x >= line_len - 1) {
    x = line_len -1;
  }

  Buff.cursor.x = x;
  Buff.cursor.y = y; 
  dprintf(STDOUT_FILENO, "\033[%d;%dH", Buff.cursor.y, Buff.cursor.x);
}

void write_char(char c) {

}

void editor_key_press(int mode) {
  char c;
  if(mode == VIEWING) {
    while(1) {
      read(STDIN_FILENO, &c, sizeof(c)); 
      switch (c) {
        case 'h': 
          move_cursor(Buff.cursor.x - 1, Buff.cursor.y);
          fflush(stdout);
          break;
        case 'l':
          move_cursor(Buff.cursor.x + 1, Buff.cursor.y);
          fflush(stdout);
          break;
        case 'j': 
          move_cursor(Buff.cursor.x, Buff.cursor.y + 1);
          fflush(stdout);
          break;
        case 'k': 
          move_cursor(Buff.cursor.x, Buff.cursor.y - 1);
          fflush(stdout);
          break;
        case 'q':
          return;
      }
    }
  }
}

// Initialization of Buffer 
void init_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  Buff.mode = VIEWING;
  Buff.document_capacity = 64;
  Buff.document_size = 0;
  Buff.document = malloc(sizeof(Line) * Buff.document_capacity);
  if(!Buff.document) {
    perror("Malloc failled");
    exit(1);
  }

  for(int i = 0; i < Buff.document_capacity; i++) {
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
  create_window();
  enable_raw_mode();
  draw_editor();
  editor_key_press(Buff.mode);

 
  free_editor();
  disable_raw_mode();
  return 0;
}
