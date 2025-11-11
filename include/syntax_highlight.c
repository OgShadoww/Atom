#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>

#define KEY_TOKEN 1
#define TYPE_TOKEN 2

typedef enum {
  COLOR_DEFAULT,
  COLOR_RESET,
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_MAGENTA,
  COLOR_CYAN,
  COLOR_WHITE
} Colors;

const char *ansi_colors[] = {  
  [COLOR_DEFAULT] = "\033[2J",
  [COLOR_BLUE] = "\033[34m",
  [COLOR_CYAN] = "\033[38;5;81m",
  [COLOR_RESET] = "\033[0m",
};

static const char *c_keywords[] = {
  "if", "else", "while", "for", "do", "switch", "case",
  "return", "break", "continue", "goto", "default",
  "sizeof", "typedef", "struct", "union", "enum",
  NULL
};

static const char *c_types[] = {
  "int", "char", "size_t", "float", "double", "bool", "signed char", "unsigned char",
  NULL
};

int is_keyword(char *token, int size) {
  for(int i = 0; c_keywords[i] != NULL; i++) {
    if(size == strlen(c_keywords[i]) && strncmp(token, c_keywords[i], size) == 0) {
      return KEY_TOKEN;
    }
  }
  for(int i = 0; c_types[i] != NULL; i++) {
    if(size == strlen(c_types[i]) && strncmp(token, c_types[i], size) == 0) {
      return TYPE_TOKEN;
    }
  }

  return -1;
}

void syntax_highlight_and_print(char *line, int size) {
  int pos = 0;

  while(pos < size) {
    while(pos < size && line[pos] == ' ') {
      write(STDOUT_FILENO, " ", 1);
      pos++;
    }

    int token_start = pos;
    while(pos < size && isalnum(line[pos])) pos++;
    int token_len = pos - token_start;

    if(token_len > 0) {
      if(is_keyword(line + token_start, token_len) == KEY_TOKEN) {
        write(STDOUT_FILENO, ansi_colors[COLOR_BLUE], strlen(ansi_colors[COLOR_BLUE])); // blue
        write(STDOUT_FILENO, line + token_start, token_len);
        write(STDOUT_FILENO, ansi_colors[COLOR_RESET], strlen(ansi_colors[COLOR_RESET])); // reset color
      }
      else if(is_keyword(line + token_start, token_len) == TYPE_TOKEN) {
        write(STDOUT_FILENO, ansi_colors[COLOR_CYAN], strlen(ansi_colors[COLOR_CYAN])); // blue
        write(STDOUT_FILENO, line + token_start, token_len);
        write(STDOUT_FILENO, ansi_colors[COLOR_RESET], strlen(ansi_colors[COLOR_RESET])); // reset color
      }
      else {
        write(STDOUT_FILENO, line + token_start, token_len);
      }
    }
    if(pos < size && !isalnum(line[pos]) && line[pos] != ' ') {
      write(STDOUT_FILENO, &line[pos], 1);
      pos++;
    }
  }
}
