#include "body.h"
#include "collision.h"
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
const vector_t WINDOW = (vector_t){.x = 1000, .y = 800};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};

// Enemy constants
const size_t NUM_ENEMIES = 15;
const size_t ENEMY_ROWS = 3;
const size_t GAP = 10;
const vector_t ENEMY_VELO = {80, 0};
const size_t ENEMY_ID = 0;
const rgb_color_t ENEMY_COLOR = {0.5, 0.5, 0.5};
const double ENEMY_ANGLE = 2.2;
const double ENEMY_RADIUS = 50;

// Hero constants
const rgb_color_t HERO_COLOR = {0, 1, 0};
const size_t HERO_BODY_INDEX = 0;
const double HERO_SPEED = 650;
const double HERO_ACCELERATION = 0;
const size_t HERO_ID = 1;
const double HERO_BOUNDARY = 20;
const double HERO_MAJOR_AXIS = 50;
const double HERO_MINOR_AXIS = 20;

// Drawing constants
const size_t CURVE_POINTS = 40;

// Border constants
const double BORDER_GAP = 25;
const size_t ON_SCREEN = 0;
const size_t LEFT_BORDER = 1;
const size_t RIGHT_BORDER = 2;
const size_t BOTTOM_BORDER = 3;
const size_t TOP_BORDER = 4;

// Pellet constants
const size_t HERO_PELLET_ID = 2;
const size_t ENEMY_PELLET_ID = 3;
const double FIRING_TIME = 1.0;
const vector_t PELLET_SPEED = {0, 800};
const vector_t PELLET_SIZE = {4, 16};
const size_t PELLET_POINTS = 4;

// Math constants
const double TWO_PI = 2 * M_PI;
const double PI = M_PI;

typedef struct state {
  scene_t *scene;
  size_t num_enemies;
  double time_since_last_fire;
} state_t;

// Function prototypes
void generate_hero(scene_t *scene);
body_t *generate_enemy(double radius, double center_x, double center_y,
                       double radians);
void generate_enemies(scene_t *scene);
void translate_enemy(state_t *state, body_t *body, size_t collision);
size_t check_off_screen(body_t *body);
void on_key(char key, key_event_type_t type, double held_time, void *present);
list_t *generate_pellet_shape(vector_t pos);
void generate_hero_pellet(state_t *state);
void generate_enemy_pellet(state_t *state);
state_t *emscripten_init();
void emscripten_main(state_t *state);
void emscripten_free(state_t *state);

void generate_hero(scene_t *scene) {
  list_t *oval = list_init(CURVE_POINTS, free);
  double curr_angle = 0;
  double offset_angle = TWO_PI / CURVE_POINTS;
  for (size_t i = 0; i < CURVE_POINTS; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * HERO_MAJOR_AXIS + WINDOW.x / 2;
    vec->y = sin(curr_angle) * HERO_MINOR_AXIS + GAP + HERO_MINOR_AXIS;
    list_add(oval, vec);
    curr_angle += offset_angle;
  }
  double mass = polygon_area(oval);
  size_t *id = malloc(sizeof(size_t));
  *id = HERO_ID;
  body_t *hero = body_init_with_info(oval, mass, HERO_COLOR, id, NULL);
  scene_add_body(scene, hero);
}

body_t *generate_enemy(double radius, double center_x, double center_y,
                       double radians) {
  double curr_angle = (PI - radians) / 2;
  double offset_angle = radians / CURVE_POINTS;
  list_t *shape = list_init(CURVE_POINTS, free);
  for (size_t i = 0; i < CURVE_POINTS; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * radius + center_x;
    vec->y = sin(curr_angle) * radius + center_y;
    list_add(shape, vec);
    curr_angle += offset_angle;
  }
  vector_t *center = malloc(sizeof(vector_t));
  center->x = center_x;
  center->y = center_y;
  list_add(shape, center);
  double mass = polygon_area(shape);
  size_t *id = malloc(sizeof(size_t));
  *id = ENEMY_ID;
  body_t *enemy = body_init_with_info(shape, mass, ENEMY_COLOR, id, NULL);
  return enemy;
}

