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

// circle constants
const size_t CIRCLE_POINTS = 20;
const size_t CIRCLE_RADIUS = 10; // = WINDOW.x/(2*NUM_CIRCLES)
const size_t NUM_CIRCLES = 50;
const size_t SPRING_FORCE_CIRCLE_INDEX1 = 0;
const size_t SPRING_FORCE_CIRCLE_INDEX2 = 25;
const size_t SPRING_FORCE_CIRCLE_INDEX3 = 50;

// spawning constants
const vector_t MIN_BOUNDS = (vector_t){.x = 10, .y = 10};
const vector_t MAX_BOUNDS = (vector_t){.x = 990, .y = 490};

// physics constants
double SPRING_CONST = 500;
double DRAG_CONST = 50;

// math constants
const double TWO_PI = 2 * M_PI;

// curve initialization constants
const size_t AMPLITUDE = 120;
const size_t CHANGE_X = 20;
const double FREQUENCY = M_PI / 25;
const size_t VERT_SHIFT = 370;

typedef struct state {
  scene_t *scene;
} state_t;

list_t *generate_circle(size_t radius, double center_x, double center_y,
                        size_t num_points) {
  double curr_angle = 0;
  double vert_angle = TWO_PI / num_points;
  list_t *circle = list_init(num_points, NULL);
  for (size_t i = 0; i < num_points; i++) {
    vector_t *vec_ptr = malloc(sizeof(vector_t));
    vec_ptr->x = cos(curr_angle) * radius + center_x;
    vec_ptr->y = sin(curr_angle) * radius + center_y;
    list_add(circle, vec_ptr);
    curr_angle += vert_angle;
  }
  return circle;
}

void generate_circles(scene_t *scene, size_t num_circles) {
  double center_x;
  double center_y;
  for (size_t i = 1; i <= num_circles; i++) {
    center_x = MIN_BOUNDS.x + CHANGE_X * (i - 1);
    // First half of points follow cosine curve
    if (i <= num_circles / 2) {
      center_y = AMPLITUDE * cos(FREQUENCY * i) + VERT_SHIFT;
    }
    // Second half of points in straight line
    else {
      center_y = CENTER.y;
    }
    list_t *shape =
        generate_circle(CIRCLE_RADIUS, center_x, center_y, CIRCLE_POINTS);
    double mass = polygon_area(shape);
    rgb_color_t color = {(float)(rand() % 256) / 255,
                         (float)(rand() % 256) / 255,
                         (float)(rand() % 256) / 255};
    body_t *circle = body_init(shape, mass, color);
    scene_add_body(scene, circle);
  }
  for (size_t i = 0; i < num_circles / 2 - 1; i++) {
    center_x = MIN_BOUNDS.x + CHANGE_X * (i);
    center_y = CENTER.y;
    list_t *shape =
        generate_circle(CIRCLE_RADIUS, center_x, center_y, CIRCLE_POINTS);
    double mass = 1000000 * polygon_area(shape);
    rgb_color_t color = {0, 0, 0};
    body_t *circle = body_init(shape, mass, color);
    scene_add_body(scene, circle);
  }
}

void apply_spring_constant(scene_t *scene) {
  // Create a spring force between every circle on cosine curve and the
  // invisible black circle below it
  for (size_t i = 0; i < SPRING_FORCE_CIRCLE_INDEX2 - 1; i++) {
    list_t *bodies = list_init(2, body_free);
    list_add(bodies, scene_get_body(scene, SPRING_FORCE_CIRCLE_INDEX3 + i));
    list_add(bodies, scene_get_body(scene, i));
    create_spring(scene, SPRING_CONST, bodies);
    // create_spring(scene, SPRING_CONST, scene_get_body(scene,
    // SPRING_FORCE_CIRCLE_INDEX3 + i), scene_get_body(scene, i));
  }
}

void apply_drag_constant(scene_t *scene) {
  for (size_t i = 0; i < SPRING_FORCE_CIRCLE_INDEX2; i++) {
    create_drag_old(scene, DRAG_CONST, scene_get_body(scene, i));
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

  generate_circles(state->scene, NUM_CIRCLES);
  apply_spring_constant(state->scene);
  list_t *bodies = list_init(25, body_free);
  for (size_t i = 0; i < SPRING_FORCE_CIRCLE_INDEX2; i++) {
    list_add(bodies, scene_get_body(state->scene, i));
  }
  create_drag(state->scene, DRAG_CONST, bodies);
  //   apply_drag_constant(state->scene);

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
