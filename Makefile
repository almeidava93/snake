# Detect operating system
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# Define compile command
ifeq ($(DETECTED_OS), Windows)
	COMPILE_COMMAND = gcc main.c -o snake.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm

# MacOS command varies depending on the chip: Intel vs Apple Silicon
else ifeq ($(DETECTED_OS),Darwin)
    HAS_APPLE_SILICON := $(shell sysctl -a | grep -q "hw.optional.arm64: 1" && echo yes || echo no)
	ifeq ($(HAS_APPLE_SILICON),yes)
        COMPILE_COMMAND = gcc main.c -o snake.exe -L/opt/homebrew/lib -I/opt/homebrew/include -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    else
        COMPILE_COMMAND = gcc main.c -o snake.exe -L/usr/local/lib -I/usr/local/include -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    endif
endif

compile:
	$(COMPILE_COMMAND)

run: compile
	./snake.exe

format:
	find . -regex '.*\.\(cpp\|hpp\|c\|h\)' -exec clang-format -style=file -i {} \;
