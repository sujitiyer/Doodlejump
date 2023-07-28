#include "forces.h"
#include "collision.h"
#include "polygon.h"
#include "scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double MIN_GRAV_DISTANCE = 5.0;

typedef struct gravity_auxillary {
  double grav_constant;
} gravity_auxillary_t;

typedef struct spring_auxillary {
  double spring_constant;
} spring_auxillary_t;

typedef struct drag_auxillary {
  double drag_constant;
} drag_auxillary_t;

typedef struct physics_collision_auxillary {
  double elasticity;
} physics_collision_auxillary_t;

typedef struct horizontal_motion_auxillary {
  vector_t window;
  int64_t speed;
  size_t buffer;
} horizontal_motion_auxillary_t;

void auxillary_freer(void *auxillary) { free(auxillary); }

void newtonian_gravity_force_creator(void *auxillary, list_t *bodies) {
  gravity_auxillary_t *aux = (gravity_auxillary_t *)auxillary;
  double G = aux->grav_constant;
  for (size_t i = 0; i < list_size(bodies) - 1; i++) {
    for (size_t j = i + 1; j < list_size(bodies); j++) {
      body_t *body1 = list_get(bodies, i);
      body_t *body2 = list_get(bodies, j);
      vector_t span =
          vec_subtract(body_get_centroid(body1), body_get_centroid(body2));
      double dist = vec_magnitude(span);
      if (dist < MIN_GRAV_DISTANCE) {
        return;
      }
      double mag =
          G * body_get_mass(body1) * body_get_mass(body2) / (pow(dist, 3));
      vector_t grav_force = vec_multiply(mag, span);
      body_add_force(body1, vec_negate(grav_force));
      body_add_force(body2, grav_force);
    }
  }
}

void create_newtonian_gravity(scene_t *scene, double G, list_t *bodies) {
  gravity_auxillary_t *aux = malloc(sizeof(gravity_auxillary_t));
  assert(aux);
  aux->grav_constant = G;
  scene_add_bodies_force_creator(scene, newtonian_gravity_force_creator, NULL,
                                 aux, bodies, auxillary_freer);
}

void create_newtonian_gravity_old(scene_t *scene, double G, body_t *body1,
                                  body_t *body2) {
  list_t *bodies = list_init(2, (void *)body_free);
  list_add(bodies, body1);
  list_add(bodies, body2);
  create_newtonian_gravity(scene, G, bodies);
}

void downward_gravity_force_creator(void *auxillary, list_t *bodies) {
  gravity_auxillary_t *aux = (gravity_auxillary_t *)auxillary;
  double G = aux->grav_constant;
  body_t *body = list_get(bodies, 0);
  vector_t acc = (vector_t){0, G};
  body_add_force(body, vec_multiply(body_get_mass(body), acc));
}

void create_downward_gravity(scene_t *scene, double G, body_t *body) {
  list_t *bodies = list_init(1, (void *)body_free);
  list_add(bodies, body);
  gravity_auxillary_t *aux = malloc(sizeof(gravity_auxillary_t));
  assert(aux);
  aux->grav_constant = G;
  scene_add_bodies_force_creator(scene, downward_gravity_force_creator, NULL,
                                 aux, bodies, auxillary_freer);
}
void spring_force_creator(void *auxillary, list_t *bodies) {
  spring_auxillary_t *aux = (spring_auxillary_t *)auxillary;
  double k = aux->spring_constant;
  for (size_t i = 0; i < list_size(bodies); i++) {
    for (size_t j = i + 1; j < list_size(bodies); j++) {
      body_t *body1 = list_get(bodies, i);
      body_t *body2 = list_get(bodies, j);
      vector_t span =
          vec_subtract(body_get_centroid(body1), body_get_centroid(body2));
      vector_t spring_force = vec_multiply(k, span);
      body_add_force(body2, spring_force);
      body_add_force(body1, vec_negate(spring_force));
    }
  }
}

void create_spring(scene_t *scene, double k, list_t *bodies) {
  spring_auxillary_t *aux = malloc(sizeof(spring_auxillary_t));
  assert(aux);
  aux->spring_constant = k;
  scene_add_bodies_force_creator(scene, spring_force_creator, NULL, aux, bodies,
                                 auxillary_freer);
}

void create_spring_old(scene_t *scene, double k, body_t *body1, body_t *body2) {
  list_t *bodies = list_init(2, body_free);
  list_add(bodies, body1);
  list_add(bodies, body2);
  create_spring(scene, k, bodies);
}

void create_drag_old(scene_t *scene, double gamma, body_t *body) {
  list_t *bodies = list_init(1, (void *)body_free);
  list_add(bodies, body);
  create_spring(scene, gamma, bodies);
}