void generate_enemies(scene_t *scene) {
  double col = NUM_ENEMIES / ENEMY_ROWS;
  for (size_t i = 0; i < col; i++) {
    for (size_t j = 0; j < ENEMY_ROWS; j++) {
      double x = ((i + 1) * ENEMY_RADIUS * 2) + (GAP * i);
      double y = (WINDOW.y - ENEMY_RADIUS) - (j * ENEMY_RADIUS) - (GAP * j);
      body_t *enemy = generate_enemy(ENEMY_RADIUS, x, y, ENEMY_ANGLE);
      body_set_velocity(enemy, ENEMY_VELO);
      scene_add_body(scene, enemy);
    }
  }
}

void translate_enemy(state_t *state, body_t *body, size_t collision) {
  vector_t old_centroid = body_get_centroid(body);
  vector_t new_centroid = old_centroid;
  if (collision == RIGHT_BORDER) {
    new_centroid =
        (vector_t){WINDOW.x - BORDER_GAP - ENEMY_RADIUS,
                   old_centroid.y - ENEMY_ROWS * (GAP + ENEMY_RADIUS)};
    body_set_centroid(body, new_centroid);
  } else if (collision == LEFT_BORDER) {
    new_centroid =
        (vector_t){BORDER_GAP + ENEMY_RADIUS,
                   old_centroid.y - ENEMY_ROWS * (GAP + ENEMY_RADIUS)};
    body_set_centroid(body, new_centroid);
  } else if (collision == BOTTOM_BORDER) {
    printf("You lose!\n");
    exit(0);
  }
}

size_t check_off_screen(body_t *body) {
  list_t *shape = body_get_shape(body);
  for (size_t i = 0; i < list_size(shape); i++) {
    vector_t vec = *(vector_t *)(list_get(shape, i));
    if (vec.x < BORDER_GAP) {
      return LEFT_BORDER;
    } else if (vec.x > WINDOW.x - BORDER_GAP) {
      return RIGHT_BORDER;
    } else if (vec.y < BORDER_GAP) {
      return BOTTOM_BORDER;
    } else if (vec.y > WINDOW.y - BORDER_GAP) {
      return TOP_BORDER;
    }
  }
  return ON_SCREEN;
}

list_t *generate_pellet_shape(vector_t pos) {
  list_t *shape = list_init(PELLET_POINTS, free);
  vector_t *v1 = malloc(sizeof(vector_t));
  vector_t *v2 = malloc(sizeof(vector_t));
  vector_t *v3 = malloc(sizeof(vector_t));
  vector_t *v4 = malloc(sizeof(vector_t));
  *v1 = (vector_t){.x = pos.x - PELLET_SIZE.x / 2,
                   .y = pos.y - PELLET_SIZE.y / 2};
  *v2 = (vector_t){.x = pos.x + PELLET_SIZE.x / 2,
                   .y = pos.y - PELLET_SIZE.y / 2};
  *v3 = (vector_t){.x = pos.x + PELLET_SIZE.x / 2,
                   .y = pos.y + PELLET_SIZE.y / 2};
  *v4 = (vector_t){.x = pos.x - PELLET_SIZE.x / 2,
                   .y = pos.y + PELLET_SIZE.y / 2};
  list_add(shape, v1);
  list_add(shape, v2);
  list_add(shape, v3);
  list_add(shape, v4);
  return shape;
}

void generate_hero_pellet(state_t *state) {
  vector_t pos =
      body_get_centroid(scene_get_body(state->scene, HERO_BODY_INDEX));
  list_t *shape = generate_pellet_shape(pos);
  double mass = polygon_area(shape);
  size_t *pellet_id = malloc(sizeof(size_t));
  *pellet_id = HERO_PELLET_ID;
  body_t *pellet =
      body_init_with_info(shape, mass, HERO_COLOR, pellet_id, NULL);
  scene_add_body(state->scene, pellet);
  body_set_velocity(pellet, PELLET_SPEED);

  for (size_t i = 1; i <= state->num_enemies; i++) {
    create_destructive_collision(state->scene, pellet,
                                 scene_get_body(state->scene, i));
  }
}

