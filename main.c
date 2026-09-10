#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

const int screenWidth = 800;
const int screenHeight = 450;
const int targetFPS = 60;  // Target frames-per-second
const char* windowTitle = "Snake";
const int initPlayerLifes = 3;  // Starting number of player lifes

// Player structure
typedef enum PlayerDirection { LEFT, RIGHT, UP, DOWN } PlayerDirection;

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
  int bodyPartsToAdd;
} Player;

//------------------------------------------------------------------------------------
// Food, obstacles and other elements
//------------------------------------------------------------------------------------

typedef enum FoodType { APPLE_FRUIT, ORANGE_FRUIT, BANANA_FRUIT } FoodType;

typedef struct Food {
  Vector2 position;
  Vector2 size;
  FoodType type;
} Food;

Food* create_new_food(void) {
  Food* food = calloc(1, sizeof(Food));
  food->size.x = 20;
  food->size.y = 20;
  food->type = APPLE_FRUIT;

  // Generate random position for the food
  srand(time(NULL));  // seed random number generator with current time
  int min = 0;
  int max_x = screenWidth - food->size.x;
  int max_y = screenHeight - food->size.y;

  food->position.x = (rand() % (max_x - min + 1)) + min;
  food->position.y = (rand() % (max_y - min + 1)) + min;

  return food;
}

void free_food(Food* food) {
  if (food == NULL) {
    return;
  }
  free(food);
}

void draw_food(Food* food) {
  if (food == NULL) return;
  DrawRectangle(food->position.x, food->position.y, food->size.x, food->size.y,
                RED);
}

//------------------------------------------------------------------------------------
// Player functions
//------------------------------------------------------------------------------------

Player* create_new_player(Player playerReference) {
  Player* player = calloc(1, sizeof(Player));
  *player = playerReference;

  for (size_t i = 0; i < player->numBodyParts; i++) {
    player->bodyPartsStates[i] =
        (BodyPartState){.x = player->position.x + player->size.x * (i + 1),
                        .y = player->position.y,
                        .direction = player->direction};
  }

  return player;
}

void reset_player(Player* player, Player playerReference) {
  *player = playerReference;

  for (size_t i = 0; i < player->numBodyParts; i++) {
    player->bodyPartsStates[i] =
        (BodyPartState){.x = player->position.x + player->size.x * (i + 1),
                        .y = player->position.y,
                        .direction = player->direction};
  }
}

void free_player(Player* player) { free(player); }

void move_head(Player* player) {
  switch (player->direction) {
    case LEFT:
      player->position.x -= player->stepSize.x;
      break;
    case RIGHT:
      player->position.x += player->stepSize.x;
      break;
    case UP:
      player->position.y -= player->stepSize.y;
      break;
    case DOWN:
      player->position.y += player->stepSize.y;
      break;
  }
}

void update_player_direction(Player* player) {
  if (IsKeyDown(KEY_LEFT) && player->direction != RIGHT) {
    player->direction = LEFT;
  } else if (IsKeyDown(KEY_RIGHT) && player->direction != LEFT) {
    player->direction = RIGHT;
  } else if (IsKeyDown(KEY_UP) && player->direction != DOWN) {
    player->direction = UP;
  } else if (IsKeyDown(KEY_DOWN) && player->direction != UP) {
    player->direction = DOWN;
  }
}

void move_player(Player* player) {
  if (player->collided) {
    return;
  }

  // Save the head's current position before it moves
  BodyPartState prevBodyPartState = {.x = player->position.x,
                                     .y = player->position.y,
                                     .direction = player->direction};

  // Move the head
  move_head(player);

  // Update body parts' position
  BodyPartState tempBodyPartState;
  for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts;
       body_part_idx++) {
    tempBodyPartState = player->bodyPartsStates[body_part_idx];

    // Move this body part into into the previous part position
    player->bodyPartsStates[body_part_idx].x = prevBodyPartState.x;
    player->bodyPartsStates[body_part_idx].y = prevBodyPartState.y;
    player->bodyPartsStates[body_part_idx].direction =
        prevBodyPartState.direction;

    // Update previous body part position
    prevBodyPartState = tempBodyPartState;
  }

  if (player->bodyPartsToAdd > 0) {
    player->bodyPartsStates[player->numBodyParts] = prevBodyPartState;
    player->bodyPartsToAdd--;
    player->numBodyParts++;
  }
}