void drag_force_creator(void *auxillary, list_t *bodies) {
  drag_auxillary_t *aux = (drag_auxillary_t *)auxillary;
  double gamma = aux->drag_constant;
  for (size_t i = 0; i < list_size(bodies); i++) {
    body_t *body = list_get(bodies, i);
    vector_t drag_force =
        vec_negate(vec_multiply(gamma, body_get_velocity(body)));
    body_add_force(body, drag_force);
  }
}

void create_drag(scene_t *scene, double gamma, list_t *bodies) {
  drag_auxillary_t *aux = malloc(sizeof(drag_auxillary_t));
  assert(aux);
  aux->drag_constant = gamma;
  scene_add_bodies_force_creator(scene, drag_force_creator, NULL, aux, bodies,
                                 auxillary_freer);
}

void horizontal_motion_force_creator(void *auxillary, list_t *bodies) {
  horizontal_motion_auxillary_t *aux = (horizontal_motion_auxillary_t *)auxillary;
  body_t *body = list_get(bodies, 0);
  vector_t curr_velo = body_get_velocity(body);
  vector_t center = body_get_centroid(body);
  if (curr_velo.x == 0) {
    body_set_velocity(body, (vector_t){aux->speed, curr_velo.y});
  }
  else if (curr_velo.x < 0 && center.x < aux->buffer) {
    body_set_velocity(body, (vector_t){aux->speed, curr_velo.y});
  }
  else if (curr_velo.x > 0 && center.x > aux->window.x - aux->buffer) {
    body_set_velocity(body, (vector_t){-1 * aux->speed, curr_velo.y});
  }
}

void create_horizontal_motion(scene_t *scene, vector_t window, size_t speed,
                              size_t buffer, body_t *body) {
  horizontal_motion_auxillary_t *aux = malloc(sizeof(horizontal_motion_auxillary_t));
  assert(aux);
  list_t *bodies = list_init(1, body_free);
  list_add(bodies, body);
  aux->window = window;
  aux->speed = speed;
  aux->buffer = buffer;
  scene_add_bodies_force_creator(scene, horizontal_motion_force_creator, NULL,
                                 aux, bodies, auxillary_freer);
}

void destructive_collision_force_creator(body_t *body1, body_t *body2,
                                         vector_t axis, void *aux) {
  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  collision_info_t info = find_collision(shape1, shape2);
  if (info.collided) {
    body_remove(body1);
    body_remove(body2);
  }
  free(list_get_data(shape1));
  free(list_get_data(shape2));
  free(shape1);
  free(shape2);
}

void physics_collision_force_creator(body_t *body1, body_t *body2,
                                     vector_t axis, void *aux) {
  physics_collision_auxillary_t *auxillary =
      (physics_collision_auxillary_t *)aux;
  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  collision_info_t collision_info = find_collision(shape1, shape2);
  free(list_get_data(shape1));
  free(list_get_data(shape2));
  free(shape1);
  free(shape2);
  vector_t collision_axis = collision_info.axis;

  double mass1 = body_get_mass(body1);
  double mass2 = body_get_mass(body2);
  if (mass1 == INFINITY) {
    double reduced_mass = mass2;
    double impulse_component = reduced_mass * (1 + auxillary->elasticity);
    vector_t impulse_proj = vec_proj(body_get_velocity(body2), collision_axis);
    vector_t impulse_collision_axis =
        vec_multiply(-impulse_component, impulse_proj);
    body_add_impulse(body2, impulse_collision_axis);
  } else if (mass2 == INFINITY) {
    double reduced_mass = mass1;
    double impulse_component = reduced_mass * (1 + auxillary->elasticity);
    vector_t impulse_proj = vec_proj(body_get_velocity(body1), collision_axis);
    vector_t impulse_collision_axis =
        vec_multiply(-impulse_component, impulse_proj);
    body_add_impulse(body1, impulse_collision_axis);
  } else {
    double reduced_mass = (body_get_mass(body1) * body_get_mass(body2)) /
                          (body_get_mass(body1) + body_get_mass(body2));
    double impulse_component = reduced_mass * (1 + auxillary->elasticity);
    vector_t impulse_body1_proj =
        vec_proj(body_get_velocity(body1), collision_axis);
    vector_t impulse_body2_proj =
        vec_proj(body_get_velocity(body2), collision_axis);
    vector_t body_impulse =
        vec_multiply(impulse_component,
                     vec_subtract(impulse_body2_proj, impulse_body1_proj));
    body_add_impulse(body1, body_impulse);
    body_add_impulse(body2, vec_negate(body_impulse));
  }
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  list_t *bodies = list_init(2, body_free);
  physics_collision_auxillary_t *aux =
      malloc(sizeof(physics_collision_auxillary_t));
  assert(aux);
  aux->elasticity = elasticity;
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, NULL, physics_collision_force_creator,
                                 aux, bodies, auxillary_freer);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  list_t *bodies = list_init(2, body_free);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, NULL,
                                 destructive_collision_force_creator, NULL,
                                 bodies, auxillary_freer);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  list_t *bodies = list_init(2, body_free);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, NULL, handler, aux, bodies, freer);
}