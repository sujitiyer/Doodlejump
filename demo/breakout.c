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

// body IDs
const size_t BRICK_ID = 0;
const size_t BALL_ID = 1;
const size_t PLAYER_ID = 2;

// brick consts
const size_t NUM_BRICKS = 30;
const size_t BRICK_ROWS = 3;
const size_t BRICK_GAP = 5;
const vector_t BRICK_SIZE = {.x = 95, .y = 45};
const size_t BRICK_HEALTH = 2;

// player consts
const vector_t PLAYER_SIZE = {400, 30};
const size_t PLAYER_GAP = 8;
const size_t PLAYER_BODY_INDEX = 0;
const double PLAYER_SPEED = 800;

// ball consts
const size_t BALL_RADIUS = 6;
const size_t BALL_PTS = 20;
const vector_t BALL_INIT_VELO = {400, 400};
const size_t BALL_BODY_INDEX = 1;
const double VELO_SCALE_FACTOR = 1.8;

// color consts
const rgb_color_t RED = {1, 0, 0};
const rgb_color_t ORANGE = {1, 0.5, 0};
const rgb_color_t YELLOW = {1, 1, 0};
const rgb_color_t LIGHT_GREEN = {0, 1, 0};
const rgb_color_t GREEN = {0, 1, 0.5};
const rgb_color_t LIGHT_BLUE = {0, 1, 1};
const rgb_color_t BLUE = {0, 0.5, 1};
const rgb_color_t DARK_BLUE = {0, 0, 1};
const rgb_color_t PURPLE = {0.5, 0, 1};
const rgb_color_t PINK = {1, 0, 1};
const rgb_color_t GREY = {0.5, 0.5, 0.5};

// Border constants
const double BORDER_GAP = 5;
const size_t ON_SCREEN = 0;
const size_t LEFT_BORDER = 1;
const size_t RIGHT_BORDER = 2;
const size_t BOTTOM_BORDER = 3;
const size_t TOP_BORDER = 4;

// math consts
const double TWO_PI = M_PI * 2;

// physics consts
const double ELASTICITY = 0.5;

typedef struct state {
  scene_t *scene;
  size_t num_bricks;
} state_t;

typedef struct brick_info {
  size_t id;
  size_t health;
} brick_info_t;

list_t *generate_rect_shape(vector_t pos, vector_t dim) {
  list_t *shape = list_init(4, free);
  vector_t *v1 = malloc(sizeof(vector_t));
  vector_t *v2 = malloc(sizeof(vector_t));
  vector_t *v3 = malloc(sizeof(vector_t));
  vector_t *v4 = malloc(sizeof(vector_t));
  *v1 = (vector_t){.x = pos.x - dim.x / 2, .y = pos.y - dim.y / 2};
  *v2 = (vector_t){.x = pos.x - dim.x / 2, .y = pos.y + dim.y / 2};
  *v3 = (vector_t){.x = pos.x + dim.x / 2, .y = pos.y + dim.y / 2};
  *v4 = (vector_t){.x = pos.x + dim.x / 2, .y = pos.y - dim.y / 2};
  list_add(shape, v1);
  list_add(shape, v2);
  list_add(shape, v3);
  list_add(shape, v4);
  return shape;
}

body_t *generate_brick(vector_t pos, vector_t dim, rgb_color_t color) {
  brick_info_t *info = malloc(sizeof(brick_info_t));
  info->id = BRICK_ID;
  info->health = BRICK_HEALTH;
  list_t *shape = generate_rect_shape(pos, dim);
  body_t *rect = body_init_with_info(shape, INFINITY, color, info, free);
  return rect;
}

void generate_bricks(state_t *state) {
  size_t brick_cols = NUM_BRICKS / BRICK_ROWS;
  rgb_color_t colors[10] = {RED,        ORANGE, YELLOW,    LIGHT_GREEN, GREEN,
                            LIGHT_BLUE, BLUE,   DARK_BLUE, PURPLE,      PINK};
  for (size_t i = 0; i < brick_cols; i++) {
    rgb_color_t color = colors[i];
    for (size_t j = 0; j < BRICK_ROWS; j++) {
      vector_t brick_pos = (vector_t){
          .x = (i * (BRICK_SIZE.x + BRICK_GAP)) + BRICK_SIZE.x / 2 +
               BRICK_GAP / 2,
          .y = WINDOW.y - j * (BRICK_SIZE.y + BRICK_GAP) - BRICK_SIZE.y / 2};
      body_t *brick = generate_brick(brick_pos, BRICK_SIZE, color);
      scene_add_body(state->scene, brick);
    }
  }
}

void destroy_brick(body_t *ball, body_t *brick, vector_t axis, void *aux) {
  brick_info_t *auxillary = (brick_info_t *)body_get_info(brick);
  auxillary->health -= 1;
  body_set_color(brick, GREY);
  if (auxillary->health == 0) {
    body_remove(brick);
  }
}

void generate_player(state_t *state) {
  vector_t pos = {WINDOW.x / 2, PLAYER_SIZE.y / 2 + PLAYER_GAP};
  size_t *info = malloc(sizeof(size_t));
  *info = PLAYER_ID;
  list_t *shape = generate_rect_shape(pos, PLAYER_SIZE);
  body_t *body = body_init_with_info(shape, INFINITY, RED, info, free);
  scene_add_body(state->scene, body);
}

