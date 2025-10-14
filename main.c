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
#define INSERTING_MODE 0
#define VIEWING_MODE 1
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
  ANSI_CURSOR_DOWN,
  ANSI_ERASE_CHARACTER,
  ANSI_CURSOR_BLOCK,
  ANSI_CURSOR_UNDERLINE,
  ANSI_CURSOR_BAR,
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
  [ANSI_ERASE_CHARACTER] = "\b \b",
  [ANSI_CURSOR_BLOCK] = "\033[2 q",
  [ANSI_CURSOR_UNDERLINE] = "\033[4 q",
  [ANSI_CURSOR_BAR] = "\033[6 q",
};

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
  int menu;
  Line *document;
  int document_size;
  int document_capacity;
  Cursor cursor;
  char *file_name;
} Buffer;

// Initialization
Buffer Buff;
Window Win;

// ---------------
// FUNCTIONS PROTOTYPES
// ---------------

// main.c
void ansi_emit(enum AnsiCode code);
static int clamp(int v, int lo, int hi);
void disable_raw_mode();
void enable_raw_mode();
void append_char(char c);
void delete_char();
void append_line();

void cmd_save_file();
void cmd_quit();

void ensure_document_capacity();
void create_window();
void draw_editor();
void move_cursor_horizontaly(int direction);
void move_cursor_verticaly(int direction);
void enter_inserting_mode();
void handle_inserting_input(char c);
void exit_inserting_mode();
void enter_command_mode();
void process_command_input(char *command);
void handle_command_input(char c);
void exit_command_mode();
void enter_viewing_mode();
void handle_viewing_input(char c);
void editor_key_press();
void free_editor();

// menu.c 
void start_menu(int win_h, int win_w);

// ----------
// HELPERS
// ----------

// Fucntion to emit ansi codes and run it
void ansi_emit(enum AnsiCode code) {
  write(STDOUT_FILENO, ansi_codes[code], strlen(ansi_codes[code]));
}

