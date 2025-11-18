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
  TOKEN_FUNCTION,
  TOKEN_BRACKET,
  TOKEN_UNKNOWN,
} TokenType;

const char *ansi_colors[] = {  
  [TOKEN_RESET]       = "\033[0m",
  [TOKEN_KEYWORD]     = "\033[38;2;255;121;198m",  // Pink
  [TOKEN_TYPE]        = "\033[38;2;139;233;253m",  // Cyan
  [TOKEN_PREPROCESSOR]= "\033[38;2;241;250;140m",  // Yellow
  [TOKEN_CONSTANT]    = "\033[38;2;189;147;249m",  // Purple
  [TOKEN_STRING]      = "\033[38;2;241;250;140m",  // Yellow
  [TOKEN_NUMBER]      = "\033[38;2;189;147;249m",  // Purple
  [TOKEN_COMMENT]     = "\033[38;2;98;114;164m",   // Dark Blue-Gray
  [TOKEN_OPERATOR]    = "\033[38;2;248;248;242m",  // White
  [TOKEN_FUNCTION]    = "\033[38;2;80;250;123m",   // Green
  [TOKEN_BRACKET]     = "\033[38;2;248;248;242m",  // White
  [TOKEN_IDENTIFIER]  = "\033[38;2;248;248;242m",  // White
  [TOKEN_UNKNOWN]     = "\033[38;2;248;248;242m",  // White}
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
  // Check if the preprocessor
  if(token[0] == '#') {
    for(int i = 0; c_preprocessor[i] != NULL; i++) {
      int prep_len = strlen(c_preprocessor[i]);
      if(size >= prep_len && strncmp(token, c_preprocessor[i], prep_len) == 0) {
        return TOKEN_PREPROCESSOR;
      }
    }
  }
 
  // Checking for the keywords 
  for(int i = 0; c_keywords[i] != NULL; i++) {
    if(size == strlen(c_keywords[i]) && strncmp(token, c_keywords[i], size) == 0) {
      return TOKEN_KEYWORD;
    }
  }

  // Checking for the types
  for(int i = 0; c_types[i] != NULL; i++) {
    if(size == strlen(c_types[i]) && strncmp(token, c_types[i], size) == 0) {
      return TOKEN_TYPE;
    }
  }

  // Checking for the constants
  for(int i = 0; c_constants[i] != NULL; i++) {
    if(size == strlen(c_constants[i]) && strncmp(token, c_constants[i], size) == 0) {
      return TOKEN_CONSTANT;
    }
  }
  
  return TOKEN_UNKNOWN;
}

void print_colored_token(char *token, size_t size, int type) {
  const char *color = ansi_colors[type];
  
  if(color != NULL) {
    write(STDOUT_FILENO, color, strlen(color));
    write(STDOUT_FILENO, token, size);
    write(STDOUT_FILENO, ansi_colors[TOKEN_RESET], strlen(ansi_colors[TOKEN_RESET]));
  } 
  else {
    write(STDOUT_FILENO, token, size);
  }
}

void syntax_highlight_and_print(char *line, int size) {
  int pos = 0;
  int after_include = 0;

  while(pos < size) {
    // Handle whitespaces
    while (pos < size && isspace((unsigned char)line[pos])) {
      write(STDOUT_FILENO, &line[pos], 1);
      pos++;
    }
    
    int token_start = pos;
    int type = TOKEN_UNKNOWN;
    size_t token_len = 0;

    // Handle strings
    if(line[token_start] == '"') {
      type = TOKEN_STRING;
      pos++;
      while(pos < size && line[pos] != '"') pos++;
      if (pos < size && line[pos] == '"') pos++;
      token_len = pos - token_start;
      print_colored_token(line + token_start, token_len, type);
      continue;
    }
    // Handle comments
    if(token_start + 1 < size && line[token_start] == '/' && line[token_start + 1] == '/') {
      type = TOKEN_COMMENT;
      token_len = size - pos;
      
      print_colored_token(line + token_start, token_len, type);
      break;
    } 
    // Handle numbers
    if(isdigit(line[pos])) {
      type = TOKEN_NUMBER;
      while(pos < size && (isdigit(line[pos]) || line[pos] == '.' || tolower(line[pos]) == 'f' || tolower(line[pos]) == 'x')) pos++;  
      token_len = pos - token_start;
      print_colored_token(line + token_start, token_len, type);
      continue;
    }
    // Handle words: Keywords, types, operators, preprocessor
    if(isalnum(line[pos]) || line[pos] == '_' || line[pos] == '#') {
      // Handle preprocessor
      if(line[pos] == '#') {
        while(pos < size && (isalnum(line[pos]) || line[pos] == '_' || line[pos] == '#')) {
          pos++;
        }
      }
      else {
        while(pos < size && (isalnum(line[pos]) || line[pos] == '_')) {
          pos++;
        }
      }

      if(strncmp(line + token_start, "#include", 8) == 0) {
        after_include = 1;  // Next < > should be colored
      }
      
      token_len = pos - token_start;
      type = classify_token(line + token_start, token_len);

      print_colored_token(line + token_start, token_len, type);
      continue;
    }
    // Handle OPERATORS and PUNCTUATION
    write(STDOUT_FILENO, &line[pos], 1);
    pos++;
  }
}
