#include "collision.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

list_t *projection(list_t *shape, vector_t axis) {
  list_t *endpoints = list_init(2, free);
  list_t *projections = list_init(list_size(shape), free);
  // find all projections
  for (size_t i = 0; i < list_size(shape); i++) {
    vector_t *vertex = list_get(shape, i);
    double *dot = malloc(sizeof(double));
    assert(dot);
    *dot = vec_dot(*vertex, axis);
    list_add(projections, dot);
  }
  // identify min and max projections
  double *min = malloc(sizeof(double));
  assert(min);
  *min = *((double *)list_get(projections, 0));
  double *max = malloc(sizeof(double));
  assert(max);
  *max = *((double *)list_get(projections, 0));
  for (size_t i = 1; i < list_size(projections); i++) {
    double *curr = list_get(projections, i);
    if (*curr < *min) {
      *min = *curr;
    } else if (*curr > *max) {
      *max = *curr;
    }
  }
  list_add(endpoints, min);
  list_add(endpoints, max);
  list_free(projections);
  return endpoints;
}

bool not_overlap(list_t *proj1, list_t *proj2) {
  double *a1 = list_get(proj1, 0);
  double *a2 = list_get(proj1, 1);
  double *b1 = list_get(proj2, 0);
  double *b2 = list_get(proj2, 1);
  return ((*a1 < *b1 && *a1 < *b2 && *a2 < *b1 && *a2 < *b2) ||
          (*b1 < *a1 && *b1 < *a2 && *b2 < *a1 && *b2 < *a2));
}

double proj_overlap(list_t *proj1, list_t *proj2) {
  double a1 = *(double *)list_get(proj1, 0);
  double a2 = *(double *)list_get(proj1, 1);
  double b1 = *(double *)list_get(proj2, 0);
  double b2 = *(double *)list_get(proj2, 1);
  double ans = fabs(a2 - b1);
  if (fabs(b2 - a1) < ans) {
    ans = fabs(b2 - a1);
  }
  return ans;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  collision_info_t info = {true, VEC_ZERO};
  double curr_min_overlap = 0;
  for (size_t i = 0; i < list_size(shape1); i++) {
    size_t next_i = i + 1;
    if (next_i == list_size(shape1)) {
      next_i = 0;
    }
    vector_t *vertex1 = list_get(shape1, i);
    vector_t *vertex2 = list_get(shape1, next_i);
    vector_t edge = vec_subtract(*vertex1, *vertex2);
    vector_t perp_vec = vec_norm(vec_perpendicular(edge));
    list_t *proj1 = projection(shape1, perp_vec);
    list_t *proj2 = projection(shape2, perp_vec);
    if (not_overlap(proj1, proj2)) {
      list_free(proj1);
      list_free(proj2);
      return (collision_info_t){false, VEC_ZERO};
    } else {
      double overlap_amount = proj_overlap(proj1, proj2);
      if (curr_min_overlap == 0 || overlap_amount < curr_min_overlap) {
        curr_min_overlap = overlap_amount;
        info.axis = perp_vec;
      }
    }
    list_free(proj1);
    list_free(proj2);
  }
  for (size_t i = 0; i < list_size(shape2); i++) {
    size_t next_i = i + 1;
    if (next_i == list_size(shape2)) {
      next_i = 0;
    }
    vector_t *vertex1 = list_get(shape2, i);
    vector_t *vertex2 = list_get(shape2, next_i);
    vector_t edge = vec_subtract(*vertex1, *vertex2);
    vector_t perp_vec = vec_norm(vec_perpendicular(edge));
    list_t *proj1 = projection(shape1, perp_vec);
    list_t *proj2 = projection(shape2, perp_vec);
    if (not_overlap(proj1, proj2)) {
      list_free(proj1);
      list_free(proj2);
      return (collision_info_t){false, VEC_ZERO};
    } else {
      double overlap_amount = proj_overlap(proj1, proj2);
      if (curr_min_overlap == 0 || overlap_amount < curr_min_overlap) {
        curr_min_overlap = overlap_amount;
        info.axis = perp_vec;
      }
    }
    list_free(proj1);
    list_free(proj2);
  }
  return info;
}
