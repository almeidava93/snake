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
  int lifes;
  int numBodyParts;
  BodyPartState bodyPartsStates[100];
  bool collided;
  PlayerDirection direction;
  int bodyPartsToAdd;
  int score;
} Player;

//------------------------------------------------------------------------------------
// Difficulty levels
//------------------------------------------------------------------------------------

typedef struct DifficultyLevelSetting {
  int framesPerStep;
  int stepSize;
  int scoreTrigger;
} DifficultyLevelSetting;

DifficultyLevelSetting difficultyLevels[] = {
    {.framesPerStep = 10, .stepSize = 20, .scoreTrigger = 10},
    {.framesPerStep = 9, .stepSize = 20, .scoreTrigger = 20},
    {.framesPerStep = 8, .stepSize = 20, .scoreTrigger = 30},
    {.framesPerStep = 7, .stepSize = 20, .scoreTrigger = 40},
    {.framesPerStep = 6, .stepSize = 20, .scoreTrigger = 50},
    {.framesPerStep = 5, .stepSize = 20, .scoreTrigger = 60},
    {.framesPerStep = 4, .stepSize = 20, .scoreTrigger = 70},
    {.framesPerStep = 3, .stepSize = 20, .scoreTrigger = 80},
    {.framesPerStep = 2, .stepSize = 20, .scoreTrigger = 90},
    {.framesPerStep = 1, .stepSize = 20, .scoreTrigger = 100},
};

//------------------------------------------------------------------------------------
// Random number generators and random selectors
//------------------------------------------------------------------------------------

// Returns a random number between min and max (inclusive)
int random_int(
    int min,
    int max) {  // Returns a random number between min and max (inclusive)
  return (rand() % (max - min + 1)) + min;
}

// Returns a random float between min and max (inclusive)
float random_float(float min, float max) {
  return ((float)rand() / RAND_MAX) * (max - min) + min;
}

// Returns a random number between 0 and numOptions, e.g., the index of a
// random element in an array of size numOptions
int random_select(int numOptions) { return random_int(0, numOptions - 1); }

//------------------------------------------------------------------------------------
// Food, obstacles and other elements
//------------------------------------------------------------------------------------

typedef struct FoodType {
  char* name[20];
  int points;
  float probability;
  Color color;
  Texture2D texture;
  char* texturePath[100];
} FoodType;

FoodType foodTypes[] = {{.name = "Apple",
                         .points = 1,
                         .probability = 0.6,
                         .color = RED,
                         .texturePath = "assets/apple.png"},
                        {.name = "Orange",
                         .points = 2,
                         .probability = 0.2,
                         .color = ORANGE,
                         .texturePath = "assets/apple.png"},
                        {.name = "Banana",
                         .points = 3,
                         .probability = 0.1,
                         .color = YELLOW,
                         .texturePath = "assets/banana.png"},
                        {.name = "Grapes",
                         .points = 4,
                         .probability = 0.05,
                         .color = PURPLE,
                         .texturePath = "assets/apple.png"},
                        {.name = "Watermelon",
                         .points = 5,
                         .probability = 0.05,
                         .color = GREEN,
                         .texturePath = "assets/watermelon.png"}};

typedef struct Food {
  Vector2 position;
  Vector2 size;
  FoodType type;
  int points;
} Food;

FoodType sample_food_type() {
  float randomValue = random_float(0.0, 1.0);
  float cumulativeProbability = 0.0;

  for (size_t i = 0; i < sizeof(foodTypes) / sizeof(FoodType); i++) {
    cumulativeProbability += foodTypes[i].probability;
    if (randomValue <= cumulativeProbability) {
      return foodTypes[i];
    }
  }

  // Fallback in case of rounding errors
  return foodTypes[sizeof(foodTypes) / sizeof(FoodType) - 1];
}

Food* create_new_food(void) {
  Food* food = calloc(1, sizeof(Food));
  food->size.x = 20;
  food->size.y = 20;
  food->type = sample_food_type();

  // Generate random position for the food
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

  // DrawRectangle(food->position.x, food->position.y, food->size.x,
  // food->size.y,      food->type.color);
  float scale = 1.5;
  float rotation = 0.0;
  Vector2 texturePosition = {food->position.x - (food->size.x * scale) / 2,
                             food->position.y - (food->size.y * scale) / 2};
  DrawTextureEx(food->type.texture, texturePosition, rotation, scale, WHITE);
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
    player->score += food->type.points;
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
  srand(time(NULL));  // seed random number generator with current time
  int framesCounter = 0;
  bool foodIsAvailable = false;
  Food* currentFood = NULL;
  int currentDifficultyLevel = 0;

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
  // Load Textures
  //--------------------------------------------------------------------------------------
  for (size_t i = 0; i < sizeof(foodTypes) / sizeof(FoodType); i++) {
    foodTypes[i].texture = LoadTexture(*foodTypes[i].texturePath);
  }

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

    switch (currentGameScreen) {
      case GAMEPLAY:
        // Food
        if (foodIsAvailable == false) {
          currentFood = create_new_food();
          foodIsAvailable = true;
        }

        // Movement
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

        // Difficulty level
        if (player->score >=
                difficultyLevels[currentDifficultyLevel].scoreTrigger &&
            currentDifficultyLevel <
                sizeof(difficultyLevels) / sizeof(DifficultyLevelSetting) - 1) {
          player->framesPerStep =
              difficultyLevels[currentDifficultyLevel].framesPerStep;
          player->stepSize =
              (Vector2){.x = difficultyLevels[currentDifficultyLevel].stepSize,
                        .y = difficultyLevels[currentDifficultyLevel].stepSize};
          currentDifficultyLevel++;
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
        DrawText(TextFormat("Score: %d", player->score), 10, 40, 20, DARKGRAY);
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

  // Unload textures
  for (size_t i = 0; i < sizeof(foodTypes) / sizeof(FoodType); i++) {
    UnloadTexture(foodTypes[i].texture);
  }
  //--------------------------------------------------------------------------------------

  return 0;
}