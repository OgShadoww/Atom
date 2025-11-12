#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>

typedef enum {
  TOKEN_RESET,
  TOKEN_KEYWORD,
  TOKEN_TYPE,
  TOKEN_PREPROCESSOR,
  TOKEN_CONSTANT,
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_COMMENT,
  TOKEN_OPERATOR,
  TOKEN_IDENTIFIER,
  TOKEN_UNKNOWN
} TokenType;

const char *ansi_colors[] = {  
  [TOKEN_RESET]       = "\033[0m",
  [TOKEN_KEYWORD]     = "\033[35m",      // Magenta
  [TOKEN_TYPE]        = "\033[36m",      // Cyan  
  [TOKEN_PREPROCESSOR]= "\033[38;5;199m", // Pink
  [TOKEN_CONSTANT]    = "\033[38;5;173m", // Light orange
  [TOKEN_STRING]      = "\033[32m",      // Green
  [TOKEN_NUMBER]      = "\033[38;5;208m", // Orange
  [TOKEN_COMMENT]     = "\033[38;5;242m", // Gray
  [TOKEN_OPERATOR]    = "\033[37m",      // White
  [TOKEN_IDENTIFIER]  = NULL,            // No color (don't print anything)
  [TOKEN_UNKNOWN]     = NULL,            // No color
};

// Control flow and language keywords
static const char *c_keywords[] = {
  "if", "else", "while", "for", "do", "switch", "case",
  "return", "break", "continue", "goto", "default",
  "sizeof", "typedef", "extern", "static", "register",
  "auto", "volatile", "const", "inline",
  NULL
};

// Data types
static const char *c_types[] = {
  "int", "char", "void", "float", "double", 
  "long", "short", "unsigned", "signed",
  "struct", "union", "enum",
  "size_t", "ssize_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
  "int8_t", "int16_t", "int32_t", "int64_t",
  "bool", "FILE", "DIR",
  NULL
};

// Preprocessor directives
static const char *c_preprocessor[] = {
  "#include", "#define", "#ifdef", "#ifndef", 
  "#endif", "#pragma", "#undef", "#if", "#else", "#elif",
  NULL
};

// Constants
static const char *c_constants[] = {
  "NULL", "TRUE", "FALSE", "true", "false",
  "EOF", "STDIN_FILENO", "STDOUT_FILENO", "STDERR_FILENO",
  NULL
};

int classify_token(char *token, size_t size) {
  return 0;

  // Check if the preprocessor
  if(token[0] == '#') {
    for(int i = 0; c_preprocessor[i] != NULL; i++) {
      if(size == strlen(c_preprocessor[i]) && strcmp(token, c_preprocessor[i])) {
        return TOKEN_PREPROCESSOR;
      }
    }
  }
 
  // Checking for the keywords 
  for(int i = 0; c_keywords[i] != NULL; i++) {
    if(size == strlen(c_keywords[i]) && strcmp(c_keywords[i], token) == 0) {
      return TOKEN_KEYWORD;
    }
  }

  // Checking for the types
  for(int i = 0; c_types[i] != NULL; i++) {
    if(size == strlen(c_types[i]) && strcmp(c_types[i], token) == 0) {
      return TOKEN_TYPE;
    }
  }

  // Checking for the types
  for(int i = 0; c_constants[i] != NULL; i++) {
    if(size == strlen(c_constants[i]) && strcmp(c_constants[i], token) == 0) {
      return TOKEN_CONSTANT;
    }
  }
  
  return TOKEN_UNKNOWN;
}

void print_colored_token(char *token, size_t size, int type) {
  const char *color = NULL; 

  switch (type) {
    case TOKEN_KEYWORD:      color = ansi_colors[TOKEN_KEYWORD]; break;
    case TOKEN_TYPE:         color = ansi_colors[TOKEN_TYPE]; break;
    case TOKEN_STRING:       color = ansi_colors[TOKEN_STRING]; break;
    case TOKEN_NUMBER:       color = ansi_colors[TOKEN_NUMBER]; break;
    case TOKEN_COMMENT:      color = ansi_colors[TOKEN_COMMENT]; break;
    case TOKEN_PREPROCESSOR: color = ansi_colors[TOKEN_PREPROCESSOR]; break;
    case TOKEN_CONSTANT:     color = ansi_colors[TOKEN_CONSTANT]; break;
    default: break;
  }

  write(STDOUT_FILENO, color, strlen(color));
  write(STDOUT_FILENO, token, size);
  write(STDOUT_FILENO, ansi_colors[TOKEN_RESET], strlen(ansi_colors[TOKEN_RESET]));
}

void syntax_highlight_and_print(char *line, int size) {
  int pos = 0;
  int in_string = 0;

  while(pos < size) {
    while(pos < size && line[pos] == ' ') {
      write(STDOUT_FILENO, " ", 1);
      pos++;
    }
    
    int token_start = pos;
    int type;
    size_t token_len = 0;

    if(line[token_start] == '"') {
      type = TOKEN_STRING;
      pos++;
      while(pos < size && line[pos] != '"') pos++;
      token_len = pos - token_start;
    }
      
    print_colored_token(line + token_start, token_len, type);
  }
}
