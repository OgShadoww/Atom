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

enum AnsiCode;

// ===============================
// GLOBAL 
// ===============================

FileBrowser Browser = {0};
void ansi_emit(enum AnsiCode code);


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
  FileEntry *entries = malloc(sizeof(FileEntry) * capacity);

  struct dirent *entry;

  while((entry = readdir(dir)) != NULL) {
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    if(count >= capacity) {
      capacity *= 2; 
      entries = realloc(entries, sizeof(FileEntry) * capacity);
    }

    entries[count].name = strdup(entry->d_name);

    count++;
  }

  closedir(dir);
  dir = NULL;
  *total_count = count;

  return entries; 
}

void init_file_browser() {
  char path[128];
  if(getcwd(path, sizeof(path)) == NULL) perror("getcwd() error");
  strcpy(Browser.current_path, path);
  Browser.entries = load_all_entries(Browser.current_path, &Browser.count);
  Browser.selected = 0;
}

void free_file_browser() {
 free(Browser.entries); 
}

void draw_browser() {
  char line[56] = "========================================================";
  //ansi_emit(ANSI_CLEAR);
}

void handle_browsing() {

}

void start_browsing() {
  init_file_browser();
  for(int i = 0; i < Browser.count; i++) {
    write(STDOUT_FILENO, Browser.entries[i].name, strlen(Browser.entries[i].name));
    write(STDOUT_FILENO, "\n", 1);
  }

  free_file_browser(); 
  return;
}
