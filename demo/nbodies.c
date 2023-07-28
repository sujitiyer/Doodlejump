#include "body.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// window constants
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

// objects on screen constants
const size_t NUM_STARS = 60;

// star constants
const size_t STAR_PTS = 4;
const size_t STAR_MIN_SIZE = 8;
const size_t STAR_MAX_SIZE = 20;
const float STAR_SHAPE_RATIO = 3;

// spawning constants
const vector_t MIN_BOUNDS = (vector_t){.x = 200, .y = 100};
const vector_t MAX_BOUNDS = (vector_t){.x = 800, .y = 400};

// physics constants
double GRAVITY_CONST = 50;

// math constants
const double TWO_PI = 2 * M_PI;

typedef struct state {
  scene_t *scene;
} state_t;

list_t *generate_star(size_t outer_radius, size_t center_x, size_t center_y,
                      size_t num_points) {
  size_t num_verticies = 2 * num_points;
  double curr_angle = 0;
  double offset_angle = TWO_PI / num_verticies;
  double inner_radius = outer_radius / STAR_SHAPE_RATIO;
  list_t *star = (list_t *)list_init(num_verticies, free);
  for (size_t i = 0; i < num_points; i++) {
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

void generate_random_stars(scene_t *scene, size_t num_stars) {
  for (size_t i = 0; i < num_stars; i++) {
    float radius =
        (float)(rand() % (STAR_MAX_SIZE - STAR_MIN_SIZE)) + STAR_MIN_SIZE;
    double x =
        (float)(rand() % (size_t)(MAX_BOUNDS.x - MIN_BOUNDS.x)) + MIN_BOUNDS.x;
    double y =
        (float)(rand() % (size_t)(MAX_BOUNDS.y - MIN_BOUNDS.y)) + MIN_BOUNDS.y;
    list_t *shape = generate_star(radius, x, y, STAR_PTS);
    double mass = polygon_area(shape);
    rgb_color_t color = {(float)(rand() % 256) / 255,
                         (float)(rand() % 256) / 255,
                         (float)(rand() % 256) / 255};
    body_t *star = body_init(shape, mass, color);
    scene_add_body(scene, star);
  }
}

void apply_gravity(scene_t *scene, double G) {
  size_t num_bodies = scene_bodies(scene);
  for (size_t i = 0; i < num_bodies - 1; i++) {
    for (size_t j = i + 1; j < num_bodies; j++) {
      create_newtonian_gravity_old(scene, G, scene_get_body(scene, i),
                                   scene_get_body(scene, j));
    }
  }
}

state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = scene_init();

  time_t t;
  srand((unsigned)time(&t));

  generate_random_stars(state->scene, NUM_STARS);
  apply_gravity(state->scene, GRAVITY_CONST);

  return state;
}

void emscripten_main(state_t *state) {
  scene_t *scene = state->scene;
  double dt = time_since_last_tick();
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// frees the memory associated with everything
void emscripten_free(state_t *state) { scene_free(state->scene); }
