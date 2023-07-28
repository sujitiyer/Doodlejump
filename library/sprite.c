#include "sprite.h"

// Platform
const vector_t PLATFORM_SIZE = {150, 30};
const rgb_color_t PLAT_COLOR = {0, 0.8, 0};

// Spring
const vector_t SPRING_SIZE = {25,30};
const rgb_color_t SPRING_COLOR = {0.8, 0.5, 0.5};

// Jetpack
const vector_t JETPACK_SIZE = {70, 70};
const rgb_color_t JETPACK_COLOR = {0.8, 0.5, 0.5};

// Player Constants
const size_t CURVE_POINTS = 60;
const double PLAYER_RADIUS = 25;
const vector_t PLAYER_IMG_SIZE = {80, 80};
const rgb_color_t PLAYER_COLOR = {0.9, 0.9, 0};

//Monster Constants
const vector_t MONSTER_SIZE = {170, 110};
const rgb_color_t MONSTER_COLOR = {0,0,1};

//Bullet Constants
const double BULLET_RADIUS = 4;
const rgb_color_t BULLET_COLOR = {0.2, 0.7, 0};
const double BULLET_IMG_RADIUS = 6;

//Black Hole Constants
const double BLACKHOLE_RADIUS = 60;
const rgb_color_t BLACKHOLE_COLOR = {0, 0, 0};

// Math Constants
const double PI = M_PI;
const double TWO_PI = 2 * M_PI;

body_t *generate_player(vector_t center) {
  double curr_angle = 0;
  double offset_angle = PI / CURVE_POINTS;
  list_t *shape = list_init(CURVE_POINTS,free);
  vector_t *right_corner = malloc(sizeof(vector_t));
  assert(right_corner);
  *right_corner = (vector_t){center.x + PLAYER_RADIUS, center.y - 2 * PLAYER_RADIUS};
  list_add(shape,right_corner);
  for (size_t i = 0; i < CURVE_POINTS; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * PLAYER_RADIUS + center.x;
    vec->y = sin(curr_angle) * PLAYER_RADIUS + center.y;
    list_add(shape, vec);
    curr_angle += offset_angle;
  }
  vector_t *left_corner = malloc(sizeof(vector_t));
  assert(left_corner);
  *left_corner = (vector_t){center.x - PLAYER_RADIUS, center.y - 2 * PLAYER_RADIUS};
  list_add(shape,left_corner);
  double mass = polygon_area(shape);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = PLAYER;
  SDL_Rect image_rect = {0, 0, PLAYER_IMG_SIZE.x, PLAYER_IMG_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/alien.bmp");
  image->rect = image_rect;
  body_t *player = body_init_with_info(shape, mass, PLAYER_COLOR, id, free, image);
  return player;
}

list_t *generate_rect_shape(vector_t pos, vector_t dim) {
  list_t *shape = list_init(4, free);
  vector_t *v1 = malloc(sizeof(vector_t));
  vector_t *v2 = malloc(sizeof(vector_t));
  vector_t *v3 = malloc(sizeof(vector_t));
  vector_t *v4 = malloc(sizeof(vector_t));
  assert(v1);
  assert(v2);
  assert(v3);
  assert(v4);
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

list_t *generate_circle_shape(vector_t center, size_t radius) {
  list_t *circle = list_init(CURVE_POINTS, free);
  double curr_angle = 0;
  double offset_angle = TWO_PI / CURVE_POINTS;
  for (size_t i = 0; i < CURVE_POINTS; i++) {
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = cos(curr_angle) * radius + center.x;
    vec->y = sin(curr_angle) * radius + center.y;
    list_add(circle, vec);
    curr_angle += offset_angle;
  }
  return circle;
}

body_t *generate_platform(vector_t pos){
  list_t *plat = generate_rect_shape(pos, PLATFORM_SIZE);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = PLATFORM;
  SDL_Rect image_rect = {0, 0, PLATFORM_SIZE.x, PLATFORM_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/platform.bmp");
  image->rect = image_rect;
  body_t *platform = body_init_with_info(plat, INFINITY, PLAT_COLOR, id, free, image);
  return platform;
}

body_t *generate_blue_platform(vector_t pos){
  list_t *plat = generate_rect_shape(pos, PLATFORM_SIZE);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = MOVING_PLATFORM;
  SDL_Rect image_rect = {0, 0, PLATFORM_SIZE.x, PLATFORM_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/blue_platform.bmp");
  image->rect = image_rect;
  body_t *platform = body_init_with_info(plat, INFINITY, PLAT_COLOR, id, free, image);
  return platform;
}

body_t *generate_spring(vector_t pos){
  list_t *spring = generate_rect_shape(pos, SPRING_SIZE);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = SPRING;
  SDL_Rect image_rect = {0, 0, SPRING_SIZE.x, SPRING_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/spring.bmp");
  image->rect = image_rect;
  body_t *spring_body = body_init_with_info(spring, INFINITY, SPRING_COLOR, id, free, image);
  return spring_body;
}

body_t *generate_jetpack(vector_t pos){
  list_t *jetpack = generate_rect_shape(pos, JETPACK_SIZE);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = JETPACK;
  SDL_Rect image_rect = {0, 0, JETPACK_SIZE.x, JETPACK_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/jetpack.bmp");
  image->rect = image_rect;
  body_t *jetpack_body = body_init_with_info(jetpack, INFINITY, JETPACK_COLOR, id, free, image);
  return jetpack_body;
}

body_t *generate_bullet(vector_t center) {
  list_t *circle = generate_circle_shape(center, BULLET_RADIUS);
  double mass = polygon_area(circle);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = BULLET;
  SDL_Rect image_rect = {0, 0, BULLET_IMG_RADIUS * 2, BULLET_IMG_RADIUS * 2};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/bullet.bmp");
  image->rect = image_rect;
  body_t *bullet = body_init_with_info(circle, mass, BULLET_COLOR, id, free, image);
  return bullet;
}

body_t *generate_monster(vector_t center) {
  list_t *rect = generate_rect_shape(center, MONSTER_SIZE);
  double mass = polygon_area(rect);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = MONSTER;
  SDL_Rect image_rect = {0, 0, MONSTER_SIZE.x, MONSTER_SIZE.y};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/monster.bmp");
  image->rect = image_rect;
  body_t *monster = body_init_with_info(rect, mass, MONSTER_COLOR, id, free, image);
  return monster;
}

body_t *generate_blackhole(vector_t center) {
  list_t *circle = generate_circle_shape(center, BLACKHOLE_RADIUS);
  double mass = polygon_area(circle);
  size_t *id = malloc(sizeof(size_t));
  assert(id);
  *id = BLACKHOLE;
  SDL_Rect image_rect = {0, 0, BLACKHOLE_RADIUS * 2, BLACKHOLE_RADIUS * 2};
  image_t *image = malloc(sizeof(image_t));
  assert(image);
  image->image = SDL_LoadBMP("assets/blackhole.bmp");
  image->rect = image_rect;
  body_t *blackhole = body_init_with_info(circle, mass, BLACKHOLE_COLOR, id, free, image);
  return blackhole;
}
