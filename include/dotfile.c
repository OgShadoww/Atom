#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOTFILE_PATH "~/.atomrc"
#define SETTING_COUNT 4

enum KEYS { 
  TAB_VAL
};

char *values[] = {
  [TAB_VAL] = "2",
};

char *default_layout[] = {
  "tab_value = 2",
  "escape_key_1 = j",
  "escape_key_2 = j",
  "escape_timeout_ms = 300"
};

int check_dotfile() {
  FILE *dotfile = fopen(DOTFILE_PATH, "r");

  if(dotfile) {
    fclose(dotfile);
    return 1;
  }
  
  return 0;
}

void handle_dotfile() {
  if(check_dotfile() == 1) {
    FILE *dotfile = fopen(DOTFILE_PATH, "r");

    char buff[128];

    while(fgets(buff, 128, dotfile)) {
      size_t size = strlen(buff);
      int pos = 0;
      
      while(pos < size) {
        while(pos < size && buff[pos] == ' ') pos++;

        int key_start = pos;
        int key_length = 0;

        while(pos < size && buff[pos] != '=') {
          pos++;
        }
      }
    }

    fclose(dotfile);
  }
  else {
    FILE *dotfile = fopen(DOTFILE_PATH, "w");
    if(!dotfile) {
      perror("Error opening file");
      exit(EXIT_FAILURE);
    }
  
    for(int i = 0; i < SETTING_COUNT; i++) {
      fputs(default_layout[i], dotfile);
      fputc('\n', dotfile);
    }
    fclose(dotfile);
  }
}
