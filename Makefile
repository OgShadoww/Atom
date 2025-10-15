CC = gcc
TARGET = atom
SRC = main.c include/menu.c include/syntax_highlight.c

$(TARGET): Makefile $(SRC)
	$(CC) $(SRC) -o $(TARGET)
