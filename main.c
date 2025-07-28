#include "raylib.h"
#include "raymath.h"
#include <stddef.h>

#define TAU_OVER_3 2.0f * PI / 3.0f

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int screenWidth = 800;
int screenHeight = 800;
bool debug_mode = false;

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
// core_basic_windowUTILITIES

// INPUT
typedef struct {
  bool accelerate;
  bool turn_left;
  bool turn_right;
  bool fire;
  bool reset;
  bool toggle_debug;
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
  if (IsKeyPressed(KEY_R)) {
    user_input->reset = true;
  }
  if (IsKeyPressed(KEY_GRAVE)) {
    debug_mode = !debug_mode;
  }
}
// INPUT

#pragma region SHIP
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
  if (debug_mode) {
    DrawCircleLinesV(ship->position, ship->size, RED);
  }
}
#pragma endregion SHIP

// PROJECTILES
#define MAX_PROJECTILES 16
#define PROJECTILE_SPEED 500
#define PROJECTILE_TIME_TO_LIVE 1
#define PROJECTILE_RADIUS 3
#define PROJECTILE_DELAY 0.1
typedef struct {
  Vector2 position[MAX_PROJECTILES];
  Vector2 velocity[MAX_PROJECTILES];
  float time_to_live[MAX_PROJECTILES];
  double time_of_last_fire;
  size_t length;
} Projectiles;

void projectiles_spawn(Projectiles *projectiles, Vector2 position,
                       float heading, double timestamp) {
  if (projectiles->length >= MAX_PROJECTILES)
    return;
  if (timestamp < projectiles->time_of_last_fire + PROJECTILE_DELAY)
    return;
  size_t i = projectiles->length;
  projectiles->position[i] = position;
  projectiles->velocity[i] =
      Vector2Rotate((Vector2){PROJECTILE_SPEED, 0.0f}, heading);
  projectiles->time_to_live[i] = PROJECTILE_TIME_TO_LIVE;
  projectiles->length++;
  projectiles->time_of_last_fire = timestamp;
}

void projectiles_unordered_remove(Projectiles *projectiles, size_t at) {
  projectiles->position[at] = projectiles->position[projectiles->length - 1];
  projectiles->velocity[at] = projectiles->velocity[projectiles->length - 1];
  projectiles->time_to_live[at] =
      projectiles->time_to_live[projectiles->length - 1];
  projectiles->length--;
}

void projectiles_update(Projectiles *projectiles, float dt) {
  for (size_t i = 0; i < projectiles->length; i++) {
    projectiles->time_to_live[i] -= dt;
    if (projectiles->time_to_live[i] <= 0) {
      projectiles_unordered_remove(projectiles, i);
      i--;
    } else {
      projectiles->position[i] = Vector2Add(
          projectiles->position[i], Vector2Scale(projectiles->velocity[i], dt));
      projectiles->position[i] = Vector2Wrap(
          projectiles->position[i], Vector2Zero(), (Vector2){800.0f, 800.0f});
    }
  }
}

void projectiles_draw(Projectiles *projectiles) {
  for (size_t i = 0; i < projectiles->length; i++) {
    DrawCircleV(projectiles->position[i], PROJECTILE_RADIUS, WHITE);
  }
}
// PROJECTILES

// ASTEROIDS
typedef enum {
  AsteroidType_Large,
  AsteroidType_Medium,
  AsteroidType_Small
} AsteroidType;

float asteroid_get_radius(AsteroidType type) {
  switch (type) {
  case AsteroidType_Large:
    return 40;
  case AsteroidType_Medium:
    return 20;
  case AsteroidType_Small:
    return 10;
  }
}

float asteroid_get_speed(AsteroidType type) {
  switch (type) {
  case AsteroidType_Large:
    return 20;
  case AsteroidType_Medium:
    return 40;
  case AsteroidType_Small:
    return 80;
  }
}

#define STARTING_ASTEROIDS 24
#define MAX_ASTEROIDS STARTING_ASTEROIDS * 4
typedef struct {
  AsteroidType type[MAX_ASTEROIDS];
  Vector2 position[MAX_ASTEROIDS];
  Vector2 velocity[MAX_ASTEROIDS];
  size_t length;
} Asteroids;

void asteroids_spawn(Asteroids *asteroids, size_t at, Vector2 position,
                     AsteroidType type) {
  asteroids->type[at] = type;
  asteroids->position[at] = position;
  asteroids->position[at] = position;
  float asteroids_base_speed = asteroid_get_speed(type);
  float asteroid_speed =
      GetRandomValue(asteroids_base_speed * 0.5, asteroids_base_speed * 1.5);
  asteroids->velocity[at] = Vector2Rotate((Vector2){asteroid_speed, 0.0f},
                                          GetRandomValue(0.0f, 2.0f * PI));
}

