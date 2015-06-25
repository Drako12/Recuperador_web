CC = gcc
CFLAGS = -g -Wall
TARGET = dget
HEADER = client.h
SOURCE = client.c

all: $(TARGET)

$TARGET: $TARGET.o

	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
