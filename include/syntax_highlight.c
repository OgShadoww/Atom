#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>

typedef enum {
    COLOR_DEFAULT = 0,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
} Colors;

static const char *c_keywords[] = {
  "if", "else", "while", "for", "do", "switch", "case",
  "return", "break", "continue", "goto", "default",
  "sizeof", "typedef", "struct", "union", "enum",
  NULL
};

int is_keyword(char *token, int size) {
  for(int i = 0; c_keywords[i] != NULL; i++) {
    if(size == strlen(c_keywords[i]) && strncmp(token, c_keywords[i], size) == 0) {
      return 1;
    }
  }

  return 0;
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
      if(is_keyword(line + token_start, token_len)) {
        write(STDOUT_FILENO, "\033[34m", 5);  // Blue
        write(STDOUT_FILENO, line + token_start, token_len);
        write(STDOUT_FILENO, "\033[0m", 4);   // Reset
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
