# Detect operating system
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# Define compile command
ifeq ($(DETECTED_OS), Windows)
	COMPILE_COMMAND = gcc main.c -o snake.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
else ifeq ($(DETECTED_OS),Darwin)
	COMPILE_COMMAND = gcc main.c -o snake.exe -L/opt/homebrew/lib -I/opt/homebrew/include -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
endif

compile:
	$(COMPILE_COMMAND)

run: compile
	./snake.exe