void checkCollisions(Player* player) {
  if (player->collided) {
    return;
  }

  // Check collision with screen bounds
  if (player->position.x > screenWidth || player->position.x < 0 ||
      player->position.y > screenHeight || player->position.y < 0) {
    player->collided = true;
  }

  // Check collision with bodyparts
  Rectangle head = {player->position.x, player->position.y, player->size.x,
                    player->size.y};
  for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts;
       body_part_idx++) {
    BodyPartState currentBodyPartState = player->bodyPartsStates[body_part_idx];
    Rectangle currentBodyPartRectangle = {currentBodyPartState.x,
                                          currentBodyPartState.y,
                                          player->size.x, player->size.y};
    if (CheckCollisionRecs(head, currentBodyPartRectangle)) {
      player->collided = true;
      break;
    }
  }
}

void checkIfFoundFood(Player* player, Food* food, bool* foodIsAvailable) {
  if (food == NULL) return;
  Rectangle head = {player->position.x, player->position.y, player->size.x,
                    player->size.y};
  Rectangle foodRect = {food->position.x, food->position.y, food->size.x,
                        food->size.y};
  if (CheckCollisionRecs(head, foodRect)) {
    player->bodyPartsToAdd++;
    free_food(food);
    *foodIsAvailable = false;
  }
}

//------------------------------------------------------------------------------------
// Drawing functions
//------------------------------------------------------------------------------------

void DrawTextCentered(const char* text, int centerY, int fontSize,
                      Color color) {
  int textWidth = MeasureText(text, fontSize);
  int posX = (GetScreenWidth() / 2) - (textWidth / 2);
  DrawText(text, posX, centerY, fontSize, color);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------

int main(void) {
  // Initialization
  //--------------------------------------------------------------------------------------

  int framesCounter = 0;
  bool foodIsAvailable = false;
  Food* currentFood;

  // Initialize player
  Player startingPlayerReference = {
      .position = {.x = screenWidth / 2, .y = screenHeight / 2},
      .size = {.x = 20, .y = 20},
      .stepSize = {.x = 20, .y = 20},
      .framesPerStep = 10,
      .lifes = 5,
      .numBodyParts = 10,
      .collided = false,
      .direction = LEFT,
      .bodyPartsToAdd = 0};
  Player* player = create_new_player(startingPlayerReference);

  typedef enum GameScreen { GAMEPLAY, GAMEOVER } GameScreen;

  InitWindow(screenWidth, screenHeight, windowTitle);
  GameScreen currentGameScreen = GAMEPLAY;

  SetTargetFPS(targetFPS);  // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose())  // Detect window close button or ESC key
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
        if (foodIsAvailable == false) {
          currentFood = create_new_food();
          foodIsAvailable = true;
        }

        update_player_direction(player);
        if (framesCounter % player->framesPerStep == 0) {
          move_player(player);
          checkCollisions(player);
          checkIfFoundFood(player, currentFood, &foodIsAvailable);

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

        // Draw snake
        DrawRectangle(player->position.x, player->position.y, player->size.x,
                      player->size.y, BLACK);
        for (size_t body_part_idx = 0; body_part_idx < player->numBodyParts;
             body_part_idx++) {
          BodyPartState currentBodyPartState =
              player->bodyPartsStates[body_part_idx];
          DrawRectangle(currentBodyPartState.x, currentBodyPartState.y,
                        player->size.x, player->size.y, BLACK);
        }

        // Draw food
        draw_food(currentFood);

        break;

      case GAMEOVER:
        ClearBackground(RAYWHITE);
        DrawTextCentered("GAME OVER", screenHeight / 2, 20, DARKGRAY);
        DrawTextCentered("PRESS ENTER TO RESTART", screenHeight / 2 + 25, 20,
                         DARKGRAY);
        break;
    }

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow();  // Close window and OpenGL context
  free_player(player);
  free_food(currentFood);
  //--------------------------------------------------------------------------------------

  return 0;
}