void generate_ball(state_t *state) {
  double curr_angle = 0;
  double offset_angle = TWO_PI / BALL_PTS;
  list_t *arc = list_init(BALL_PTS, free);
  for (size_t i = 0; i < BALL_PTS; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * BALL_RADIUS;
    vec->y = sin(curr_angle) * BALL_RADIUS;
    list_add(arc, vec);
    curr_angle += offset_angle;
  }
  size_t *info = malloc(sizeof(info));
  *info = BALL_ID;
  body_t *ball = body_init_with_info(arc, polygon_area(arc), RED, info, free);
  body_set_centroid(ball, (vector_t){CENTER.x, CENTER.y});
  body_set_velocity(ball, BALL_INIT_VELO);
  scene_add_body(state->scene, ball);
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

void on_key(char key, key_event_type_t type, double held_time, void *status) {
  state_t *state = (state_t *)status;
  body_t *player = scene_get_body(state->scene, PLAYER_BODY_INDEX);
  if (type == KEY_PRESSED) {
    switch (key) {
    case LEFT_ARROW:
      body_set_velocity(player, (vector_t){.x = -PLAYER_SPEED, .y = 0});
      break;
    case RIGHT_ARROW:
      body_set_velocity(player, (vector_t){.x = PLAYER_SPEED, .y = 0});
      break;
    default:
      break;
    }
  } else if (type == KEY_RELEASED) {
    body_set_velocity(player, VEC_ZERO);
  }
}

void scale_ball_velo(body_t *ball) {
  vector_t curr_velo = body_get_velocity(ball);
  double mag = vec_magnitude(curr_velo);
  double factor = vec_magnitude(BALL_INIT_VELO) / mag;
  vector_t new_velo = vec_multiply(factor, curr_velo);
  if (fabs(new_velo.x / new_velo.y) > VELO_SCALE_FACTOR) {
    new_velo = (vector_t){.x = new_velo.x / VELO_SCALE_FACTOR,
                          new_velo.y * VELO_SCALE_FACTOR};
  }
  body_set_velocity(ball, new_velo);
}

state_t *reset_game(state_t* state) {
  for(size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_remove(scene_get_body(state->scene, i));
  }
  scene_tick(state->scene, 1);
  state->scene = scene_init();
  state->num_bricks = NUM_BRICKS;

  generate_player(state);
  generate_ball(state);
  generate_bricks(state);

  body_t *player = list_get(scene_get_bodies(state->scene), PLAYER_BODY_INDEX);
  body_t *ball = list_get(scene_get_bodies(state->scene), BALL_BODY_INDEX);
  create_physics_collision(state->scene, ELASTICITY, ball, player);

  for (size_t i = 2; i <= state->num_bricks + 1; i++) {
    body_t *brick = scene_get_body(state->scene, i);
    create_physics_collision(state->scene, ELASTICITY, ball, brick);
    create_collision(state->scene, ball, brick, destroy_brick, state->scene,
                     NULL);
  }

  return state;
}

state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = scene_init();
  reset_game(state);
  return state;
}

void emscripten_main(state_t *state) {
  scene_t *scene = state->scene;
  double dt = time_since_last_tick();

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    size_t collision = check_off_screen(curr_body);

    switch (*(size_t *)body_get_info(curr_body)) {
    case PLAYER_ID:
      if (collision == LEFT_BORDER) {
        body_set_centroid(curr_body,
                          (vector_t){.x = PLAYER_SIZE.x / 2,
                                     .y = body_get_centroid(curr_body).y});
      } else if (collision == RIGHT_BORDER) {
        body_set_centroid(curr_body,
                          (vector_t){.x = WINDOW.x - PLAYER_SIZE.x / 2,
                                     .y = body_get_centroid(curr_body).y});
      }
      break;
    case BALL_ID:
      if (collision == LEFT_BORDER) {
        body_set_velocity(curr_body,
                          (vector_t){.x = -body_get_velocity(curr_body).x,
                                     .y = body_get_velocity(curr_body).y});
        body_set_centroid(
            curr_body,
            (vector_t){.x = body_get_centroid(curr_body).x + BORDER_GAP,
                       .y = body_get_centroid(curr_body).y});
      } else if (collision == RIGHT_BORDER) {
        body_set_velocity(curr_body,
                          (vector_t){.x = -body_get_velocity(curr_body).x,
                                     .y = body_get_velocity(curr_body).y});
        body_set_centroid(
            curr_body,
            (vector_t){.x = body_get_centroid(curr_body).x - BORDER_GAP,
                       .y = body_get_centroid(curr_body).y});
      } else if (collision == TOP_BORDER) {
        body_set_velocity(curr_body,
                          (vector_t){.x = body_get_velocity(curr_body).x,
                                     .y = -body_get_velocity(curr_body).y});
      } else if (collision == BOTTOM_BORDER) {
        reset_game(state);
        exit(0);
      }
      scale_ball_velo(curr_body);
    default:
      break;
    }
  }

  if (scene_bodies(scene) <= 2) {
    printf("You win!\n");
    exit(0);
  }

  sdl_on_key(on_key);
  scene_tick(scene, dt);
  sdl_render_scene(scene);
}

// frees the memory associated with everything
void emscripten_free(state_t *state) { scene_free(state->scene); }