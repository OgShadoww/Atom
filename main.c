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
  ANSI_CLEAR_SCROLL,
  ANSI_CLEAR_LINE,
  ANSI_CURSOR_HIDE,
  ANSI_CURSOR_HOME,
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
  [ANSI_CLEAR_SCROLL] = "\033[3J",
  [ANSI_CLEAR_LINE] = "\033[2K",
  [ANSI_CURSOR_HIDE] = "\033[?25l",
  [ANSI_CURSOR_HOME] = "\033[1;1H", 
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
  int desired_x;
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
// HELPERS
// ----------

static int clamp(int v, int lo, int hi) {
  if(v > hi) return hi;
  if(v < lo) return lo;
  return v;
}

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
  ansi_emit(ANSI_CURSOR_HIDE);
  ansi_emit(ANSI_CLEAR);
  ansi_emit(ANSI_CLEAR_SCROLL);
  ansi_emit(ANSI_CURSOR_HOME);

  if(Buff.document_size < Win.height) {
    for(int i = 0; i < Buff.document_size; i++) {
      write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size); 
    } 
  }
  else {
    for(int i = Win.scroll_y; i < Win.scroll_y + Win.height - 1; i++) {
      if(i < Buff.document_size) {
        write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
      }
    }
  }
  
  // Showing cursor
  int screen_y = Buff.cursor.y - Win.scroll_y + 1;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", screen_y, Buff.cursor.x + 1);
  ansi_emit(ANSI_CURSOR_SHOW);
}


void enter_viewing_mode() {
  
}

// Cursor movements
void move_cursor_horizontaly(int direction) {
  if (Buff.document_size <= 0) return;  
  
  int doc_x = clamp(Buff.cursor.x + direction, 0, Buff.document[Buff.cursor.y].size - 1);
  Buff.cursor.desired_x = doc_x;
  Buff.cursor.x = doc_x;

  draw_editor();
}

void move_cursor_verticaly(int direction) {
  if(Buff.document_size <= 0) return;  
  
  int doc_y = clamp(Buff.cursor.y + direction, 0, Buff.document_size - 1);
  int doc_x = clamp(Buff.cursor.desired_x, 0, Buff.document[doc_y].size - 1);
  Buff.cursor.y = doc_y;
  Buff.cursor.x = doc_x;

  // Handle scrolling
  if (Buff.cursor.y > Win.height + Win.scroll_y - 2 && Buff.cursor.y < Buff.document_size - 1) {
    Win.scroll_y++;
  }
  if (Win.scroll_y > 0 && Buff.cursor.y < Win.scroll_y) {
    Win.scroll_y--;
  }

  draw_editor();
}

void enter_inserting_mode() {
  
}

void write_char(char c) {
  write(STDIN_FILENO, &c, sizeof(c));
}

void enter_command_mode() {
  Buff.mode = COMMAND_MODE;
  move_cursor_verticaly(Buff.document_size - 1);
  write(STDIN_FILENO, ":", 2);
  //write_char(':');
  draw_editor(); 
}

void process_command_mode() {

}

void exit_command_mode() {

}

void editor_key_press(int mode) {
  char c;
  if(mode == VIEWING) {
    while(1) {
      read(STDIN_FILENO, &c, sizeof(c)); 
      switch (c) {
        case 'h': move_cursor_horizontaly(-1); break;
        case 'l': move_cursor_horizontaly(1); break;
        case 'j': move_cursor_verticaly(1); break;
        case 'k': move_cursor_verticaly(-1); break;        
        case 'i': enter_inserting_mode(); break;
        case ':': enter_command_mode(); break;
        case 'q': return;
      }
    }
  }
}

// Initialization of Buffer 
void init_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  Buff.cursor.desired_x = 0;
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
  //write(STDOUT_FILENO, "\x1b[H", 3);
  editor_key_press(Buff.mode);

 
  free_editor();
  disable_raw_mode();
  return 0;
}
