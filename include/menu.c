#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int WIDTH, HEIGHT;

void cmd_quit();
void start_buffer(char *filepath);

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

void print_menu() {
  write(STDOUT_FILENO, "\033[2J", 4);
  write(STDOUT_FILENO, "\033[1;1H", 7);
    
  int line_count = 11;
  int start_y = HEIGHT / 2 - line_count / 2;
    
  for(int i = 0; i < line_count; i++) {
    int center_x = (WIDTH - get_visible_length(welcome_lines[i])) / 2;
    dprintf(STDOUT_FILENO, "\033[%d;%dH", start_y + i, center_x + 1);
    write(STDOUT_FILENO, welcome_lines[i], strlen(welcome_lines[i]));
  }
    
  dprintf(STDOUT_FILENO, "\033[%d;1H", 1);
}

int check_file_exist(const char *path) {
  struct stat st;

  if(stat(path, &st) == -1)
    return 0;
  else if(S_ISDIR(st.st_mode))
    return -1;

  return 1;
}

void process_menu_command_mode(char *buffer) {
  if(strncmp(buffer, "help", 4) == 0) {
    start_buffer("help.txt");
  }
  else if(strncmp(buffer, "open", 4) == 0) {
    if(check_file_exist(buffer+5) == 1) {
      start_buffer(buffer+5);
    }
    else if(check_file_exist(buffer+5) == -1) {
      dprintf(STDERR_FILENO, "\033[1;31mError:\033[0m '%s' is a directory.\n", buffer+5);
      return;
    }
    else {
      dprintf(STDERR_FILENO, "\033[1;31mError:\033[0m '%s' is not exist.\n", buffer+5);
    }
  }
  else if(strncmp(buffer, "new ", 4) == 0) {
    char *name = buffer+4;
    start_buffer(name);
  }
  else if(strcmp(buffer, "q") == 0) {
    cmd_quit();
  }
  else {
    dprintf(STDERR_FILENO, "\033[1;31mError:\033[0m '%s' no command found.\n", buffer);
    return;
  }
}

void handle_menu_command_mode() {
  char c;
  char buffer[256];
  int i = 0;

  while(1) {
    read(STDIN_FILENO, &c, sizeof(c)); 

    if(!c) {
      perror("Error reading character");
      exit(EXIT_FAILURE);
    }

    switch (c) {
      case 10:  // Enter key
        buffer[i+1] = '\0';
        process_menu_command_mode(buffer);
        break;
      case 127: // Backspace
        write(STDOUT_FILENO, "\b \b", 4);
        i--;
        break;
      default:
        buffer[i] = c;
        i++;
        write(STDOUT_FILENO, &c, sizeof(c));
        break;
    }
  }
}

void enter_menu_command_mode() {
  dprintf(STDOUT_FILENO, "\033[%d;1H", HEIGHT);
  write(STDOUT_FILENO, "\033[2K", 4);
  write(STDOUT_FILENO, ":", 1);
  handle_menu_command_mode();
}

void handle_menu_input(char c) {
  switch(c) {
    case ':':
      enter_menu_command_mode();
      break;
    case 'q': cmd_quit(); break; 
  }
}

void start_menu(int win_h, int win_w) {
  WIDTH = win_w;
  HEIGHT = win_h;
  print_menu();

  return;
}
