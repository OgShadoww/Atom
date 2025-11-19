#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#define PATH_LEN 128

// ===============================
// DATA STRUCTURES
// ===============================

#define KEY_ENTER 10

typedef enum {
  ENTRY_FILE,
  ENTRY_DIR,
  ENTRY_PARENT,
  ENTRY_UNKNOWN
} EntryType;

typedef struct {
  char *name;
  char *full_path;
  EntryType type;
  off_t size;
} FileEntry;

typedef struct {
  FileEntry *entries;
  int count;
  int selected;
  char current_path[PATH_LEN];
} FileBrowser;

typedef struct {
  int width;
  int height;
  int scroll_y;
} Window;

extern Window Win;

void end_browsing();
void cmd_quit(void);
int clamp(int v, int lo, int hi);
void start_buffer(char *filepath);

// ===============================
// GLOBAL 
// ===============================

FileBrowser Browser = {0};

// ===============================
// File Browser
// ===============================

FileEntry *load_all_entries(char *path, int *total_count) {
  DIR *dir = opendir(path);
  if(dir == NULL) {
    perror("opendir() error");
    return NULL;
  }

  // Dynamic array
  int capacity = 32;
  int count = 0;
  FileEntry *entries = calloc(capacity, sizeof(FileEntry));
  if(!entries) {
    closedir(dir);
    return NULL;
  }

  struct dirent *entry;

  while((entry = readdir(dir)) != NULL) {
    if(count >= capacity) {
      capacity *= 2; 
      FileEntry *tmp = realloc(entries, sizeof(FileEntry) * capacity);
      if(!tmp) {
        for(int i = 0; i < count; i++) {
          free(entries[i].name);
          free(entries[i].full_path);
        }
        free(entries);
        closedir(dir);
        return NULL;
      }
      entries = tmp;
      memset(&entries[count], 0, (capacity - count) * sizeof(FileEntry));
    }

    char full_path[PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    struct stat st;
    if(stat(full_path, &st) == 0) {
      entries[count].type = S_ISDIR(st.st_mode) ? ENTRY_DIR : ENTRY_FILE;
      entries[count].size = st.st_size;
    }
    entries[count].full_path = strdup(full_path);

    entries[count].name = strdup(entry->d_name);

    count++;
  }

  closedir(dir);
  *total_count = count;

  return entries; 
}

void select_entry(int direction) {
  int max_size = Browser.count > Win.height ? Browser.count - 3 : Browser.count - 1;
  Browser.selected = clamp(Browser.selected + direction, 0, max_size);
  if(Browser.selected - Win.scroll_y >= Win.height - 2 && Win.scroll_y <= Browser.count - Win.height - 1) Win.scroll_y++;
  if(Browser.selected < Win.scroll_y) Win.scroll_y--;
}

void init_file_browser(char *current_dir) {
  char path[128];
  if(current_dir != NULL) {
    strcpy(Browser.current_path, current_dir);
  }
  else {
    if(getcwd(path, sizeof(path)) == NULL) {
      perror("getcwd() error");
      return;
    }
    strcpy(Browser.current_path, path);
  }

  Browser.entries = load_all_entries(Browser.current_path, &Browser.count);
  Browser.selected = 0;
}

void free_file_browser() {
  for(int i = 0; i < Browser.count; i++) {
    free(Browser.entries[i].name);
    free(Browser.entries[i].full_path);
  }
  free(Browser.entries);
}

void draw_browser() {
  dprintf(STDOUT_FILENO, "\033[2J");
  dprintf(STDOUT_FILENO, "\033[%d;1H\033[2K", 1);

  int end_point = Browser.count <= Win.height ? Browser.count : Win.height + Win.scroll_y - 2;
  if(strlen(Browser.current_path) > Win.width) {
    char *temp_name = malloc(sizeof(char)*(Win.width));
    if(!temp_name) {
      perror("Malloc failled");
      exit(0);
    }
    size_t start_pos = strlen(Browser.current_path) - Win.width + 1;
    memcpy(temp_name, Browser.current_path + start_pos, Win.width);
    dprintf(STDOUT_FILENO, "\033[1;34m%s\033[0m\n", temp_name); 
    free(temp_name);
  }
  else {
    dprintf(STDOUT_FILENO, "\033[1;34m%s\033[0m\n", Browser.current_path); 
  }

  for(int i = Win.scroll_y; i < end_point; i++) {
    if(i == Browser.selected) {
      write(STDOUT_FILENO, "\033[4m", 4);
      
      if(Browser.entries[i].type == ENTRY_DIR) {
        dprintf(STDOUT_FILENO, "%s/", Browser.entries[i].name);
      } 
      else {
        dprintf(STDOUT_FILENO, "%s", Browser.entries[i].name);
      }
      
      int name_len = strlen(Browser.entries[i].name) + 2;
      if(Browser.entries[i].type == ENTRY_DIR) name_len++;
      
      for(int j = name_len; j < Win.width; j++) {
        write(STDOUT_FILENO, " ", 1);
      }
      
      write(STDOUT_FILENO, "\033[24m", 5);
      write(STDOUT_FILENO, "\n", 1);
    }
    else {
      if(Browser.entries[i].type == ENTRY_DIR) {
        dprintf(STDOUT_FILENO, "%s/\n", Browser.entries[i].name);
      } 
      else {
        dprintf(STDOUT_FILENO, "%s\n", Browser.entries[i].name);
      }
    }
  }

  int cursor_y = Browser.selected - Win.scroll_y + 2;
  dprintf(STDOUT_FILENO, "\033[%d;%dH", cursor_y, 1);
}

void open_entry(FileEntry entry) {
  if(entry.type == ENTRY_DIR) {
    char new_path[PATH_LEN];
    strcpy(new_path, entry.full_path);
    
    free_file_browser();
    init_file_browser(new_path);
    draw_browser();  
  }
  else if(entry.type == ENTRY_FILE) {
    char filepath[PATH_LEN];
    strcpy(filepath, entry.full_path);

    free_file_browser();
    start_buffer(filepath);
  }
}

void handle_browser_input(char c) {
  switch (c) {
    case 'q': end_browsing(); break;
    case KEY_ENTER: open_entry(Browser.entries[Browser.selected]); break;
    case 'j': select_entry(1); draw_browser(); break;
    case 'k': select_entry(-1); draw_browser(); break;
  } 
}

void start_browsing(int width, int height) {
  Win.width = width;
  Win.height = height;
  Win.scroll_y = 0;
  init_file_browser(NULL);
  draw_browser();
}

void end_browsing() {
  free_file_browser(); 
  cmd_quit();
}