// clamp numbers between two numbers
static int clamp(int v, int lo, int hi) {
  if(hi < lo) return lo;
  if(v > hi) return hi;
  if(v < lo) return lo;
  return v;
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

// Inserting and deletign functions
void append_char(char c) {
  if(Buff.document_size == 0) {
    ensure_document_capacity();
    Buff.document_size = 1;
    Buff.document[0].size = 0;
    Buff.document[0].line = malloc(1);
    Buff.document_size = 1;
    Buff.cursor.x = 0;
    Buff.cursor.y = 0;
  }

  int original_size = Buff.document[Buff.cursor.y].size;
  int insert_pos = Buff.cursor.x;

  Buff.document[Buff.cursor.y].size++; 
  Buff.document[Buff.cursor.y].line = realloc(Buff.document[Buff.cursor.y].line, Buff.document[Buff.cursor.y].size + 1);
  for(int i = original_size - 1; i >= insert_pos; i--) {
    Buff.document[Buff.cursor.y].line[i + 1] = Buff.document[Buff.cursor.y].line[i];
  }
  
  Buff.document[Buff.cursor.y].line[insert_pos] = c;
  move_cursor_horizontaly(1);
}

void append_line() {
  if (Buff.document_size == 0) {
    ensure_document_capacity();
    Buff.document[0].size = 0;
    Buff.document[0].line = malloc(1);
    Buff.document[0].line[0] = '\0';
    Buff.document_size = 1;
    Buff.cursor.x = 0;
    Buff.cursor.y = 0;
    draw_editor();
    return;
  }

  int current_line = Buff.cursor.y;
  int split_pos = Buff.cursor.x;
  int current_size = Buff.document[current_line].size;

  ensure_document_capacity();

  for (int i = Buff.document_size; i > current_line + 1; i--) {
    Buff.document[i] = Buff.document[i - 1];
  }

  int new_line_size = current_size - split_pos;
  Buff.document[current_line + 1].size = new_line_size;
  Buff.document[current_line + 1].line = malloc(new_line_size + 1);

  if (new_line_size > 0) {
    memcpy(Buff.document[current_line + 1].line,
           &Buff.document[current_line].line[split_pos],
           new_line_size);
    Buff.document[current_line + 1].line[new_line_size] = '\0';
  }
  else {
    Buff.document[current_line + 1].line[0] = '\0';
  }

  Buff.document[current_line].size = split_pos;
  Buff.document[current_line].line = realloc(Buff.document[current_line].line, split_pos + 1);
  Buff.document[current_line].line[split_pos] = '\0';

  Buff.document_size++;
  Buff.cursor.y++;
  Buff.cursor.x = 0;
  Buff.cursor.desired_x = 0;

  draw_editor();
  fflush(stdout);
}

void delete_char() {
  int original_size = Buff.document[Buff.cursor.y].size;
  int delete_pos = Buff.cursor.x - 1;
   if(delete_pos < 0) {
    if(Buff.cursor.y > 0) {
      int prev_line = Buff.cursor.y - 1;
      int prev_size = Buff.document[prev_line].size;
      int current_size = Buff.document[Buff.cursor.y].size;
      
      Buff.document[prev_line].line = realloc(Buff.document[prev_line].line, 
                                             prev_size + current_size + 1);
      strcpy(&Buff.document[prev_line].line[prev_size], Buff.document[Buff.cursor.y].line);
      Buff.document[prev_line].size = prev_size + current_size;
      
      Buff.cursor.y = prev_line;
      Buff.cursor.x = prev_size;
      Buff.cursor.desired_x = prev_size;
      
      for (int i = Buff.cursor.y + 1; i < Buff.document_size; i++) {
        Buff.document[i - 1] = Buff.document[i];
      }
      Buff.document_size--;
    }

    draw_editor();
    return;
  } 

  for(int i = delete_pos; i < original_size - 1; i++) {
    Buff.document[Buff.cursor.y].line[i] = Buff.document[Buff.cursor.y].line[i+1];
  }
  Buff.document[Buff.cursor.y].size--;
  Buff.document[Buff.cursor.y].line = realloc(Buff.document[Buff.cursor.y].line, Buff.document[Buff.cursor.y].size + 1);

  move_cursor_horizontaly(-1);
  draw_editor();
}

// Colors manipulations
void set_text_color(enum AnsiCode code) {
  //ansi_emit()
}

// ----------
// COMMAND FUNCTIONS
// ----------

void cmd_save_file() {
  FILE *file = fopen(Buff.file_name, "w");
  if(!file) {
    perror("error loading file for save");
  }
  for(int i = 0; i < Buff.document_size; i++) {
    fputs(Buff.document[i].line, file);
  }

  fclose(file);

  exit_command_mode();
}

void cmd_quit() {
  free_editor();
  exit(0);
}

// ----------
// MAIN CODE 
// ----------

// Initialization of Buffer 
void init_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  Buff.cursor.desired_x = 0;
  Buff.mode = VIEWING_MODE;
  Buff.document_capacity = 512;
  Buff.document_size = 0;
  Buff.file_name = NULL;
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
    Buff.document[Buff.document_size].line = malloc(Buff.document[Buff.document_size].size +1);
    if(!Buff.document[Buff.document_size].line) {
      perror("Malloc buffer line failled");
      fclose(file);
      return;
    }
    strcpy(Buff.document[Buff.document_size].line, buffer);
    Buff.document_size++;
  }

  Buff.file_name = filen;
  fclose(file);
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

  // Render
  if(Buff.document_size < Win.height) {
    for(int i = 0; i < Buff.document_size; i++) {
      write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size); 
    } 
  }
  else {
    for(int i = Win.scroll_y; i < Win.scroll_y + Win.height - 2; i++) {
      if(i < Buff.document_size ) {
        write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
      }
    }
  }

  // Writing status bar
  dprintf(STDOUT_FILENO, "\033[%d;1H", Win.height);
  write(STDOUT_FILENO, "\033[2K", 4);
  dprintf(STDOUT_FILENO,"%s\t%d,%d", Buff.file_name, Buff.cursor.y + 1, Buff.cursor.x + 1);
  
  // Showing cursor
  int screen_y = Buff.cursor.y - Win.scroll_y + 1;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", screen_y, Buff.cursor.x + 1);
  write(STDOUT_FILENO, "\033[?7h", 5);
  ansi_emit(ANSI_CURSOR_SHOW);
}

