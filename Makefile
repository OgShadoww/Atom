CC = gcc
TARGET = atom
SRC = main.c include/menu.c include/syntax_highlight.c include/file_browser.c

$(TARGET): Makefile $(SRC)
	$(CC) $(SRC) -o $(TARGET)
