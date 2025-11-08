#include<stdio.h>

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

void set_color(Color fg) {

}

void reset_color(void) {

}

char *highlight_line(char *line) {
  char *highlighted_line;
  for(int i = 0; i < strlen(line); i++) {
    if(isdigit(line[i])) {
      
    }
  }
}
