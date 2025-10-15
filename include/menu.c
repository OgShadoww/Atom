#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *welcome_lines[11] = {
  "\033[38;2;255;120;70m原子\033[0m\n",
  "Atom Terminal Text Editor\n",
  "\n",
  "version 0.0.1\n",
  "by \033[38;2;255;215;100mOrest Halenza\033[0m et al.\n",
  "Atom is open source and freely distributable.\n",
  "\n",
  "Type :help \033[38;2;100;200;255m<Enter>\033[0m     for available commands\n",
  "Type :new  \033[38;2;100;200;255m<Enter>\033[0m       to create a new file\n", 
  "Type :open \033[38;2;100;200;255m<Enter>\033[0m             to open a file\n",
  "Type :q    \033[38;2;100;200;255m<Enter>\033[0m                    to quit\n",
};

int get_visible_length(const char *str) {
  int len = 0;
  int in_ansi = 0;
  
  for (int i = 0; str[i]; i++) {
    if (str[i] == '\033' && str[i+1] == '[') {
      in_ansi = 1;
    } else if (in_ansi && str[i] == 'm') {
      in_ansi = 0;
    } else if (!in_ansi) {
      len++;
    }
  }
  return len;
}

void print_menu(int win_h, int win_w) {
  write(STDOUT_FILENO, "\033[2J", 4);
  write(STDOUT_FILENO, "\033[1;1H", 7);
    
  int line_count = 11;
  int start_y = win_h / 2 - line_count / 2;
    
  for(int i = 0; i < line_count; i++) {
    int center_x = (win_w - get_visible_length(welcome_lines[i])) / 2;
    dprintf(STDOUT_FILENO, "\033[%d;%dH", start_y + i, center_x + 1);
    write(STDOUT_FILENO, welcome_lines[i], strlen(welcome_lines[i]));
  }
    
  dprintf(STDOUT_FILENO, "\033[%d;1H", win_h);
}

void start_menu(int win_h, int win_w) {
  print_menu(win_h, win_w);

  return;
}
