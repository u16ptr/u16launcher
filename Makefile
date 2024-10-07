CC = gcc
CFLAGS = -Wall -Wextra -O2

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/u16panel
SRC = $(SRC_DIR)/Main.c
LIBS = -lX11 -lXpm

$(TARGET): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean
