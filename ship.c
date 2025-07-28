#include "raylib.h"

typedef struct {
  Vector2 position;
  float heading;
  Vector2 velocity;
  float size;
} Ship;

void ship_init(Ship *ship) {
  ship->position = (Vector2){400.0f, 400.0f};
  ship->size = 15.0f;
}

void ship_draw(Ship *ship) { DrawCircleV(ship->position, ship->size, WHITE); }
