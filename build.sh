#!/bin/sh

clang -Wall -Wextra -Wno-c99-designator -DENABLE_ASSERT=1 -L/usr/local/lib -I/usr/local/include -lSDL2 snake.cpp -o snake
