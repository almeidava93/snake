/*******************************************************************************************
*
*   raylib [core] example - basic window
*
*   Example complexity rating: [★☆☆☆] 1/4
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute 'raylib_compile_execute' script
*   Note that compiled executable is placed in the same folder as .c file
*
*   To test the examples on Web, press F6 and execute 'raylib_compile_execute_web' script
*   Web version of the program is generated in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

const int screenWidth = 800;
const int screenHeight = 450;
const int targetFPS = 60;       // Target frames-per-second
const char* windowTitle = "Snake";
const int initPlayerLifes = 3;   // Starting number of player lifes

// Player structure
typedef enum PlayerDirection {
    LEFT, 
    RIGHT,
    UP, 
    DOWN
} PlayerDirection;

typedef struct BodyPartState {
    int x;
    int y;
    PlayerDirection direction;
} BodyPartState;

typedef struct Player {
    Vector2 position;
    Vector2 stepSize;
    int framesPerStep;
    Vector2 size;
    Rectangle bounds;
    int lifes;
    int numBodyParts;
    BodyPartState bodyPartsStates[100];
    bool collided;
    PlayerDirection direction;
} Player;





//------------------------------------------------------------------------------------
// Player functions
//------------------------------------------------------------------------------------

Player* create_new_player(Player playerReference) {
    Player* player = calloc(1, sizeof(Player));
    *player = playerReference;

    for (size_t i = 0; i < player->numBodyParts; i++) {
        player->bodyPartsStates[i] = (BodyPartState) {
            .x = player->position.x + player->size.x * (i+1),
            .y = player->position.y,
            .direction = player->direction
        };
    }

    return player;
}

void reset_player(Player* player, Player playerReference) {
    *player = playerReference;

    for (size_t i = 0; i < player->numBodyParts; i++) {
        player->bodyPartsStates[i] = (BodyPartState) {
            .x = player->position.x + player->size.x * (i+1),
            .y = player->position.y,
            .direction = player->direction
        };
    }
}

void free_player(Player* player) {
    free(player);
}

void move_body_part(BodyPartState* body_part, PlayerDirection direction, Vector2 stepSize) {
    switch (direction) {
        case LEFT:
            body_part->x -= stepSize.x;
            break;
        case RIGHT:
            body_part->x += stepSize.x;
            break;
        case UP:
            body_part->y -= stepSize.y;
            break;
        case DOWN:
            body_part->y += stepSize.y;
            break;
    }
    body_part->direction = direction;
}

void move_player(Player* player) {
    if (player->collided) {
        return;
    }

    // Player movement logic
    BodyPartState prevBodyPartState = {
        .x = player->position.x,
        .y = player->position.y,
        .direction = player->direction
    }; // Position of the head or most recently manipulated body part

    if (IsKeyDown(KEY_LEFT) && player->direction != RIGHT) {
        player->position.x -= player->stepSize.x;
        player->direction = LEFT;
    } else if (IsKeyDown(KEY_RIGHT) && player->direction != LEFT) {
        player->position.x += player->stepSize.x;
        player->direction = RIGHT;
    } else if (IsKeyDown(KEY_UP) && player->direction != DOWN) {
        player->position.y -= player->stepSize.y;
        player->direction = UP;
    } else if (IsKeyDown(KEY_DOWN) && player->direction != UP) {
        player->position.y += player->stepSize.y;
        player->direction = DOWN;
    }

    // Move body
    if (prevBodyPartState.x != player->position.x || prevBodyPartState.y != player->position.y) {
        BodyPartState tempBodyPartState;
        for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts; body_part_idx++) {
            tempBodyPartState = player->bodyPartsStates[body_part_idx];
            move_body_part(
                &player->bodyPartsStates[body_part_idx],
                prevBodyPartState.direction,
                player->stepSize
            );
            prevBodyPartState = tempBodyPartState;
        }
    }
}

void checkCollisions(Player* player) {
    if (player->collided) {
        return;
    }

    // Check collision with screen bounds
    if (player->position.x > screenWidth || 
        player->position.x < 0 ||
        player->position.y > screenHeight ||
        player->position.y < 0
    ) {
        player->collided = true;
    }

    // Check collision with bodyparts
    Rectangle head = {player->position.x, player->position.y, player->size.x, player->size.y};
    for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts; body_part_idx++) {
        BodyPartState currentBodyPartState = player->bodyPartsStates[body_part_idx];
        Rectangle currentBodyPartRectangle = {
            currentBodyPartState.x,
            currentBodyPartState.y, 
            player->size.x, 
            player->size.y
        };
        if (CheckCollisionRecs(head, currentBodyPartRectangle)) {
            player->collided = true;
            break;
        }
    }
}

//------------------------------------------------------------------------------------
// Drawing functions
//------------------------------------------------------------------------------------

void DrawTextCentered(const char* text, int centerY, int fontSize, Color color) {
    int textWidth = MeasureText(text, fontSize);
    int posX = (GetScreenWidth() / 2) - (textWidth / 2);
    DrawText(text, posX, centerY, fontSize, color);
}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------

    int framesCounter = 0;

    // Initialize player
    Player startingPlayerReference = {
        .position     = {.x = screenWidth/2, .y = screenHeight/2},
        .size         = {.x = 20, .y = 20},
        .stepSize     = {.x = 20, .y = 20},
        .framesPerStep = 5,
        .lifes        = 5,
        .numBodyParts = 10,
        .collided     = false,
        .direction    = LEFT
    };
    Player* player = create_new_player(startingPlayerReference);
    

    typedef enum GameScreen { GAMEPLAY, GAMEOVER } GameScreen; 

    InitWindow(screenWidth, screenHeight, windowTitle);
    GameScreen currentGameScreen = GAMEPLAY;

    SetTargetFPS(targetFPS);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Displays
        //----------------------------------------------------------------------------------
        framesCounter++;
        float deltaTime = GetFrameTime();


        switch (currentGameScreen) {
            case GAMEPLAY:
                if (framesCounter % player->framesPerStep == 0) {
                    move_player(player);
                    checkCollisions(player);

                    if (player->collided) {
                        currentGameScreen = GAMEOVER;
                    }
                    break;
                }
                

            case GAMEOVER:
                if (IsKeyDown(KEY_ENTER)) {
                    currentGameScreen = GAMEPLAY;
                    reset_player(player, startingPlayerReference);
                }
        }

        

        // Draw
        BeginDrawing();
        
        // Check current screen and draw it
        switch (currentGameScreen) {
            case GAMEPLAY:
                ClearBackground(RAYWHITE);
                DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);
                DrawRectangle(player->position.x, player->position.y, player->size.x, player->size.y, BLACK);
                for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts; body_part_idx++) {
                    BodyPartState currentBodyPartState = player->bodyPartsStates[body_part_idx];
                    DrawRectangle(currentBodyPartState.x, currentBodyPartState.y, player->size.x, player->size.y, BLACK);
                }
                break;

            case GAMEOVER:
                ClearBackground(RAYWHITE);
                DrawTextCentered("GAME OVER", screenHeight/2, 20, DARKGRAY);
                DrawTextCentered("PRESS ENTER TO RESTART", screenHeight/2 + 25, 20, DARKGRAY);
                break;
        }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    free_player(player);
    //--------------------------------------------------------------------------------------

    return 0;
}