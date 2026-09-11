#include <assert.h>
#include <math.h>

#define main snake_game_main
#include "../main.c"
#undef main

static Player test_player(void) {
  return (Player){.position = {600, 300}, .size = {20, 20}};
}

static Food test_rat(PlayerDirection direction) {
  return (Food){.position = {100, 100}, .size = {20, 20},
                .type = {.isLive = true, .points = 6},
                .direction = direction, .turnTimer = 10};
}

int main(void) {
  srand(42);
  // A one-second update would cross an entire segment if only the endpoint
  // were checked. Every cardinal direction must stop at the near edge.
  Vector2 barriers[] = {{60, 100}, {140, 100}, {100, 60}, {100, 140}};
  Vector2 stops[] = {{80, 100}, {120, 100}, {100, 80}, {100, 120}};
  for (int direction = LEFT; direction <= DOWN; direction++) {
    Player player = test_player();
    player.numBodyParts = 1;
    player.bodyPartsStates[0] = (BodyPartState){barriers[direction].x,
                                               barriers[direction].y, LEFT};
    Food rat = test_rat(direction);
    update_live_food(&rat, &player, 1.0f);
    assert(rat.position.x == stops[direction].x);
    assert(rat.position.y == stops[direction].y);
  }

  // Head and screen edges also block rat movement.
  Player player = test_player();
  player.position = (Vector2){140, 100};
  Food rat = test_rat(RIGHT);
  update_live_food(&rat, &player, 1.0f);
  assert(rat.position.x == 120);
  player = test_player();
  Vector2 edges[] = {{0, 100}, {780, 100}, {100, 0}, {100, 430}};
  for (int direction = LEFT; direction <= DOWN; direction++) {
    rat = test_rat(direction);
    rat.position = edges[direction];
    update_live_food(&rat, &player, 1.0f);
    assert(rat.position.x == edges[direction].x);
    assert(rat.position.y == edges[direction].y);
  }

  // A fully enclosed rat stays put, including when it changes direction.
  player.numBodyParts = 4;
  player.bodyPartsStates[0] = (BodyPartState){80, 100, LEFT};
  player.bodyPartsStates[1] = (BodyPartState){120, 100, LEFT};
  player.bodyPartsStates[2] = (BodyPartState){100, 80, LEFT};
  player.bodyPartsStates[3] = (BodyPartState){100, 120, LEFT};
  rat = test_rat(LEFT);
  for (int i = 0; i < 1000; i++) update_live_food(&rat, &player, 0.02f);
  assert(rat.position.x == 100 && rat.position.y == 100);
  assert(rat.animationFrame == 0 && rat.animationTime == 0);

  // Movement is time-based; static fruit and game-over rats do not move.
  player = test_player();
  Food oneStep = test_rat(RIGHT), twoSteps = oneStep;
  update_live_food(&oneStep, &player, 0.1f);
  update_live_food(&twoSteps, &player, 0.05f);
  update_live_food(&twoSteps, &player, 0.05f);
  assert(fabsf(oneStep.position.x - 105.5f) < 0.001f);
  assert(fabsf(oneStep.position.x - twoSteps.position.x) < 0.001f);
  assert(oneStep.animationFrame == 1);
  assert(twoSteps.animationFrame == oneStep.animationFrame);
  update_live_food(&oneStep, &player, 0.11f);
  assert(oneStep.animationFrame == 2);
  update_live_food(&oneStep, &player, 0.1f);
  assert(oneStep.animationFrame == 3);
  update_live_food(&oneStep, &player, 0.1f);
  assert(oneStep.animationFrame == 0);
  rat = test_rat(RIGHT);
  rat.type.isLive = false;
  update_live_food(&rat, &player, 1);
  assert(rat.position.x == 100);
  rat.type.isLive = true;
  player.collided = true;
  update_live_food(&rat, &player, 1);
  assert(rat.position.x == 100);
  update_live_food(NULL, &player, 1);

  // All food spawns clear of the snake; rats are sampled alongside fruit.
  player = test_player();
  player.numBodyParts = 10;
  for (int i = 0; i < 10; i++)
    player.bodyPartsStates[i] = (BodyPartState){100 + i * 20, 100, LEFT};
  int rats = 0;
  for (int i = 0; i < 10000; i++) {
    Food* food = create_new_food(&player);
    assert(food != NULL);
    Rectangle bounds = {food->position.x, food->position.y, 20, 20};
    assert(!food_position_blocked(bounds, &player));
    rats += food->type.isLive;
    free_food(food);
  }
  assert(rats > 1000 && rats < 2000);

  // Eating a rat awards points and growth once; the caller clears the pointer.
  Food* caught = malloc(sizeof(Food));
  *caught = test_rat(LEFT);
  caught->position = player.position;
  bool available = true;
  assert(checkIfFoundFood(&player, caught, &available));
  caught = NULL;
  assert(!available && player.score == 6 && player.bodyPartsToAdd == 1);
  assert(!checkIfFoundFood(&player, caught, &available));
  puts("Live food tests passed.");
  return 0;
}