void asteroids_init(Asteroids *asteroids) {
  asteroids->length = STARTING_ASTEROIDS;
  for (size_t i = 0; i < asteroids->length; i++) {
    Vector2 spawn_position =
        (Vector2){GetRandomValue(0, 800), GetRandomValue(0, 800)};
    asteroids_spawn(asteroids, i, spawn_position, AsteroidType_Large);
  }
}

void asteroids_unordered_remove(Asteroids *asteroids, size_t at) {
  asteroids->type[at] = asteroids->type[asteroids->length - 1];
  asteroids->position[at] = asteroids->position[asteroids->length - 1];
  asteroids->velocity[at] = asteroids->velocity[asteroids->length - 1];
  asteroids->length--;
}

void asteroids_update(Asteroids *asteroids, float dt) {
  for (size_t i = 0; i < asteroids->length; i++) {
    asteroids->position[i] = Vector2Add(
        asteroids->position[i], Vector2Scale(asteroids->velocity[i], dt));
    asteroids->position[i] =
        Vector2Wrap(asteroids->position[i], (Vector2){0.0f, 0.0f},
                    (Vector2){800.0f, 800.0f});
  }
}

void asteroids_draw(Asteroids *asteroids) {
  for (size_t i = 0; i < asteroids->length; i++) {
    DrawCircleLinesV(asteroids->position[i],
                     asteroid_get_radius(asteroids->type[i]), WHITE);
  }
}
// ASTEROIDS

// GAME
typedef enum {
  AppState_Menu,
  AppState_Game,
  AppState_GameOver,
} AppState;
AppState app_state = AppState_Menu;
UserInput user_input = {0};
Ship ship = {0};
Projectiles projectiles = {0};
Asteroids asteroids = {0};
unsigned int score = 0;

void reset(void) {
  ship_init(&ship);
  asteroids_init(&asteroids);
  projectiles.length = 0;
}

bool is_ship_hit(Ship *ship, Asteroids *asteroids) {
  for (size_t i = 0; i < asteroids->length; i++) {
    if (CheckCollisionCircles(ship->position, ship->size,
                              asteroids->position[i],
                              asteroid_get_radius(asteroids->type[i]))) {
      return true;
    }
  }
  return false;
}

void collision_projectiles_asteroids(Projectiles *projectiles,
                                     Asteroids *asteroids) {
  for (size_t ai = 0; ai < asteroids->length; ai++) {
    for (size_t pi = 0; pi < projectiles->length; pi++) {
      if (CheckCollisionCircles(asteroids->position[ai],
                                asteroid_get_radius(asteroids->type[ai]),
                                projectiles->position[pi], PROJECTILE_RADIUS)) {
        switch (asteroids->type[ai]) {
        case AsteroidType_Large: {
          asteroids_spawn(asteroids, asteroids->length, asteroids->position[ai],
                          AsteroidType_Medium);
          asteroids_spawn(asteroids, asteroids->length + 1,
                          asteroids->position[ai], AsteroidType_Medium);
          asteroids->length += 2;
        } break;
        case AsteroidType_Medium: {
          asteroids_spawn(asteroids, asteroids->length, asteroids->position[ai],
                          AsteroidType_Small);
          asteroids_spawn(asteroids, asteroids->length + 1,
                          asteroids->position[ai], AsteroidType_Small);
          asteroids->length += 2;
        } break;
        case AsteroidType_Small:
          break;
        }
        projectiles_unordered_remove(projectiles, pi);
        asteroids_unordered_remove(asteroids, ai);
        ai--;
        continue;
      }
    }
  }
}

int main(void) {
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
  switch (app_state) {
  case AppState_Menu: {
    if (IsKeyPressed(KEY_SPACE)) {
      app_state = AppState_Game;
    }
  } break;
  case AppState_Game: {
    user_input_update(&user_input);
    ship_update(&ship, &user_input, dt);
    if (user_input.fire) {
      projectiles_spawn(&projectiles, ship.position, ship.heading, GetTime());
    }
    projectiles_update(&projectiles, dt);
    asteroids_update(&asteroids, dt);
    if (is_ship_hit(&ship, &asteroids)) {
      reset();
    }
    collision_projectiles_asteroids(&projectiles, &asteroids);
  } break;
  case AppState_GameOver: {

  } break;
  }
  if (user_input.reset) {
    reset();
  }
  BeginDrawing();
  ClearBackground(BLACK);
  switch (app_state) {
  case AppState_Menu: {
    const char *title = "ASTEROIDS";
    int title_width = MeasureText(title, 48);
    DrawText(title, 400 - (title_width / 2), 200, 48, WHITE);
    const char *start = "PRESS SPACE TO START";
    int start_width = MeasureText(start, 24);
    DrawText(start, 400 - (start_width / 2), 400, 24, WHITE);
  } break;
  case AppState_Game: {
    ship_draw(&ship);
    projectiles_draw(&projectiles);
    asteroids_draw(&asteroids);
    DrawText(TextFormat("Score: %d", score), 680, 24, 24, WHITE);
  } break;
  case AppState_GameOver: {

  } break;
  }
  EndDrawing();
}
