#include "scene.h"
#include "collision.h"
#include "forces.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const size_t INITIAL_BODIES = 50;
const size_t INITIAL_FORCE_CREATORS = 3;

typedef struct bodies_force_container {
  force_creator_t forcer;
  force_creator_old_t forcer_old;
  collision_handler_t collision_handler;
  bool just_collided;
  void *aux;
  list_t *bodies;
  free_func_t freer;
  bool old;
} bodies_force_container_t;

typedef struct scene {
  list_t *bodies;
  list_t *force_containers;
} scene_t;

scene_t *scene_init() {
  scene_t *scene = malloc(sizeof(scene_t));
  assert(scene);
  scene->bodies = list_init(INITIAL_BODIES, body_free);
  scene->force_containers =
      list_init(INITIAL_FORCE_CREATORS, force_container_free);
  assert(scene->force_containers);
  return scene;
}

void force_container_free(void *force_container) {
  bodies_force_container_t *fc = (bodies_force_container_t *)force_container;
  if (fc->freer != NULL) {
    fc->freer(fc->aux);
  }
  free(list_get_data(fc->bodies));
  free(fc->bodies);
  free(fc);
}

void scene_free(scene_t *scene) {
  list_free(scene->force_containers);
  list_free(scene->bodies);
  free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

list_t *scene_get_bodies(scene_t *scene) { return list_copy(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
  return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
  assert(index < scene_bodies(scene));
  body_t *body = list_get(scene->bodies, index);
  body_remove(body);
}

void scene_add_force_creator(scene_t *scene, force_creator_old_t forcer,
                             void *aux, free_func_t freer) {
  bodies_force_container_t *force_container =
      malloc(sizeof(bodies_force_container_t));
  assert(force_container);
  force_container->forcer = NULL;
  force_container->aux = aux;
  force_container->bodies = scene_get_bodies(scene);
  force_container->freer = freer;
  force_container->old = true;
  force_container->forcer_old = forcer;
  list_add(scene->force_containers, force_container);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    collision_handler_t collision_handler,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  bodies_force_container_t *force_container =
      malloc(sizeof(bodies_force_container_t));
  assert(force_container);
  force_container->forcer = forcer;
  force_container->collision_handler = collision_handler;
  force_container->just_collided = false;
  force_container->aux = aux;
  force_container->bodies = bodies;
  force_container->freer = freer;
  force_container->old = false;
  force_container->forcer_old = NULL;
  list_add(scene->force_containers, force_container);
}

void scene_tick(scene_t *scene, double dt) {
  for (size_t i = 0; i < list_size(scene->force_containers); i++) {
    bodies_force_container_t *bfc = list_get(scene->force_containers, i);
    if (bfc->forcer != NULL) {
      bfc->forcer(bfc->aux, bfc->bodies);
    }
    if (bfc->forcer_old != NULL) {
      bfc->forcer_old(bfc->aux);
    }

    if (bfc->collision_handler != NULL) {
      body_t *body1 = list_get(bfc->bodies, 0);
      body_t *body2 = list_get(bfc->bodies, 1);
      list_t *shape1 = body_get_shape(body1);
      list_t *shape2 = body_get_shape(body2);
      collision_info_t info = find_collision(shape1, shape2);
      if (!info.collided) {
        if (bfc->just_collided) {
          bfc->just_collided = false;
        }
      } else {
        if (!(bfc->just_collided)) {
          bfc->collision_handler(body1, body2, info.axis, bfc->aux);
          bfc->just_collided = true;
        }
      }
      free(list_get_data(shape1));
      free(list_get_data(shape2));
      free(shape1);
      free(shape2);
    }
  }

  for (int64_t i = 0; i < list_size(scene->force_containers); i++) {
    bodies_force_container_t *bfc = list_get(scene->force_containers, i);
    bool body_was_removed = false;
    for (size_t j = 0; j < list_size(bfc->bodies); j++) {
      if (body_is_removed(list_get(bfc->bodies, j))) {
        body_was_removed = true;
        break;
      }
    }

    if (body_was_removed) {
      force_container_free(list_remove(scene->force_containers, i));
      i--;
    }
  }

  for (int64_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = list_get(scene->bodies, i);
    if (body_is_removed(body)) {
      list_remove(scene->bodies, list_get_index(scene->bodies, body));
      body_free(body);
      i--;
    } else {
      body_tick(body, dt);
    }
  }
}
