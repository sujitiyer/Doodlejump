#include "list.h"
#include "polygon.h"
#include "queue.h"
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

// number of objects
const size_t NUM_STARS = 9;
const size_t MAX_NUM_PTS = 20;

// star properties constants
const float INNER_RADIUS_RATIO = 3;

// star starting point
const size_t BUFFER = 10.0;

// motion constants
const float TRANSLATION_SPEED = 300;
const float ROTATION_SPEED = 0.01;
const float GRAVITY_ACC = -1000;
const float RESTITUTION = 0.85;

// collision constants
const size_t NO_COLLISION = 0;
const size_t TOP_COLLISION = 1;
const size_t BOTTOM_COLLISION = 2;

// math constants
const double TWO_PI = 2.0 * M_PI;

// sprite definition (objects that can change and are relevant to the main loop)
typedef struct sprite {
  list_t *body;
  vector_t velocity;
  rgb_color_t color;
} sprite_t;

typedef struct state {
  queue_t *sprite_queue;
  sprite_t *most_recent_polygon;
  size_t most_recent_points;
} state_t;

list_t *generate_star(size_t outer_radius, size_t center_x, size_t center_y,
                      size_t num_points) {
  size_t num_verticies = 2 * num_points;
  double curr_angle = 0;
  double offset_angle = TWO_PI / num_verticies;
  double inner_radius = outer_radius / INNER_RADIUS_RATIO;
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

// returns border of the screen a sprite collides with
size_t check_collisions(sprite_t *sprite, double dt) {
  list_t *obj = sprite->body;
  vector_t velocity = sprite->velocity;
  for (size_t i = 0; i < list_size(obj); i++) {
    vector_t *curr_vec = list_get(obj, i);
    if (curr_vec->y + velocity.y * dt <= 0.0) {
      return BOTTOM_COLLISION;
    } else if (curr_vec->y + velocity.y * dt >= WINDOW.y) {
      return TOP_COLLISION;
    }
  }
  return NO_COLLISION;
}

bool polygon_off_screen(list_t *obj) {
  for (size_t i = 0; i < list_size(obj); i++) {
    vector_t *vec = list_get(obj, i);
    if (vec->x <= WINDOW.x) {
      return false;
    }
  }
  return true;
}

bool hit_peak(sprite_t *obj, double dt) {
  if (obj == NULL) {
    return true;
  }
  return obj->velocity.y >= 0 && obj->velocity.y + dt * GRAVITY_ACC < 0;
}

sprite_t *spawn_sprite(size_t num_points) {
  float outer_radius = WINDOW.x / (4 * NUM_STARS);
  vector_t start = (vector_t){.x = outer_radius + BUFFER,
                              .y = WINDOW.y - (outer_radius + BUFFER)};
  list_t *new_star = generate_star(outer_radius, start.x, start.y, num_points);

  sprite_t *new_sprite = malloc(sizeof(sprite_t));
  assert(new_sprite);
  new_sprite->body = new_star;
  new_sprite->velocity =
      (vector_t){.x = TRANSLATION_SPEED / 3, .y = -TRANSLATION_SPEED};

  // generate randomly colored star
  time_t t;
  srand((unsigned)time(&t));
  float red = (float)(rand() % 256) / 255;
  float green = (float)(rand() % 256) / 255;
  float blue = (float)(rand() % 256) / 255;
  new_sprite->color.r = red;
  new_sprite->color.g = green;
  new_sprite->color.b = blue;

  return new_sprite;
}

void free_sprite(sprite_t *sprite) {
  list_free(sprite->body);
  free(sprite);
}

// initializes the game
state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->sprite_queue = queue_init((void *)free_sprite);
  state->most_recent_points = 2;
  sprite_t *new_sprite = spawn_sprite(state->most_recent_points);
  queue_enqueue(state->sprite_queue, new_sprite);
  state->most_recent_polygon = new_sprite;

  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();

  for (int i = 0; i < queue_size(state->sprite_queue); i++) {
    sprite_t *curr_obj = (sprite_t *)queue_get(state->sprite_queue, i);
    list_t *curr_body = curr_obj->body;

    size_t wall = check_collisions(curr_obj, dt);
    if (wall == NO_COLLISION) {
      polygon_translate(curr_body, vec_multiply(dt, curr_obj->velocity));
      curr_obj->velocity.y += GRAVITY_ACC * dt;
      polygon_rotate(curr_body, ROTATION_SPEED, polygon_centroid(curr_body));
    } else if (wall == TOP_COLLISION || wall == BOTTOM_COLLISION) {
      curr_obj->velocity.y *= -1 * RESTITUTION;
    }

    // polygon_translate(curr_body, (vector_t){.x = 8, .y = 0});
    sdl_draw_polygon(curr_body, curr_obj->color);

    if (hit_peak(state->most_recent_polygon, dt)) {
      sprite_t *new_sprite = spawn_sprite(state->most_recent_points + 1);
      state->most_recent_points += 1;
      state->most_recent_polygon = new_sprite;
      queue_enqueue(state->sprite_queue, new_sprite);
      state->most_recent_polygon = new_sprite;
    }

    if (polygon_off_screen(curr_body)) {
      sprite_t *old_sprite = queue_dequeue(state->sprite_queue);
      free_sprite(old_sprite);
    }
  }
  sdl_show();
}

// frees the memory associated with everything
void emscripten_free(state_t *state) {
  queue_free(state->sprite_queue);
  free(state);
}