// Functions for cursor movements
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
  if (Buff.cursor.y > Win.height + Win.scroll_y - 3 && Buff.cursor.y < Buff.document_size) {
    Win.scroll_y++;
  }
  if (Win.scroll_y > 0 && Buff.cursor.y < Win.scroll_y) {
    Win.scroll_y--;
  }

  draw_editor();
}

// Inserting mode
void enter_inserting_mode() {
  Buff.mode = INSERTING_MODE;
  ansi_emit(ANSI_CURSOR_BAR);
}

void handle_inserting_input(char c) {
  static char command_buffer;
  static int cmd_pos = 0;

  switch(c) {
    case 27: 
      exit_inserting_mode(); 
      break;
    case 127:
      delete_char();
      draw_editor();
      break;
    case 10:
      append_line();
      break;
    default:
      append_char(c); 
      draw_editor();
      break;
  }
}

void exit_inserting_mode() {
  enter_viewing_mode();
}

// Command mode
void show_command_mode() {
  dprintf(STDOUT_FILENO, "\033[%d;1H", Win.height);
  write(STDOUT_FILENO, "\033[2K", 4);
  write(STDOUT_FILENO, ":", 1);
  ansi_emit(ANSI_CURSOR_DOWN);
  fflush(stdout);
}

void enter_command_mode() {
  Buff.mode = COMMAND_MODE;
  show_command_mode();
}

void process_command_input(char *command) {
  if(strcmp(command, "q") == 0) {
    cmd_quit();
  }
  if(strcmp(command, "w") == 0) {
    cmd_save_file();
  }
  if(strcmp(command, "wq") == 0) {
    cmd_save_file();
    cmd_quit();
  }
  else {
    return;
  }
}

void handle_command_input(char c) {
  static char command_buffer[128];
  static int cmd_pos = 0;

  switch (c) {
    case 10:
      command_buffer[cmd_pos] = '\0';
      cmd_pos = 0;
      process_command_input(command_buffer);
      break;
    case '\033':
      cmd_pos = 0;
      enter_viewing_mode();
      break;
    case 127:
      if(cmd_pos > 0) {
        cmd_pos--;
        ansi_emit(ANSI_ERASE_CHARACTER);
      }
      else {
        exit_command_mode();
        break;
      }
      break;
    default: 
      if(cmd_pos < 128) {
        command_buffer[cmd_pos] = c;
        cmd_pos++;
        dprintf(STDOUT_FILENO, "%c", c);
      }
      break;
  }
}

void exit_command_mode() {
  ansi_emit(ANSI_ERASE_CHARACTER);
  int screen_y = Buff.cursor.y - Win.scroll_y + 1;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", screen_y, Buff.cursor.x + 1);
  draw_editor();
  Buff.mode = VIEWING_MODE;
}

// Viewing mode
void enter_viewing_mode() { 
  move_cursor_horizontaly(-1);
  Buff.mode = VIEWING_MODE;
  ansi_emit(ANSI_CURSOR_BLOCK);
}

void handle_viewing_input(char c) {
  switch (c) {
    case 'h': move_cursor_horizontaly(-1); break;
    case 'l': move_cursor_horizontaly(1); break;
    case 'j': move_cursor_verticaly(1); break;
    case 'k': move_cursor_verticaly(-1); break;
    case ' ': move_cursor_horizontaly(1); break;
    case ':': 
      enter_command_mode();
      break;
    case 'i': 
      enter_inserting_mode();
      break;
  }
}

void editor_key_press() {
  char c;
  while(1) {
    read(STDIN_FILENO, &c, sizeof(c)); 
   
    switch (Buff.mode) {
      case VIEWING_MODE:
        handle_viewing_input(c);
        break;
      case INSERTING_MODE:
        handle_inserting_input(c);
        break;
      case COMMAND_MODE:
        handle_command_input(c);
        break;
    }
  }
}

int main(int arg, char **file) {
  create_window();
  enable_raw_mode();

  if (arg < 2) {
    ansi_emit(ANSI_CLEAR);
    start_menu(Win.height, Win.width); 
    disable_raw_mode();
    exit(0);
  }

  init_editor();
  open_editor(file[1]);
  enable_raw_mode();
  draw_editor();
  editor_key_press();
 
  free_editor();
  disable_raw_mode();
  return 0;
}
