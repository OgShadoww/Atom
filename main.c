#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

struct termios original_termios;

#define MAX_LINES 1000

typedef struct Line {
  int size;
  char *line;
} Line;

Line Lines[MAX_LINES];
int Size = 0;

Line *open_file(char *filen) {
  FILE *file = fopen(filen, "r");

  if (file == NULL) {
    perror("Error opening file");
    return 0;
  }

  char buffer[128];
  while (fgets(buffer, sizeof(buffer), file)) {
    Lines[Size].size = strlen(buffer);
    Lines[Size].line = malloc(Lines[Size].size + 1);
    strcpy(Lines[Size].line, buffer);
    Size++;
  }

  fclose(file);
  return Lines;
}

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(disableRawMode);

  struct termios raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void inti_editor() {

}

int main(int arg, char **file) {
  open_file(file[1]);
  for (int i = 0; i < Size; i++) {
    write(STDOUT_FILENO, Lines[i].line, Lines[i].size);
  }

  enableRawMode();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (c == 127) {
      write(STDOUT_FILENO, "\b \b", 3);
    } else {
      write(STDOUT_FILENO, &c, 1);
    }
  }

  return 0;
}
