CC = gcc
TARGET = atom
SRC = main.c include/menu.c

$(TARGET): Makefile $(SRC)
	$(CC) $(SRC) -o $(TARGET)
