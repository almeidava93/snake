#include "raylib.h"

// CONSTANTS
const int screenWidth = 800;
const int screenHeight = 450;
const int targetFPS = 60;
const char* windowTitle = "Snake";
const Color BackgroundColorGameplay = {218, 233, 201, 255};

class Position {
 public:
  int x;
  int y;
};

class Size {
 public:
  float width;
  float height;
};

typedef enum PlayerDirection { LEFT, RIGHT, UP, DOWN } PlayerDirection;

class BodyPartState {
 public:
  int x;
  int y;
  PlayerDirection direction;
};

class Player {
 public:
  Position position;
  int stepSize;
  int framesPerStep;
  Size size;
  int lifes;
  int numBodyParts;
  BodyPartState bodyPartsStates[100];
  bool collided;
  PlayerDirection direction;
  int bodyPartsToAdd;
  int score;
};