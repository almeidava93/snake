#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"

const int screenWidth = 800;
const int screenHeight = 450;
const int targetFPS = 60;  // Target frames-per-second
const char* windowTitle = "Snake";
const int initPlayerLifes = 3;  // Starting number of player lifes
const Color BackgroundColorGameplay = {218, 233, 201, 255};

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
// Sound collection
//------------------------------------------------------------------------------------

typedef struct SoundCollection {
  int count;
  Sound biteSounds[10];
} SoundCollection;

SoundCollection create_sound_collection() {
  SoundCollection soundCollection;
  soundCollection.count = 0;
  return soundCollection;
}

void add_sound(SoundCollection* soundCollection, Sound sound) {
  if (soundCollection->count < 10) {
    soundCollection->biteSounds[soundCollection->count] = sound;
    soundCollection->count++;
  } else {
    printf("Sound collection is full. Cannot add more sounds.\n");
  }
}

void play_random_sound(SoundCollection* soundCollection) {
  int randomIndex = random_select(soundCollection->count);
  PlaySound(soundCollection->biteSounds[randomIndex]);
}

void unload_sound_collection(SoundCollection* soundCollection) {
  for (int i = 0; i < soundCollection->count; i++) {
    UnloadSound(soundCollection->biteSounds[i]);
  }
}

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
// Food, obstacles and other elements
//------------------------------------------------------------------------------------

typedef struct FoodType {
  char* name[20];
  int points;
  float probability;
  Color color;
  Texture2D texture;
  char* texturePath[100];
  bool isLive;
} FoodType;

FoodType foodTypes[] = {{.name = "Apple",
                         .points = 1,
                         .probability = 0.45,
                         .color = RED,
                         .texturePath = "assets/apple.png"},
                        {.name = "Orange",
                         .points = 2,
                         .probability = 0.2,
                         .color = ORANGE,
                         .texturePath = "assets/orange.png"},
                        {.name = "Banana",
                         .points = 3,
                         .probability = 0.1,
                         .color = YELLOW,
                         .texturePath = "assets/banana.png"},
                        {.name = "Grape",
                         .points = 4,
                         .probability = 0.05,
                         .color = PURPLE,
                         .texturePath = "assets/grape.png"},
                        {.name = "Watermelon",
                         .points = 5,
                         .probability = 0.05,
                         .color = GREEN,
                         .texturePath = "assets/watermelon.png"},
                        {.name = "Live rat",
                         .points = 6,
                         .probability = 0.15,
                         .color = BROWN,
                         .texturePath = "assets/rat-run.png",
                         .isLive = true}};

