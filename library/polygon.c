#include "polygon.h"
#include "list.h"
#include "vector.h"

double polygon_area(list_t *polygon) {
  double area = 0;
  size_t num_vertices = list_size(polygon);
  for (size_t i = 0; i < num_vertices; i++) {
    vector_t v1 = *((vector_t *)list_get(polygon, i));
    vector_t v2 = *((vector_t *)list_get(polygon, (i + 1) % num_vertices));
    area += vec_cross(v1, v2);
  }
  return area / 2;
}

vector_t polygon_centroid(list_t *polygon) {
  vector_t centroid = VEC_ZERO;
  size_t num_vertices = list_size(polygon);

  for (size_t i = 0; i < num_vertices; i++) {
    vector_t v1 = *((vector_t *)list_get(polygon, i));
    vector_t v2 = *((vector_t *)list_get(polygon, (i + 1) % num_vertices));
    centroid.x += (v1.x + v2.x) * vec_cross(v1, v2);
    centroid.y += (v1.y + v2.y) * vec_cross(v1, v2);
  }

  double area = polygon_area(polygon);
  centroid.x /= 6 * area;
  centroid.y /= 6 * area;
  return centroid;
}

void polygon_translate(list_t *polygon, vector_t translation) {
  for (size_t i = 0; i < list_size(polygon); i++) {
    ((vector_t *)list_get(polygon, i))->x += translation.x;
    ((vector_t *)list_get(polygon, i))->y += translation.y;
  }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t *v = list_get(polygon, i);
    *v = vec_subtract(*v, point);
    *v = vec_rotate(*v, angle);
    *v = vec_add(*v, point);
  }
}
