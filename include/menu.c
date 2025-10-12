#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *welcome_lines[10] = {
    "ATOM - Terminal Text Editor",
    "原子",
    "version 0.0.1",
    "by Orest Halenza et al.",
    "Atom is open source and freely distributable.",
    "Type :help for available commands",
    "Type :new to create a new file", 
    "Type :open <filename> to open a file",
    "Type :q to quit",
    "Press : to enter command mode"
};

void print_menu(int win_h, int win_w) {
    write(STDOUT_FILENO, "\033[2J", 4);
    write(STDOUT_FILENO, "\033[1;1H", 7);
    
    int line_count = sizeof(welcome_lines) / sizeof(welcome_lines[0]);  // ← Auto-calculate size
    int max_len = 0;
    
    // Find longest line for perfect centering
    for(int i = 0; i < line_count; i++) {
        int len = strlen(welcome_lines[i]);
        if(len > max_len) max_len = len;
    }
    
    // Calculate starting position (centered vertically)
    int start_y = win_h / 2 - line_count / 2;
    int center_x = (win_w - max_len) / 2;
    
    // Print each line centered
    for(int i = 0; i < line_count; i++) {
        dprintf(STDOUT_FILENO, "\033[%d;%dH", start_y + i, center_x + 1);
        write(STDOUT_FILENO, welcome_lines[i], strlen(welcome_lines[i]));
    }
    
    // Position cursor at bottom for command input
    dprintf(STDOUT_FILENO, "\033[%d;1H", win_h);
}
void start_menu(int win_h, int win_w) {
  print_menu(win_h, win_w);

  return;
}
