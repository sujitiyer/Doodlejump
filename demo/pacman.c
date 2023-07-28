#include "body.h"
#include "color.h"
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

// pacman constants
const size_t PACMAN_POINTS = 60;
const double PACMAN_MOUTH_ANGLE = M_PI / 2;
const double PACMAN_RADIUS = 45;
const rgb_color_t PACMAN_COLOR = {.r = 1, .g = 1, .b = 0};
const double PACMAN_SPEED = 150;
const double PACMAN_ACCELERATION = 200;
const double PACMAN_RIGHT_ANGLE = M_PI / 4;
const double PACMAN_UP_ANGLE = 3 * M_PI / 4;
const double PACMAN_LEFT_ANGLE = 5 * M_PI / 4;
const double PACMAN_DOWN_ANGLE = 7 * M_PI / 4;
const size_t PACMAN_BODY_INDEX = 0;

// pellet constants
const size_t PELLET_POINTS = 20;
const size_t PELLET_RADIUS = 8;
const rgb_color_t PELLET_COLOR = {.r = 1, .g = 0.5, .b = 0};
const size_t NUM_PELLETS = 30;
const size_t PELLET_BODY_INDEX = 1;

// collision constants
const size_t NO_BOUNDARY = 0;
const size_t LEFT_BOUNDARY = 1;
const size_t RIGHT_BOUNDARY = 2;
const size_t TOP_BOUNDARY = 3;
const size_t BOTTOM_BOUNDARY = 4;
const double BUFFER = 15;

// math constants
const double TWO_PI = 2 * M_PI;

typedef struct state {
  scene_t *scene;
} state_t;

list_t *generate_arc(size_t radius, double radians, size_t num_points, size_t x,
                     size_t y) {
  double curr_angle = 0;
  double offset_angle = radians / num_points;
  list_t *arc = list_init(num_points, free);
  for (size_t i = 0; i < num_points; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * radius + x;
    vec->y = sin(curr_angle) * radius + y;
    list_add(arc, vec);
    curr_angle += offset_angle;
  }
  return arc;
}

body_t *generate_pacman(size_t center_x, size_t center_y) {
  list_t *shape = generate_arc(PACMAN_RADIUS, TWO_PI - PACMAN_MOUTH_ANGLE,
                               PACMAN_POINTS, CENTER.x, CENTER.y);
  vector_t *center_point = malloc(sizeof(vector_t));
  assert(center_point);
  center_point->x = center_x;
  center_point->y = center_y;
  list_add(shape, center_point);
  double mass = polygon_area(shape);
  body_t *pacman = body_init(shape, mass, PACMAN_COLOR);
  return pacman;
}

body_t *generate_pellet(size_t x, size_t y) {
  list_t *shape = generate_arc(PELLET_RADIUS, TWO_PI, PELLET_POINTS, x, y);
  double mass = polygon_area(shape);
  body_t *pellet = body_init(shape, mass, PELLET_COLOR);
  return pellet;
}

vector_t get_random_pellet_coordinate() {
  double x = fabs((double)(rand() % (size_t)(WINDOW.x)) - PELLET_RADIUS);
  double y = fabs((double)(rand() % (size_t)(WINDOW.y)) - PELLET_RADIUS);
  return (vector_t){x, y};
}

size_t check_off_screen(body_t *body) {
  list_t *polygon = body_get_shape(body);
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t vec = *(vector_t *)(list_get(polygon, i));
    if (vec.x >= 0 && vec.x <= WINDOW.x && vec.y >= 0 && vec.y <= WINDOW.y) {
      return NO_BOUNDARY;
    }
  }
  vector_t velo = body_get_velocity(body);
  if (velo.x < 0) {
    return LEFT_BOUNDARY;
  } else if (velo.x > 0) {
    return RIGHT_BOUNDARY;
  } else if (velo.y < 0) {
    return TOP_BOUNDARY;
  } else {
    return BOTTOM_BOUNDARY;
  }
  list_free(polygon);
}

