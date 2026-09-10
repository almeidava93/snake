compile:
	gcc main.c -o snake.exe -O1 -Wall -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm

run: compile
	./snake.exe