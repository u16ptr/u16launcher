#!/bin/bash

BUILD_DIR=build
EXECUTABLE_NAME=u16panel

mkdir -p $BUILD_DIR
gcc -o $BUILD_DIR/$EXECUTABLE_NAME src/Main.c -lX11 -lXpm
