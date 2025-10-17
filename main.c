#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <wctype.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>

// ===============================
// CONSTANTS AND KEY DEFINITIONS
// ===============================

#define INSERTING_MODE 0
#define VIEWING_MODE 1
#define COMMAND_MODE 2

#define KEY_ESC 27
#define KEY_ENTER 10
#define KEY_BACKSPACE 127
#define KEY_TAB 9
#define TAB_VAL 2
#define ESCAPE_KEY_1 'j'
#define ESCAPE_KEY_2 'j'
#define ESCAPE_TIMEOUT_MS 300
 
// ===============================
// ANSI ESCAPE CODES
// ===============================

enum AnsiCode {
  ANSI_CLEAR,
  ANSI_CLEAR_SCROLL,
  ANSI_CURSOR_HIDE,
  ANSI_CURSOR_HOME,
  ANSI_CURSOR_SHOW,
  ANSI_ERASE_CHARACTER,
  ANSI_CURSOR_BLOCK,
  ANSI_CURSOR_BAR,
  ANSI_CUROSR_UNDERLINE,
};

const char *ansi_codes[] = {  
  [ANSI_CLEAR] = "\033[2J",
  [ANSI_CLEAR_SCROLL] = "\033[3J",
  [ANSI_CURSOR_HIDE] = "\033[?25l",
  [ANSI_CURSOR_HOME] = "\033[1;1H", 
  [ANSI_CURSOR_SHOW] = "\033[?25h",
  [ANSI_ERASE_CHARACTER] = "\b \b",
  [ANSI_CURSOR_BLOCK] = "\033[2 q",
  [ANSI_CURSOR_BAR] = "\033[6 q",
  [ANSI_CUROSR_UNDERLINE] = "\033[4 q"
};

// ===============================
// DATA STRUCTURES
// ===============================

typedef struct {
  int size;
  char *line;
} Line;

typedef struct {
  int x;
  int y;
  int desired_x;
} Cursor;

typedef struct {
  int width;
  int height;
  int scroll_y;
} Window;

typedef struct {
  int mode;
  int count_prefix;
  Line *document;
  int document_size;
  int document_capacity;
  Cursor cursor;
  char *file_name;
  char pending_escape_char;
  int has_pending_escape;
  char status_msg[128];
  int status_len;
} Buffer;

// ===============================
// GLOBAL VARIABLES
// ===============================

struct termios OriginalTermios;
Buffer Buff;
Window Win;

// ===============================
// FUNCTION PROTOTYPES
// ===============================

void ansi_emit(enum AnsiCode code);
int clamp(int v, int lo, int hi);
int wait_for_input_with_timeout(int timeout_ms);
void disable_raw_mode(void);
void enable_raw_mode(void);
void ensure_document_capacity(void);
void init_editor(void);
void free_editor(void);
void open_editor(char *filen);
void create_window(void);
void draw_editor(void);

void append_char(char c);
void delete_char(void);
void append_line(void);

void move_cursor_horizontaly(int direction);
void move_cursor_verticaly(int direction);

void enter_viewing_mode(void);
void handle_viewing_input(char c);

void enter_inserting_mode(void);
void handle_inserting_input(char c);
void exit_inserting_mode(void);

void enter_command_mode(void);
void handle_command_input(char c);
void exit_command_mode(void);
void process_command_input(char *command);

void cmd_save_file(void);
void cmd_quit(void);

void editor_key_press(void);

// External
void start_menu(int win_h, int win_w);

// ----------
// HELPERS
// ----------

void ansi_emit(enum AnsiCode code) {
  write(STDOUT_FILENO, ansi_codes[code], strlen(ansi_codes[code]));
}

int clamp(int v, int lo, int hi) {
  if(hi < lo) return lo;
  if(v > hi) return hi;
  if(v < lo) return lo;
  return v;
}

// Functions to work with terminal text modes
void disable_raw_mode() {
  ansi_emit(ANSI_CURSOR_SHOW);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &OriginalTermios);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &OriginalTermios);
  atexit(disable_raw_mode);
  struct termios raw = OriginalTermios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int wait_for_input_with_timeout(int timeout_ms) {
  fd_set readfds;
  struct timeval timeout;

  // Clear the set
  FD_ZERO(&readfds);

  // Add stdin (STDIN_FILENO) to the set
  FD_SET(STDIN_FILENO, &readfds);

  // Set timeout
  timeout.tv_sec = timeout_ms / 1000;           // seconds
  timeout.tv_usec = (timeout_ms % 1000) * 1000; // microseconds

  int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

  return result;
}