typedef struct Food {
  Vector2 position;
  Vector2 size;
  FoodType type;
  int points;
  PlayerDirection direction;
  float turnTimer;
  float animationTime;
  int animationFrame;
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

bool food_position_blocked(Rectangle area, const Player* player) {
  if (area.x < 0 || area.y < 0 || area.x + area.width > screenWidth ||
      area.y + area.height > screenHeight) return true;

  Rectangle head = {player->position.x, player->position.y,
                     player->size.x, player->size.y};
  if (CheckCollisionRecs(area, head)) return true;
  for (int i = 0; i < player->numBodyParts; i++) {
    Rectangle body = {player->bodyPartsStates[i].x, player->bodyPartsStates[i].y,
                       player->size.x, player->size.y};
    if (CheckCollisionRecs(area, body)) return true;
  }
  return false;
}

Food* create_new_food(const Player* player) {
  Food* food = calloc(1, sizeof(Food));
  if (food == NULL) return NULL;
  food->size.x = 20;
  food->size.y = 20;
  food->type = sample_food_type();
  food->direction = random_select(4);
  food->turnTimer = random_float(0.6f, 1.4f);

  // Generate random position for the food
  int min = 0;
  int max_x = screenWidth - food->size.x;
  int max_y = screenHeight - food->size.y;

  // Never spawn food inside the snake. Retry next frame if no spot is found.
  for (int attempt = 0; attempt < 128; attempt++) {
    food->position.x = random_int(min, max_x);
    food->position.y = random_int(min, max_y);
    Rectangle area = {food->position.x, food->position.y,
                       food->size.x, food->size.y};
    if (!food_position_blocked(area, player)) return food;
  }
  free(food);
  return NULL;
}

void update_live_food(Food* food, const Player* player, float deltaTime) {
  if (food == NULL || !food->type.isLive || player->collided) return;

  food->turnTimer -= deltaTime;
  if (food->turnTimer <= 0.0f) {
    food->direction = random_select(4);
    food->turnTimer = random_float(0.6f, 1.4f);
  }
  const Vector2 directions[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  Vector2 direction = directions[food->direction];
  float remaining = 55.0f * deltaTime;
  float moved = 0.0f;
  while (remaining > 0.0f) {
    // Sweep the entire path, rather than only checking its endpoint.
    float step = remaining > 1.0f ? 1.0f : remaining;
    Rectangle next = {food->position.x + direction.x * step,
                       food->position.y + direction.y * step,
                       food->size.x, food->size.y};
    if (food_position_blocked(next, player)) {
      food->direction = (food->direction + random_int(1, 3)) % 4;
      food->turnTimer = random_float(0.6f, 1.4f);
      break;
    }
    food->position = (Vector2){next.x, next.y};
    moved += step;
    remaining -= step;
  }
  if (moved > 0.0f) {
    food->animationTime += moved / 55.0f;
    while (food->animationTime >= 0.1f) {
      food->animationTime -= 0.1f;
      food->animationFrame = (food->animationFrame + 1) % 4;
    }
  } else {
    food->animationTime = 0.0f;
    food->animationFrame = 0;
  }
}

void free_food(Food* food) {
  if (food == NULL) {
    return;
  }
  free(food);
}

void draw_food(Food* food) {
  if (food == NULL) return;

  if (food->type.isLive) {
    const float rotations[] = {0, 180, 90, 270};
    int frameWidth = food->type.texture.width / 4;
    Rectangle source = {food->animationFrame * frameWidth, 0,
                         frameWidth, food->type.texture.height};
    Vector2 origin = {food->size.x / 2, food->size.y / 2};
    Rectangle destination = {food->position.x + origin.x,
                              food->position.y + origin.y,
                              food->size.x, food->size.y};
    DrawTexturePro(food->type.texture, source, destination, origin,
                   rotations[food->direction], WHITE);
    return;
  }

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

bool checkIfFoundFood(Player* player, Food* food, bool* foodIsAvailable) {
  if (food == NULL) return false;
  Rectangle head = {player->position.x, player->position.y, player->size.x,
                    player->size.y};
  Rectangle foodRect = {food->position.x, food->position.y, food->size.x,
                        food->size.y};
  if (CheckCollisionRecs(head, foodRect)) {
    player->bodyPartsToAdd++;
    player->score += food->type.points;
    free_food(food);
    *foodIsAvailable = false;
    return true;
  }
  return false;
}

void draw_snake_head(Player* player, Texture2D snakeHeadTexture,
                     Texture2D tongueTexture, bool showTongue) {
  float scale = 1.0;
  float rotation = 0.0f;
  switch (player->direction) {
    case LEFT:
      rotation = 0.0f;
      break;
    case RIGHT:
      rotation = 180.0f;
      break;
    case UP:
      rotation = 90.0f;
      break;
    case DOWN:
      rotation = 270.0f;
      break;
  }

  // Define the source rectangle (the whole texture)
  Rectangle sourceRec = {0.0f, 0.0f, (float)snakeHeadTexture.width,
                         (float)snakeHeadTexture.height};

  // Define destination rectangle (position and scaled size on screen)
  Rectangle destRec = {
      player->position.x + (float)snakeHeadTexture.width * scale * 0.5f,
      player->position.y + (float)snakeHeadTexture.height * scale * 0.5f,
      (float)snakeHeadTexture.width * scale,
      (float)snakeHeadTexture.height * scale};

  // Set origin to the center of the destination rectangle (pivot point)
  Vector2 origin = {(float)snakeHeadTexture.width * scale * 0.5f,
                    (float)snakeHeadTexture.height * scale * 0.5f};

  // Draw with rotation around the center
  if (showTongue) {
    Rectangle tongueSource = {0, 0, tongueTexture.width, tongueTexture.height};
    Rectangle tongueDest = {destRec.x, destRec.y, tongueTexture.width,
                            tongueTexture.height};
    // Rotate around the head center; tuck the tongue root one pixel under it.
    Vector2 tongueOrigin = {tongueTexture.width + origin.x - 1,
                            tongueTexture.height * 0.5f};
    DrawTexturePro(tongueTexture, tongueSource, tongueDest, tongueOrigin,
                   rotation, WHITE);
  }
  DrawTexturePro(snakeHeadTexture, sourceRec, destRec, origin, rotation, WHITE);
}

void draw_snake_bodypart(BodyPartState* bodyPart, Texture2D snakeBodyTexture,
                         PlayerDirection rotationDirection) {
  float scale = 1.0;
  float rotation = 0.0f;
  switch (rotationDirection) {
    case LEFT:
      rotation = 0.0f;
      break;
    case RIGHT:
      rotation = 180.0f;
      break;
    case UP:
      rotation = 90.0f;
      break;
    case DOWN:
      rotation = 270.0f;
      break;
  }

  // Define the source rectangle (the whole texture)
  Rectangle sourceRec = {0.0f, 0.0f, (float)snakeBodyTexture.width,
                         (float)snakeBodyTexture.height};

  // Define destination rectangle (position and scaled size on screen)
  Rectangle destRec = {
      bodyPart->x + (float)snakeBodyTexture.width * scale * 0.5f,
      bodyPart->y + (float)snakeBodyTexture.height * scale * 0.5f,
      (float)snakeBodyTexture.width * scale,
      (float)snakeBodyTexture.height * scale};

  // Set origin to the center of the destination rectangle (pivot point)
  Vector2 origin = {(float)snakeBodyTexture.width * scale * 0.5f,
                    (float)snakeBodyTexture.height * scale * 0.5f};

  // Draw with rotation around the center
  DrawTexturePro(snakeBodyTexture, sourceRec, destRec, origin, rotation, WHITE);
}

typedef struct SnakeTextures {
  Texture2D head;
  Texture2D bodyStraight;
  Texture2D bodyClockwiseTurn;
  Texture2D bodyCounterClockwiseTurn;
  Texture2D tail;
} SnakeTextures;

enum { SNAKE_OPTION_COUNT = 3 };

typedef struct SnakeOption {
  const char* id;
  const char* name;
  const char* species;
  SnakeTextures textures;
} SnakeOption;

Rectangle snake_option_bounds(int index) {
  return (Rectangle){48 + index * 240, 210, 224, 140};
}

//------------------------------------------------------------------------------------
// Drawing functions
//------------------------------------------------------------------------------------

void draw_gameplay_background(void) {
  ClearBackground(BackgroundColorGameplay);
  const Color alternateGrass = {213, 229, 195, 255};
  const Color grassShadow = {199, 217, 181, 255};
  const Color grassHighlight = {230, 240, 215, 255};

  for (int y = 0; y < screenHeight; y += 40) {
    for (int x = 0; x < screenWidth; x += 40) {
      int column = x / 40;
      int row = y / 40;
      if ((column + row) % 2 == 0) {
        DrawRectangle(x, y, 40, 40, alternateGrass);
      }

      // Fixed variation keeps the grass still without consuming game randomness.
      int pattern = (column * 17 + row * 31) % 11;
      if (pattern < 3) {
        int grassX = x + 9 + pattern * 7;
        int grassY = y + 12 + pattern * 5;
        DrawRectangle(grassX, grassY, 2, 4, grassShadow);
        DrawRectangle(grassX - 2, grassY - 2, 2, 3, grassShadow);
        DrawRectangle(grassX + 2, grassY - 3, 2, 4, grassHighlight);
      }
    }
  }
}

void DrawTextCentered(const char* text, int centerY, int fontSize,
                      Color color) {
  int textWidth = MeasureText(text, fontSize);
  int posX = (GetScreenWidth() / 2) - (textWidth / 2);
  DrawText(text, posX, centerY, fontSize, color);
}

void draw_snake_options(SnakeOption* options, int selected) {
  DrawTextCentered("CHOOSE YOUR SNAKE", 170, 20, DARKGRAY);
  for (int i = 0; i < SNAKE_OPTION_COUNT; i++) {
    Rectangle card = snake_option_bounds(i);
    Color border = i == selected ? (Color){65, 105, 60, 255} : LIGHTGRAY;
    Color fill = i == selected ? (Color){224, 238, 204, 255} :
                                 (Color){240, 242, 232, 255};
    DrawRectangleRec(card, fill);
    DrawRectangleLinesEx(card, i == selected ? 3 : 1, border);
    DrawText(options[i].name,
             card.x + (card.width - MeasureText(options[i].name, 20)) / 2,
             card.y + 14, 20, DARKGRAY);
    DrawTextureEx(options[i].textures.head,
                  (Vector2){card.x + 32, card.y + 50}, 0, 2, WHITE);
    for (int part = 0; part < 3; part++) {
      DrawTextureEx(part == 2 ? options[i].textures.tail :
                               options[i].textures.bodyStraight,
                    (Vector2){card.x + 72 + part * 40, card.y + 50},
                    0, 2, WHITE);
    }
    DrawText(options[i].species,
             card.x + (card.width - MeasureText(options[i].species, 10)) / 2,
             card.y + 116, 10, DARKGRAY);
  }
  DrawTextCentered("LEFT / RIGHT  -  1 / 2 / 3  -  CLICK TO SELECT", 372, 10,
                   DARKGRAY);
  DrawTextCentered("PRESS ENTER TO START", 402, 20, DARKGRAY);
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
  InitAudioDevice();
  Sound snakeSound = LoadSound("assets/sound/snake/var-1.wav");
  float snakeSoundTimer = 0.0f;

  SoundCollection biteSoundCollection = create_sound_collection();
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-1.mp3"));
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-2.wav"));
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-3.wav"));
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-4.wav"));
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-5.wav"));
  add_sound(&biteSoundCollection,
            LoadSound("assets/sound/bites/bite-var-6.wav"));

  SoundCollection levelUpSoundCollection = create_sound_collection();
  add_sound(&levelUpSoundCollection,
            LoadSound("assets/sound/levelup/var-1.wav"));

  SoundCollection gameOverSoundCollection = create_sound_collection();
  add_sound(&gameOverSoundCollection,
            LoadSound("assets/sound/gameover/var-1.wav"));

  // Initialize player
  Player startingPlayerReference = {
      .position = {.x = screenWidth / 2, .y = screenHeight / 2},
      .size = {.x = 20, .y = 20},
      .stepSize = {.x = difficultyLevels[0].stepSize,
                   .y = difficultyLevels[0].stepSize},
      .framesPerStep = difficultyLevels[0].framesPerStep,
      .lifes = 5,
      .numBodyParts = 10,
      .collided = false,
      .direction = LEFT,
      .bodyPartsToAdd = 0};
  Player* player = create_new_player(startingPlayerReference);

  typedef enum GameScreen { START, GAMEPLAY, GAMEOVER } GameScreen;

  InitWindow(screenWidth, screenHeight, windowTitle);
  GameScreen currentGameScreen = START;

  SetTargetFPS(targetFPS);  // Set our game to run at 60 frames-per-second

  //--------------------------------------------------------------------------------------
  // Load Textures
  //--------------------------------------------------------------------------------------
  for (size_t i = 0; i < sizeof(foodTypes) / sizeof(FoodType); i++) {
    foodTypes[i].texture = LoadTexture(*foodTypes[i].texturePath);
  }
  SnakeOption snakeOptions[SNAKE_OPTION_COUNT] = {
      {.id = "sucuri", .name = "SUCURI", .species = "Eunectes murinus"},
      {.id = "caninana", .name = "CANINANA", .species = "Spilotes pullatus"},
      {.id = "cascavel", .name = "CASCAVEL", .species = "Crotalus durissus"}};
  int selectedSnake = 0;
  Texture2D snakeLogo = LoadTexture("assets/snake-logo.png");
  Texture2D snakeTongue = LoadTexture("assets/snake-tongue.png");
  Texture2D gameOverArt = LoadTexture("assets/game-over.png");
  for (int i = 0; i < SNAKE_OPTION_COUNT; i++) {
    const char* id = snakeOptions[i].id;
    SnakeTextures* textures = &snakeOptions[i].textures;
    textures->head = LoadTexture(TextFormat("assets/snakes/%s-head.png", id));
    textures->tail = LoadTexture(TextFormat("assets/snakes/%s-tail.png", id));
    textures->bodyStraight =
        LoadTexture(TextFormat("assets/snakes/%s-body-part-straight.png", id));
    textures->bodyClockwiseTurn = LoadTexture(
        TextFormat("assets/snakes/%s-body-part-clockwise-turn.png", id));
    textures->bodyCounterClockwiseTurn = LoadTexture(
        TextFormat("assets/snakes/%s-body-part-counter-clockwise-turn.png", id));
  }
  SnakeTextures snakeTextures = snakeOptions[selectedSnake].textures;

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
      case START:
        if (IsKeyPressed(KEY_LEFT)) {
          selectedSnake = (selectedSnake + SNAKE_OPTION_COUNT - 1) %
                          SNAKE_OPTION_COUNT;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
          selectedSnake = (selectedSnake + 1) % SNAKE_OPTION_COUNT;
        }
        for (int i = 0; i < SNAKE_OPTION_COUNT; i++) {
          if (IsKeyPressed(KEY_ONE + i) ||
              (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
               CheckCollisionPointRec(GetMousePosition(), snake_option_bounds(i)))) {
            selectedSnake = i;
          }
        }
        snakeTextures = snakeOptions[selectedSnake].textures;
        if (IsKeyPressed(KEY_ENTER)) {
          currentGameScreen = GAMEPLAY;
          snakeSoundTimer = random_float(4.0f, 9.0f);
        }
        break;
      case GAMEPLAY:
        // Food
        if (foodIsAvailable == false) {
          currentFood = create_new_food(player);
          foodIsAvailable = currentFood != NULL;
        }

        // Movement
        update_player_direction(player);
        if (framesCounter % player->framesPerStep == 0) {
          move_player(player);
          checkCollisions(player);
          if (player->collided) {
            StopSound(snakeSound);
            play_random_sound(&gameOverSoundCollection);
            currentGameScreen = GAMEOVER;
            break;
          }
        }

        // Catch the rat before it can move away from the head's new position.
        if (checkIfFoundFood(player, currentFood, &foodIsAvailable)) {
          currentFood = NULL;
          play_random_sound(&biteSoundCollection);
        }
        update_live_food(currentFood, player, GetFrameTime());

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
          play_random_sound(&levelUpSoundCollection);
        }
        break;

      case GAMEOVER:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_S)) {
          currentGameScreen = IsKeyPressed(KEY_S) ? START : GAMEPLAY;
          reset_player(player, startingPlayerReference);
          currentDifficultyLevel = 0;
          framesCounter = 0;
          free_food(currentFood);
          currentFood = NULL;
          foodIsAvailable = false;
          snakeSoundTimer = random_float(4.0f, 9.0f);
        }
    }

    // Wait 4-9 seconds between calls, counting only while the snake is quiet.
    if (currentGameScreen == GAMEPLAY && !IsSoundPlaying(snakeSound)) {
      snakeSoundTimer -= GetFrameTime();
      if (snakeSoundTimer <= 0.0f) {
        PlaySound(snakeSound);
        snakeSoundTimer = random_float(4.0f, 9.0f);
      }
    }

    // Draw
    BeginDrawing();

    // Check current screen and draw it
    switch (currentGameScreen) {
      case START:
        ClearBackground(RAYWHITE);
        DrawTextureEx(snakeLogo,
                      (Vector2){(screenWidth - snakeLogo.width * 1.5f) / 2, 0},
                      0.0f, 1.5f, WHITE);
        draw_snake_options(snakeOptions, selectedSnake);
        break;
      case GAMEPLAY:
        draw_gameplay_background();
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Score: %d", player->score), 10, 40, 20, DARKGRAY);
        // Draw snake
        draw_snake_head(player, snakeTextures.head, snakeTongue,
                        IsSoundPlaying(snakeSound));
        PlayerDirection previousBodyPartDirection =
            player->bodyPartsStates[player->numBodyParts - 1].direction;

        for (int body_part_idx = player->numBodyParts - 1; body_part_idx >= 0;
             body_part_idx--) {
          BodyPartState currentBodyPartState =
              player->bodyPartsStates[body_part_idx];

          if (body_part_idx == player->numBodyParts - 1) {
            draw_snake_bodypart(&currentBodyPartState, snakeTextures.tail,
                                currentBodyPartState.direction);
          } else if (previousBodyPartDirection == currentBodyPartState.direction) {
            draw_snake_bodypart(&currentBodyPartState,
                                snakeTextures.bodyStraight,
                                currentBodyPartState.direction);
          } else if ((previousBodyPartDirection == LEFT &&
                      currentBodyPartState.direction == UP) ||
                     (previousBodyPartDirection == DOWN &&
                      currentBodyPartState.direction == LEFT) ||
                     (previousBodyPartDirection == RIGHT &&
                      currentBodyPartState.direction == DOWN) ||
                     (previousBodyPartDirection == UP &&
                      currentBodyPartState.direction == RIGHT)) {
            draw_snake_bodypart(&currentBodyPartState,
                                snakeTextures.bodyClockwiseTurn,
                                previousBodyPartDirection);
          } else if ((previousBodyPartDirection == LEFT &&
                      currentBodyPartState.direction == DOWN) ||
                     (previousBodyPartDirection == UP &&
                      currentBodyPartState.direction == LEFT) ||
                     (previousBodyPartDirection == RIGHT &&
                      currentBodyPartState.direction == UP) ||
                     (previousBodyPartDirection == DOWN &&
                      currentBodyPartState.direction == RIGHT)) {
            draw_snake_bodypart(&currentBodyPartState,
                                snakeTextures.bodyCounterClockwiseTurn,
                                previousBodyPartDirection);
          }

          previousBodyPartDirection = currentBodyPartState.direction;
        }

        // Draw food
        draw_food(currentFood);

        break;

      case GAMEOVER:
        ClearBackground(RAYWHITE);
        DrawTextureEx(gameOverArt,
                      (Vector2){(screenWidth - gameOverArt.width * 2) / 2, 60},
                      0.0f, 2.0f, WHITE);
        DrawTextCentered(TextFormat("FINAL SCORE: %d", player->score),
                         screenHeight / 2 + 70, 20, DARKGRAY);
        DrawTextCentered("PRESS ENTER TO RESTART", screenHeight / 2 + 115, 20,
                         DARKGRAY);
        DrawTextCentered("PRESS S TO CHOOSE A SNAKE", screenHeight / 2 + 155,
                         10, DARKGRAY);
        break;
    }

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadTexture(snakeLogo);
  UnloadTexture(snakeTongue);
  UnloadTexture(gameOverArt);
  free_player(player);
  free_food(currentFood);

  // Unload textures
  for (size_t i = 0; i < sizeof(foodTypes) / sizeof(FoodType); i++) {
    UnloadTexture(foodTypes[i].texture);
  }
  for (int i = 0; i < SNAKE_OPTION_COUNT; i++) {
    UnloadTexture(snakeOptions[i].textures.head);
    UnloadTexture(snakeOptions[i].textures.tail);
    UnloadTexture(snakeOptions[i].textures.bodyStraight);
    UnloadTexture(snakeOptions[i].textures.bodyClockwiseTurn);
    UnloadTexture(snakeOptions[i].textures.bodyCounterClockwiseTurn);
  }
  CloseWindow();  // Release textures before closing the OpenGL context.

  // Unload sounds
  UnloadSound(snakeSound);
  unload_sound_collection(&biteSoundCollection);
  unload_sound_collection(&levelUpSoundCollection);
  unload_sound_collection(&gameOverSoundCollection);
  CloseAudioDevice();
  //--------------------------------------------------------------------------------------

  return 0;
}
