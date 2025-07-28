/*******************************************************************************************
 *
 *   raylib [core] example - Basic window (adapted for HTML5 platform)
 *
 *   This example is prepared to compile for PLATFORM_WEB and PLATFORM_DESKTOP
 *   As you will notice, code structure is slightly different to the other
 *   examples... To compile it for PLATFORM_WEB just uncomment #define
 *PLATFORM_WEB at beginning
 *
 *   This example has been created using raylib 1.3 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h
 *   for details)
 *
 *   Copyright (c) 2015 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#define TAU_OVER_3 2.0f * PI / 3.0f

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int screenWidth = 800;
int screenHeight = 800;

void UpdateDrawFrame(void);

typedef struct {
  bool accelerate;
  bool turn_left;
  bool turn_right;
  bool fire;
} UserInput;

void user_input_update(UserInput *user_input) {
  *user_input = (UserInput){0};
  if (IsKeyDown(KEY_UP)) {
    user_input->accelerate = true;
  }
  if (IsKeyDown(KEY_LEFT)) {
    user_input->turn_left = true;
  }
  if (IsKeyDown(KEY_RIGHT)) {
    user_input->turn_right = true;
  }
  if (IsKeyPressed(KEY_SPACE)) {
    user_input->fire = true;
  }
}

typedef struct {
  Vector2 position;
  float heading;
  Vector2 velocity;
  float acceleration;
  float max_speed;
  float deceleration;
  float size;
  float turn_speed; // rad/s
} Ship;

void ship_init(Ship *ship) {
  ship->position = (Vector2){400.0f, 400.0f};
  ship->size = 15.0f;
  ship->acceleration = 500.0f;
  ship->max_speed = 1000.0f;
  ship->deceleration = 100.0f;
  ship->turn_speed = 2.0f * PI;
}

void ship_update(Ship *ship, UserInput *input, float dt) {
  if (input->turn_left) {
    ship->heading -= ship->turn_speed * dt;
  }
  if (input->turn_right) {
    ship->heading += ship->turn_speed * dt;
  }
  if (input->accelerate) {
    ship->velocity = Vector2Add(
        Vector2Rotate((Vector2){ship->acceleration * dt, 0.0f}, ship->heading),
        ship->velocity);
  }
  ship->position = Vector2Add(ship->position, Vector2Scale(ship->velocity, dt));
}

void ship_draw(Ship *ship) {
  Vector2 pt0 = Vector2Rotate((Vector2){ship->size, 0.0f}, ship->heading);
  Vector2 pt1 = Vector2Rotate(pt0, TAU_OVER_3);
  Vector2 pt2 = Vector2Rotate(pt1, TAU_OVER_3);
  DrawLineEx(Vector2Add(pt0, ship->position), Vector2Add(pt1, ship->position),
             2.0f, WHITE);
  DrawLineEx(Vector2Add(pt0, ship->position), Vector2Add(pt2, ship->position),
             2.0f, WHITE);
  DrawLineEx(Vector2Add(Vector2Scale(pt1, 0.5f), ship->position),
             Vector2Add(Vector2Scale(pt2, 0.5f), ship->position), 2.0f, WHITE);
  DrawCircleV(ship->position, 2.0f, RED);
}

// game state
UserInput user_input = {0};
Ship ship = {0};

int main() {
  InitWindow(screenWidth, screenHeight, "asteroids");
  ship_init(&ship);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }
#endif

  CloseWindow();
  return 0;
}

void UpdateDrawFrame(void) {
  float dt = GetFrameTime();
  user_input_update(&user_input);
  ship_update(&ship, &user_input, dt);
  BeginDrawing();
  ClearBackground(BLACK);
  ship_draw(&ship);
  EndDrawing();
}