void set_command_status(const char* s) {
  if (!s) { Buff.status_msg[0] = '\0'; Buff.status_len = 0; return; }
  size_t n = strlen(s);
  if (n >= sizeof(Buff.status_msg)) n = sizeof(Buff.status_msg) - 1;
  memcpy(Buff.status_msg, s, n);
  Buff.status_msg[n] = '\0';
  Buff.status_len = (int)n;
}

void clear_command_status(void) {
  Buff.status_msg[0] = '\0';
  Buff.status_len = 0;
}

// ===============================
// BUFFER MANAGEMENT
// ===============================

void init_editor() {
  Buff.cursor.x = 0;
  Buff.cursor.y = 0;
  Buff.cursor.desired_x = 0;
  Buff.mode = VIEWING_MODE;
  Buff.count_prefix = 0;
  Buff.document_capacity = 512;
  Buff.document_size = 0;
  Buff.file_name = NULL;
  Buff.document = malloc(sizeof(Line) * Buff.document_capacity);
  Buff.pending_escape_char = 0;
  Buff.has_pending_escape = 0;
  Buff.status_msg[0] = '\0';
  Buff.status_len = 0;
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

void open_editor(char *filen) {
  struct stat st;
  if (stat(filen, &st) != 0) {
    dprintf(STDERR_FILENO, "\033[1;31mError:\033[0m File '%s' not found.\n", filen);
    disable_raw_mode();
    exit(1);
  }

  if (S_ISDIR(st.st_mode)) {
    dprintf(STDERR_FILENO, "\033[1;31mError:\033[0m '%s' is a directory.\n", filen);
    disable_raw_mode();
    exit(1);
  }

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

// ===============================
// WINDOW AND RENDERING
// ===============================

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
  int max_lines = Win.height - 2;
  int start_line = Win.scroll_y;
  int end_line = Win.scroll_y + max_lines;

  // Render visible lines
  for(int i = start_line; i < end_line && i < Buff.document_size; i++) {
    write(STDOUT_FILENO, Buff.document[i].line, Buff.document[i].size);
  }
  
  // Writing status bar
  dprintf(STDOUT_FILENO, "\033[%d;1H", Win.height - 1);
  write(STDOUT_FILENO, "\033[2K", 4);
  if(Buff.count_prefix > 0) {
    int padding = Win.width - strlen(Buff.file_name) - 20;
    dprintf(STDOUT_FILENO,"%s\t%d,%d%*s%d", Buff.file_name, Buff.cursor.y + 1, Buff.cursor.x + 1, padding, "", Buff.count_prefix);
    //Buff.count_prefix = 0;
  }
  else {
    dprintf(STDOUT_FILENO,"%s\t%d,%d", Buff.file_name, Buff.cursor.y + 1, Buff.cursor.x + 1);
  }

  // Writing command message
  if(Buff.status_len > 0) {
    dprintf(STDOUT_FILENO, "\033[%d;1H", Win.height);
    dprintf(STDOUT_FILENO, "%.*s", Buff.status_len, Buff.status_msg);
  }
  
  // Showing cursor
  int screen_y = Buff.cursor.y - Win.scroll_y + 1;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", screen_y, Buff.cursor.x + 1);
  write(STDOUT_FILENO, "\033[?7h", 5);
  ansi_emit(ANSI_CURSOR_SHOW);
}

// ===============================
// TEXT EDITING OPERATIONS
// ===============================

// Inserting and deletign functions
void append_char(char c) {
  if(Buff.document_size == 0) {
    ensure_document_capacity();
    Buff.document[0].size = 0;
    Buff.document[0].line = malloc(1);
    if (!Buff.document[0].line) perror("malloc"); exit(1);
    Buff.document[0].line[0] = '\0';
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
    Buff.document[0].size = 1;
    Buff.document[0].line = malloc(1);
    if (!Buff.document[0].line) perror("malloc"); exit(1);
    Buff.document[0].line[1] = '\0';
    Buff.document_size = 1;
    Buff.cursor.x = 0;
    Buff.cursor.y = 0;
    draw_editor();
    return;
  }

  int current_line = Buff.cursor.y;
  int split_pos = Buff.cursor.x;
  int current_size = Buff.document[current_line].size;

  int content_size = current_size;

  ensure_document_capacity();

  for (int i = Buff.document_size; i > current_line + 1; i--) {
    Buff.document[i] = Buff.document[i - 1];
  }

  int remaining_size = content_size - split_pos;
  
  Buff.document[current_line + 1].size = remaining_size;
  Buff.document[current_line + 1].line = malloc(remaining_size + 1);

  if (remaining_size > 0) {
    memcpy(Buff.document[current_line + 1].line,
           &Buff.document[current_line].line[split_pos],
           remaining_size);
  }
  Buff.document[current_line + 1].line[remaining_size] = '\0';

  Buff.document[current_line].size = split_pos + 1;
  Buff.document[current_line].line = realloc(Buff.document[current_line].line, split_pos + 2);
  Buff.document[current_line].line[split_pos] = '\n';
  Buff.document[current_line].line[split_pos + 1] = '\0';

  Buff.document_size++;
  move_cursor_verticaly(1);
  Buff.cursor.x = 0;
  Buff.cursor.desired_x = 0;

  draw_editor();
}

void delete_char() {
  int original_size = Buff.document[Buff.cursor.y].size;
  int delete_pos = Buff.cursor.x - 1;
   if(delete_pos < 0) {
    if(Buff.cursor.y > 0) {
      if(Buff.cursor.y > 0) {
        int current_pos = Buff.cursor.y;
        int current_size = Buff.document[current_pos].size;
        int previous_size = Buff.document[current_pos - 1].size;

        int previous_content_size = previous_size;
        if(previous_size > 0 && Buff.document[current_pos - 1].line[previous_size - 1] == '\n') {
          previous_content_size = previous_size - 1;
        }
  
        Buff.document[current_pos - 1].size = previous_content_size + current_size;
  
        Buff.document[current_pos - 1].line = realloc(
          Buff.document[current_pos - 1].line,
          previous_content_size + current_size + 1
        );
  
        memcpy(
          &Buff.document[current_pos - 1].line[previous_content_size],
          Buff.document[current_pos].line,
          current_size
        );
  
        Buff.document[current_pos - 1].line[previous_content_size + current_size] = '\0';
  
        free(Buff.document[current_pos].line);
  
        for(int i = current_pos; i < Buff.document_size - 1; i++) {
          Buff.document[i] = Buff.document[i + 1];
        }
  
        Buff.document_size--;
        move_cursor_verticaly(-1);
        Buff.cursor.x = previous_content_size;
        Buff.cursor.desired_x = previous_content_size;
      }    
    }
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

void delete_line() {
  if(Buff.document_size == 0) {
    draw_editor();
    return;
  }
  
  int current_line = Buff.cursor.y; 

  free(Buff.document[current_line].line);
  Buff.document[current_line].size = 0;
  Buff.document[current_line].line = NULL;

  for(int i = current_line; i < Buff.document_size; i++) {
    Buff.document[i] = Buff.document[i+1];
  }

  Buff.document_size--;
  if(Buff.cursor.y >= Buff.document_size) {
    move_cursor_verticaly(-1);
  }
  if(Buff.cursor.y <= 1) {
    move_cursor_horizontaly(-Buff.document_size);
  }
  draw_editor();
}

// ===============================
// CURSOR MOVEMENT
// ===============================

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

  int max_visible_lines = Win.height - 2;
  
  if (Buff.cursor.y >= Win.scroll_y + max_visible_lines) {
    Win.scroll_y = Buff.cursor.y - max_visible_lines + 1;
  }
  
  if (Buff.cursor.y < Win.scroll_y) {
    Win.scroll_y = Buff.cursor.y;
  }
  draw_editor();
}

// ===============================
// MODE HANDLERS
// ===============================

// --- VIEWING MODE ---

void enter_viewing_mode() { 
  move_cursor_horizontaly(-1);
  Buff.mode = VIEWING_MODE;
  Buff.count_prefix = 0;
  ansi_emit(ANSI_CURSOR_BLOCK);
  clear_command_status();
}

void handle_viewing_input(char c) {
  int movement = (Buff.count_prefix > 0) ? Buff.count_prefix : 1;
    
  if (c > '0' && c <= '9') {
    Buff.count_prefix = Buff.count_prefix * 10 + (c - '0');
    Buff.count_prefix = clamp(Buff.count_prefix, 0, 100000);
    draw_editor();
    return;
  }

  switch (c) {
    case 'h': move_cursor_horizontaly(-movement); Buff.count_prefix = 0; break;
    case 'l': move_cursor_horizontaly(movement); break;
    case 'j': move_cursor_verticaly(movement); break;
    case 'k': move_cursor_verticaly(-movement); break;
    case ' ': move_cursor_horizontaly(1); break;
    case ':': enter_command_mode(); break;
    case 'i': enter_inserting_mode(); break;
    case 'a': move_cursor_horizontaly(1); enter_inserting_mode(); break;
    case 'I':
      for(int i = 0; i < Buff.document[Buff.cursor.y].size; i++) {
        if(Buff.document[Buff.cursor.y].line[i] != ' ') {
          move_cursor_horizontaly(i - Buff.cursor.x);
          enter_inserting_mode();
          break;
        }
      }
      move_cursor_horizontaly(0);
      break;
    case 'A':
      move_cursor_horizontaly(Buff.document[Buff.cursor.y].size);
      enter_inserting_mode();
      break;
    case '0':
      move_cursor_horizontaly(-Buff.document[Buff.cursor.y].size);
      break;
    case 'G':
      move_cursor_verticaly(-Buff.document_size);
      break;
    case 'd':
      delete_line();
      break;
  }
  
  Buff.count_prefix = 0;
}

// --- INSERTING MODE ---

void enter_inserting_mode() {
  Buff.mode = INSERTING_MODE;
  ansi_emit(ANSI_CURSOR_BAR);
  clear_command_status();
  set_command_status("-- INSERT --");
}

void handle_inserting_input(char c) {
  if(Buff.has_pending_escape) {
    if(c == ESCAPE_KEY_2) {
      exit_inserting_mode();
    }
    else {
      if (c == KEY_BACKSPACE) {
        delete_char();
      } 
      else if (c == KEY_ENTER) {
        append_line();
      } 
      else if (c >= 32 && c <= 126) {
        append_char(c);
      } 
    }

    Buff.has_pending_escape = 0; 
    Buff.pending_escape_char = 0;
    draw_editor();
    return;
  }

  if(c == ESCAPE_KEY_1) {
    int result = wait_for_input_with_timeout(ESCAPE_TIMEOUT_MS);

    if(result > 0) {
      Buff.pending_escape_char = c;
      Buff.has_pending_escape = 1;
      return;
    }
    else if(result == 0) {
      append_char(c); 
      draw_editor();
      Buff.has_pending_escape = 0;
      return;
    }
    else {
      append_char(c);
      draw_editor();
      Buff.has_pending_escape = 0;
      return;
    }
  }

  switch (c) {
    case KEY_ESC: 
      exit_inserting_mode(); 
      break;
    case KEY_BACKSPACE:
      delete_char();
      draw_editor();
      break;
    case KEY_ENTER:
      append_line();
      draw_editor();
      break;
    case KEY_TAB:
      for(int i = 0; i < TAB_VAL; i++) append_char(' ');
      draw_editor();
      break;
    default:
      if (c >= 32 && c <= 126) {
        append_char(c);
        draw_editor();
      }
      break;
  }
}

void exit_inserting_mode() {
  enter_viewing_mode();
}

// --- COMMAND MODE ---

void enter_command_mode() {
  Buff.mode = COMMAND_MODE;
  ansi_emit(ANSI_CUROSR_UNDERLINE);
  dprintf(STDOUT_FILENO, "\033[%d;1H", Win.height);
  write(STDOUT_FILENO, "\033[2K", 4);
  write(STDOUT_FILENO, ":", 1);
}

void handle_command_input(char c) {
  static char command_buffer[128];
  static int cmd_pos = 0;

  switch (c) {
    case KEY_ENTER:
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

void process_command_input(char *command) {
  if (strcmp(command, "q") == 0) {
    cmd_quit();
  } 
  else if (strcmp(command, "w") == 0) {
    cmd_save_file();
    exit_command_mode();
  } 
  else if (strcmp(command, "wq") == 0) {
    cmd_save_file();
    cmd_quit();
  } 
  else {
    set_command_status("\033[1;31mError:\033[0m Command not found");
    exit_command_mode();
  }
}

void exit_command_mode() {
  ansi_emit(ANSI_ERASE_CHARACTER);
  int screen_y = Buff.cursor.y - Win.scroll_y + 1;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", screen_y, Buff.cursor.x + 1);
  draw_editor();
  enter_viewing_mode();
}

// ===============================
// COMMAND IMPLEMENTATIONS
// ===============================
//
void cmd_save_file(void) {
  if (!Buff.file_name) {
    exit_command_mode();
    return;
  }
  
  FILE *file = fopen(Buff.file_name, "w");
  if (file == NULL) {
    perror("Error saving file");
    exit_command_mode();
    return;
  }
  
  for (int i = 0; i < Buff.document_size; i++) {
    fputs(Buff.document[i].line, file);
  }
  
  fclose(file);
  exit_command_mode();
  set_command_status("Saved");
}

void cmd_quit(void) {
  ansi_emit(ANSI_CURSOR_SHOW);
  free_editor();
  disable_raw_mode();
  ansi_emit(ANSI_CLEAR);
  ansi_emit(ANSI_CURSOR_HOME);
  exit(0);
}

// ===============================
// MAIN EVENT LOOP
// ===============================

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