void generate_enemy_pellet(state_t *state) {
  size_t firing_idx = rand() % state->num_enemies + 1;
  body_t *firing_enemy = scene_get_body(state->scene, firing_idx);
  vector_t pos = body_get_centroid(firing_enemy);
  list_t *shape = generate_pellet_shape(pos);
  double mass = polygon_area(shape);
  size_t *pellet_id = malloc(sizeof(size_t));
  *pellet_id = ENEMY_PELLET_ID;
  body_t *pellet =
      body_init_with_info(shape, mass, ENEMY_COLOR, pellet_id, NULL);
  scene_add_body(state->scene, pellet);
  body_set_velocity(pellet, vec_negate(PELLET_SPEED));
}

void on_key(char key, key_event_type_t type, double held_time, void *present) {
  state_t *state = (state_t *)present;
  body_t *hero = scene_get_body(state->scene, HERO_BODY_INDEX);
  if (type == KEY_PRESSED) {
    double speed = HERO_SPEED + held_time * HERO_ACCELERATION;
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(hero, (vector_t){.x = -speed, .y = 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(hero, (vector_t){.x = speed, .y = 0});
      break;
    case ' ':
      generate_hero_pellet(state);
      break;
    default:
      break;
    }
  } else if (type == KEY_RELEASED && key != ' ') {
    body_set_velocity(hero, VEC_ZERO);
  }
}

state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = scene_init();
  state->num_enemies = NUM_ENEMIES;
  state->time_since_last_fire = 0;

  time_t t;
  srand((unsigned)time(&t));

  generate_hero(state->scene);
  generate_enemies(state->scene);

  return state;
}

void emscripten_main(state_t *state) {
  scene_t *scene = state->scene;
  double dt = time_since_last_tick();
  state->time_since_last_fire += dt;

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    size_t collision = check_off_screen(curr_body);

    switch (*(size_t *)body_get_info(curr_body)) {
    case HERO_ID:
      if (collision == LEFT_BORDER) {
        body_set_centroid(curr_body,
                          (vector_t){.x = HERO_MAJOR_AXIS + BORDER_GAP,
                                     .y = body_get_centroid(curr_body).y});
      } else if (collision == RIGHT_BORDER) {
        body_set_centroid(
            curr_body, (vector_t){.x = WINDOW.x - HERO_MAJOR_AXIS - BORDER_GAP,
                                  .y = body_get_centroid(curr_body).y});
      }
      break;
    case ENEMY_ID:
      if (state->time_since_last_fire > FIRING_TIME) {
        generate_enemy_pellet(state);
        state->time_since_last_fire = 0;
      }
      if (collision == LEFT_BORDER || collision == RIGHT_BORDER) {
        translate_enemy(state, curr_body, collision);
        vector_t velo = body_get_velocity(curr_body);
        body_set_velocity(curr_body, (vector_t){.x = -velo.x, .y = 0});
      }
      if (collision == BOTTOM_BORDER) {
        printf("You lose!\n");
        exit(0);
      }
      break;
    case HERO_PELLET_ID:
      if (collision == TOP_BORDER) {
        body_remove(curr_body);
      }
      break;
    case ENEMY_PELLET_ID:
      if (collision == BOTTOM_BORDER) {
        body_remove(curr_body);
      }
      body_t *hero = scene_get_body(scene, HERO_BODY_INDEX);
      if (find_collision(body_get_shape(curr_body), body_get_shape(hero)).collided) {
        printf("You lose!\n");
        exit(0);
      }
      break;
    default:
      break;
    }
  }

  size_t num_enemies = 0;
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    body_t *body = list_get(scene_get_bodies(scene), i);
    if (*(size_t *)(body_get_info(body)) == ENEMY_ID) {
      num_enemies++;
    }
  }
  state->num_enemies = num_enemies;

  if (state->num_enemies == 0) {
    printf("You win!\n");
    exit(0);
  }

  sdl_on_key(on_key);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// frees the memory associated with everything
void emscripten_free(state_t *state) { scene_free(state->scene); }
