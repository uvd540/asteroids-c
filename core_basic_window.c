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
#include <stddef.h>

#define TAU_OVER_3 2.0f * PI / 3.0f

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int screenWidth = 800;
int screenHeight = 800;

void UpdateDrawFrame(void);

// UTILITIES
Vector2 Vector2Wrap(Vector2 v, Vector2 min, Vector2 max) {
  Vector2 result = v;
  if (v.x < min.x) {
    result.x = max.x;
  }
  if (v.x > max.x) {
    result.x = min.x;
  }
  if (v.y < min.y) {
    result.y = max.y;
  }
  if (v.y > max.y) {
    result.y = min.y;
  }
  return result;
}
// UTILITIES

// INPUT
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
  if (IsKeyDown(KEY_SPACE)) {
    user_input->fire = true;
  }
}
// INPUT

// SHIP
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
  ship->max_speed = 500.0f;
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
  ship->velocity = Vector2MoveTowards(ship->velocity, Vector2Zero(),
                                      ship->deceleration * dt);
  ship->velocity = Vector2ClampValue(ship->velocity, 0.0f, ship->max_speed);
  ship->position = Vector2Add(ship->position, Vector2Scale(ship->velocity, dt));
  ship->position =
      Vector2Wrap(ship->position, Vector2Zero(), (Vector2){800.0f, 800.0f});
  // TODO: Figure out why movement is jittery
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
  // draw hitbox
  // DrawCircleLinesV(ship->position, ship->size, RED);
}
// SHIP

// PROJECTILES
#define PROJECTILE_SPEED 500
#define PROJECTILE_TIME_TO_LIVE 1
#define PROJECTILE_RADIUS 3
typedef struct {
  Vector2 position;
  Vector2 velocity;
  float time_to_live;
} Projectile;

#define MAX_PROJECTILES 100
#define PROJECTILE_DELAY 0.1
typedef struct {
  Projectile items[MAX_PROJECTILES];
  size_t length;
  double time_of_last_fire;
} Projectiles;

void projectiles_spawn(Projectiles *projectiles, Vector2 position,
                       float heading, double timestamp) {
  if (projectiles->length >= MAX_PROJECTILES)
    return;
  if (timestamp < projectiles->time_of_last_fire + PROJECTILE_DELAY)
    return;
  projectiles->items[projectiles->length].position = position;
  projectiles->items[projectiles->length].velocity =
      Vector2Rotate((Vector2){PROJECTILE_SPEED, 0.0f}, heading);
  projectiles->items[projectiles->length].time_to_live =
      PROJECTILE_TIME_TO_LIVE;
  projectiles->length++;
  projectiles->time_of_last_fire = timestamp;
}

void projectiles_update(Projectiles *projectiles, float dt) {
  for (size_t i = 0; i < projectiles->length; i++) {
    Projectile *p = &projectiles->items[i];
    p->time_to_live -= dt;
    if (p->time_to_live <= 0) {
      projectiles->items[i] = projectiles->items[projectiles->length - 1];
      projectiles->length--;
      i--;
    } else {
      p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
      p->position =
          Vector2Wrap(p->position, Vector2Zero(), (Vector2){800.0f, 800.0f});
    }
  }
}

void projectiles_draw(Projectiles *projectiles) {
  for (size_t i = 0; i < projectiles->length; i++) {
    DrawCircleV(projectiles->items[i].position, PROJECTILE_RADIUS, WHITE);
  }
}
// PROJECTILES

// ASTEROIDS
typedef enum { Large, Medium, Small } AsteroidType;

float asteroid_get_radius(AsteroidType type) {
  switch (type) {
  case Large:
    return 40;
  case Medium:
    return 20;
  case Small:
    return 10;
  }
}

float asteroid_get_speed(AsteroidType type) {
  switch (type) {
  case Large:
    return 20;
  case Medium:
    return 40;
  case Small:
    return 80;
  }
}

typedef struct {
  AsteroidType type;
  Vector2 position;
  Vector2 velocity;
} Asteroid;

#define STARTING_ASTEROIDS 24
#define MAX_ASTEROIDS STARTING_ASTEROIDS * 4
typedef struct {
  Asteroid items[MAX_ASTEROIDS];
  size_t length;
} Asteroids;

void asteroids_init(Asteroids *asteroids) {
  asteroids->length = STARTING_ASTEROIDS;
  for (size_t i = 0; i < asteroids->length; i++) {
    Asteroid *a = &asteroids->items[i];
    a->position.x = GetRandomValue(0, 800);
    a->position.y = GetRandomValue(0, 800);
    float asteroids_base_speed = asteroid_get_speed(a->type);
    float asteroid_speed =
        GetRandomValue(asteroids_base_speed * 0.5, asteroids_base_speed * 1.5);
    a->velocity = Vector2Rotate((Vector2){asteroid_speed, 0.0f},
                                GetRandomValue(0.0f, 2.0f * PI));
  }
}

void asteroids_update(Asteroids *asteroids, float dt) {
  for (size_t i = 0; i < asteroids->length; i++) {
    Asteroid *a = &asteroids->items[i];
    a->position = Vector2Add(a->position, Vector2Scale(a->velocity, dt));
    a->position = Vector2Wrap(a->position, (Vector2){0.0f, 0.0f},
                              (Vector2){800.0f, 800.0f});
  }
}

void asteroids_draw(Asteroids *asteroids) {
  for (size_t i = 0; i < asteroids->length; i++) {
    Asteroid *a = &asteroids->items[i];
    DrawCircleLinesV(a->position, asteroid_get_radius(a->type), WHITE);
  }
}
// ASTEROIDS

// game state
UserInput user_input = {0};
Ship ship = {0};
Projectiles projectiles = {0};
Asteroids asteroids = {0};

int main() {
  InitWindow(screenWidth, screenHeight, "asteroids");
  ship_init(&ship);
  asteroids_init(&asteroids);

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
  if (user_input.fire) {
    projectiles_spawn(&projectiles, ship.position, ship.heading, GetTime());
  }
  projectiles_update(&projectiles, dt);
  asteroids_update(&asteroids, dt);
  BeginDrawing();
  ClearBackground(BLACK);
  ship_draw(&ship);
  projectiles_draw(&projectiles);
  asteroids_draw(&asteroids);
  EndDrawing();
}
