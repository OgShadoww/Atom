#include<stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<string.h>

typedef struct Line {
  int size;
  char *line;
} Line;

#define MAX_LINES 1000

Line Lines[MAX_LINES];
int Size = 0;

Line *open_file(char *filen) {
  FILE *file = fopen(filen, "r");

  if(file == NULL) {
    perror("Error openning file");

    return 0;
  }
  char buffer[128];
  while(fgets(buffer, sizeof(buffer), file)) {
    Lines[Size].size = strlen(buffer);
    Lines[Size].line = malloc(Lines[Size].size + 1);
    strcpy(Lines[Size].line, buffer);
    Size++;
  }

  fclose(file);
  return Lines;
}

void inti_editor() {
  
}

int main(int arg, char **file) {
  open_file(file[1]);
  for(int i = 0; i < Size; i++) {
    write(STDOUT_FILENO, Lines[i].line, Lines[i].size);
  }

  return 0;
}
