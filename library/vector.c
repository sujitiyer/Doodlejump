#include "vector.h"
#include <math.h>

const vector_t VEC_ZERO = {0, 0};

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t v3;
  v3.x = v1.x + v2.x;
  v3.y = v1.y + v2.y;
  return v3;
}

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t v2;
  v2.x = scalar * v.x;
  v2.y = scalar * v.y;
  return v2;
}

vector_t vec_negate(vector_t v) { return vec_multiply(-1.0, v); }

vector_t vec_subtract(vector_t v1, vector_t v2) {
  return vec_add(v1, vec_negate(v2));
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v2.x * v1.y; }

vector_t vec_rotate(vector_t v, double angle) {
  vector_t v2;
  v2.x = v.x * cos(angle) - v.y * sin(angle);
  v2.y = v.x * sin(angle) + v.y * cos(angle);
  return v2;
}

double vec_magnitude(vector_t v) { return sqrt(pow(v.x, 2) + pow(v.y, 2)); }

double vec_angle(vector_t v) { return atan(v.y / v.x); }

vector_t vec_perpendicular(vector_t v) {
  vector_t v2;
  v2.x = v.y;
  v2.y = -v.x;
  return v2;
}

vector_t vec_norm(vector_t v) {
  vector_t v2 = vec_multiply(1 / vec_magnitude(v), v);
  return v2;
}

vector_t vec_proj(vector_t v, vector_t axis) {
  return vec_multiply(vec_dot(v, axis), axis);
}
