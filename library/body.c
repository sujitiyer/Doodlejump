#include "color.h"
#include "list.h"
#include "image.h"
#include "polygon.h"
#include "vector.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct body {
  list_t *shape;
  double mass;
  double angle;
  vector_t velocity;
  vector_t acceleration;
  vector_t force;
  vector_t impulse;
  rgb_color_t color;
  vector_t centroid;
  void *info;
  free_func_t info_freer;
  bool removed;
  image_t *image;
  vector_t image_size;

} body_t;

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer, image_t *image) {
  body_t *body = malloc(sizeof(body_t));
  assert(body);
  body->shape = shape;
  body->mass = mass;
  body->angle = 0;
  body->velocity = VEC_ZERO;
  body->acceleration = VEC_ZERO;
  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
  body->color = color;
  body->centroid = polygon_centroid(body->shape);
  body->info = info;
  body->info_freer = info_freer;
  body->removed = false;
  body->image = image;
  return body;
}

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  return body_init_with_info(shape, mass, color, NULL, free, NULL);
}

image_t *body_get_image(body_t *body) { return body->image; }

void body_free(void *b) {
  body_t *body = (body_t *)b;
  if (body->info_freer != NULL) {
    body->info_freer(body->info);
  }
  list_free(body->shape);
  if (body->image != NULL) {
    image_free(body->image);
  }
  free(body);
}

list_t *body_get_shape(body_t *body) { return list_copy(body->shape); }

vector_t body_get_centroid(body_t *body) { return body->centroid; }

vector_t body_get_velocity(body_t *body) { return body->velocity; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void *body_get_info(body_t *body) { return body->info; }

void body_set_info(body_t *body, void *info) {body->info = info;}

void body_set_centroid(body_t *body, vector_t x) {
  polygon_translate(body->shape, vec_subtract(x, body->centroid));
  body->centroid = x;
}

void body_set_color(body_t *body, rgb_color_t color) { body->color = color; }

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_rotation(body_t *body, double angle) {
  polygon_rotate(body->shape, angle - body->angle, body->centroid);
  body->angle = angle;
}

double body_get_mass(body_t *body) { return body->mass; }

void body_add_force(body_t *body, vector_t force) {
  body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
  body->impulse = vec_add(body->impulse, impulse);
}

void body_tick(body_t *body, double dt) {
  body->acceleration = vec_multiply(1.0 / body->mass, body->force);
  vector_t dv = vec_multiply(dt, body->acceleration);
  vector_t new_velo = vec_add(body->velocity, dv);
  new_velo = vec_add(new_velo, vec_multiply(1.0 / body->mass, body->impulse));
  vector_t avg_velo = vec_multiply(0.5, vec_add(body->velocity, new_velo));
  body->velocity = new_velo;
  vector_t dx = vec_multiply(dt, avg_velo);
  polygon_translate(body->shape, dx);
  body->centroid = vec_add(body->centroid, dx);
  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
}

void body_remove(body_t *body) { body->removed = true; }

bool body_is_removed(body_t *body) { return body->removed; }