bool pellet_collision(body_t *pacman, body_t *pellet) {
  vector_t pacman_pos = body_get_centroid(pacman);
  vector_t pellet_pos = body_get_centroid(pellet);
  double dist = sqrt(pow(pacman_pos.x - pellet_pos.x, 2) +
                     pow(pacman_pos.y - pellet_pos.y, 2));
  return dist <= PACMAN_RADIUS;
}

void on_key(char key, key_event_type_t type, double held_time, void *present) {
  state_t *state = (state_t *)present;
  body_t *pacman = scene_get_body(state->scene, 0);
  if (type == KEY_PRESSED) {
    double speed = PACMAN_SPEED + (held_time * PACMAN_ACCELERATION);
    switch (key) {
    case UP_ARROW:
      body_set_velocity(pacman, (vector_t){.x = 0, .y = speed});
      body_set_rotation(pacman, PACMAN_UP_ANGLE);
      break;
    case DOWN_ARROW:
      body_set_velocity(pacman, (vector_t){.x = 0, .y = -speed});
      body_set_rotation(pacman, PACMAN_DOWN_ANGLE);
      break;
    case LEFT_ARROW:
      body_set_velocity(pacman, (vector_t){.x = -speed, .y = 0});
      body_set_rotation(pacman, PACMAN_LEFT_ANGLE);
      break;
    case RIGHT_ARROW:
      body_set_velocity(pacman, (vector_t){.x = speed, .y = 0});
      body_set_rotation(pacman, PACMAN_RIGHT_ANGLE);
      break;
    default:
      break;
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

  body_t *pacman = generate_pacman(CENTER.x, CENTER.y);
  body_set_rotation(pacman, PACMAN_RIGHT_ANGLE);
  scene_add_body(state->scene, pacman);

  time_t t;
  srand((unsigned)time(&t));
  for (size_t i = 0; i < NUM_PELLETS; i++) {
    vector_t pos = get_random_pellet_coordinate();
    scene_add_body(state->scene, generate_pellet(pos.x, pos.y));
  }
  return state;
}

void emscripten_main(state_t *state) {
  scene_t *scene = state->scene;
  body_t *pacman = scene_get_body(scene, PACMAN_BODY_INDEX);
  double dt = time_since_last_tick();

  size_t boundary_detection = check_off_screen(pacman);
  if (boundary_detection == LEFT_BOUNDARY) {
    body_set_centroid(pacman, (vector_t){.x = WINDOW.x + PACMAN_RADIUS - BUFFER,
                                         .y = body_get_centroid(pacman).y});
  } else if (boundary_detection == RIGHT_BOUNDARY) {
    body_set_centroid(pacman, (vector_t){.x = -PACMAN_RADIUS + BUFFER,
                                         .y = body_get_centroid(pacman).y});

  } else if (boundary_detection == TOP_BOUNDARY) {
    body_set_centroid(pacman,
                      (vector_t){.x = body_get_centroid(pacman).x,
                                 .y = WINDOW.y + PACMAN_RADIUS - BUFFER});
  } else if (boundary_detection == BOTTOM_BOUNDARY) {
    body_set_centroid(pacman, (vector_t){.x = body_get_centroid(pacman).x,
                                         .y = -PACMAN_RADIUS + BUFFER});
  }

  for (size_t i = PELLET_BODY_INDEX; i <= NUM_PELLETS; i++) {
    body_t *pellet = scene_get_body(scene, i);
    if (pellet_collision(pacman, pellet)) {
      scene_remove_body(scene, i);
      vector_t pos = get_random_pellet_coordinate();
      scene_add_body(state->scene, generate_pellet(pos.x, pos.y));
    }
  }
  sdl_on_key(on_key);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// frees the memory associated with everything
void emscripten_free(state_t *state) { scene_free(state->scene); }