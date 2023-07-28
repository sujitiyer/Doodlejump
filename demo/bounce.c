#include "list.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

// color constants
const rgb_color_t color = {.r = 0, .g = 1, .b = 1};

// star properties constants
const size_t NUM_VERTICES = 10;
const size_t NUM_STAR_PTS = 5;
const float INNER_RADIUS_RATIO = 3;
const float STAR_OUTER_RAD = 100;

// motion constants
const float TRANSLATION_SPEED = 100;
const float ROTATION_SPEED = 0.01;

// collision constants
const size_t NO_COLLISION = 0;
const size_t TOP_COLLISION = 1;
const size_t RIGHT_COLLISION = 2;
const size_t BOTTOM_COLLISION = 3;
const size_t LEFT_COLLISION = 4;

// math constants
const double TWO_PI = 2.0 * M_PI;

list_t *generate_5pt_star(size_t outer_radius, size_t center_x,
                          size_t center_y) {
  double curr_angle = 0;
  double offset_angle = TWO_PI / NUM_VERTICES;
  double inner_radius = outer_radius / INNER_RADIUS_RATIO;
  list_t *star = list_init(NUM_VERTICES, NULL);
  for (size_t i = 0; i < NUM_STAR_PTS; i++) {
    vector_t *outer_point = malloc(sizeof(vector_t));
    assert(outer_point);
    outer_point->x = cos(curr_angle) * outer_radius + center_x;
    outer_point->y = sin(curr_angle) * outer_radius + center_y;
    list_add(star, outer_point);
    curr_angle += offset_angle;
    vector_t *inner_point = malloc(sizeof(vector_t));
    assert(inner_point);
    inner_point->x = cos(curr_angle) * inner_radius + center_x;
    inner_point->y = sin(curr_angle) * inner_radius + center_y;
    list_add(star, inner_point);
    curr_angle += offset_angle;
  }
  return star;
}

// returns a number corresponding to the wall the star will hit 1-4 or 0
// otherwise

// state definition (things that can change and are relevant to the main loop)
typedef struct state {
  list_t *star;
  vector_t *velocity;
} state_t;

// initializes the state
state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->star = generate_5pt_star(STAR_OUTER_RAD, CENTER.x, CENTER.y);
  vector_t *velocity = malloc(sizeof(vector_t));
  assert(velocity);
  state->velocity = velocity;
  state->velocity->x = TRANSLATION_SPEED;
  state->velocity->y = TRANSLATION_SPEED;
  return state;
}

size_t check_collisions(state_t *state, double dt) {
  list_t *star = state->star;
  vector_t *velocity = state->velocity;
  for (size_t i = 0; i < list_size(star); i++) {
    if (((vector_t *)list_get(star, i))->y + velocity->y * dt <= 0.0) {
      return TOP_COLLISION;
    } else if (((vector_t *)list_get(star, i))->x + velocity->x * dt >=
               WINDOW.x) {
      return RIGHT_COLLISION;
    } else if (((vector_t *)list_get(star, i))->y + velocity->y * dt >=
               WINDOW.y) {
      return BOTTOM_COLLISION;
    } else if (((vector_t *)list_get(star, i))->x + velocity->x * dt <= 0.0) {
      return LEFT_COLLISION;
    }
  }
  return NO_COLLISION;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  list_t *star = state->star;
  vector_t *velocity = state->velocity;
  double dt = time_since_last_tick();
  size_t wall = check_collisions(state, dt);
  if (wall == NO_COLLISION) {
    polygon_translate(star, vec_multiply(dt, *velocity));
  } else if (wall == TOP_COLLISION || wall == BOTTOM_COLLISION) {
    velocity->y *= -1;
  } else if (wall == RIGHT_COLLISION || wall == LEFT_COLLISION) {
    velocity->x *= -1;
  }
  polygon_rotate(star, ROTATION_SPEED, polygon_centroid(star));
  sdl_draw_polygon(star, color);
  sdl_show();
}

void emscripten_free(state_t *state) {
  list_t *star = state->star;
  list_free(star);
  free(state);
}
