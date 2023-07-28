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
#include "sprite.h"
#include "text.h"
#include "image.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// window constants
const vector_t WINDOW = (vector_t){.x = 800, .y = 1200};
const vector_t CENTER = (vector_t){.x = 400, .y = 600};

// wall collision constants
typedef enum {
  NO_BOUNDARY,
  LEFT_BOUNDARY,
  RIGHT_BOUNDARY,
  TOP_BOUNDARY,
  BOTTOM_BOUNDARY
} boundary_type_t;

// scene constants
const double GRAVITY_CONST = -2500;
const double SCROLL_THRESHOLD = 600;
const double SPAWN_PLATFORM_THRESHOLD = 1800;
const double SPAWN_OBJECT_THRESHOLD = 1500;
const sprite_type_t OBJECT_TYPES[4] = {SPRING, MONSTER, BLACKHOLE, JETPACK};

// player constants
const vector_t PLAYER_INIT_LOCATION = (vector_t){.x = 400, .y = 500};
const vector_t PLAYER_SIZE = (vector_t){.x = 60, .y = 30};
const double PLAYER_CONTROL_SPEED = 300;
const double PLAYER_JUMP_SPEED = 1200;

// platform constants
const double PLATFORM_JUMP_MULTIPLIER = 1.0;
const vector_t PLATFORM_CONFIGURATIONS[4][4] = {
  {{450, 100}, {600, 225}, {550, 400}, {300, 550}},
  {{140, 100}, {225, 250}, {270, 400}, {500, 510}},
  {{450, 100}, {200, 175}, {600, 310}, {260, 460}},
  {{600, 100}, {450, 250}, {640, 400}, {200, 510}}
};
const size_t NUM_CONFIGURATIONS = sizeof(PLATFORM_CONFIGURATIONS[0]) / sizeof(PLATFORM_CONFIGURATIONS[0][0]);
const size_t NUM_PLATFORMS_CONFIGURATION = sizeof(PLATFORM_CONFIGURATIONS) / sizeof(PLATFORM_CONFIGURATIONS[0]);
const size_t PLATFORM_CONFIG_SIZE = 600;
const size_t MOVING_PROPORTION = 7;
const int64_t PLATFORM_SPEED = 300;
const size_t PLATFORM_BUFFER = 200;

// spring constants
const double SPRING_JUMP_MULTIPLIER = 1.5;
const size_t SPRING_BUFFER = 30;

// jetpack constants
const double JETPACK_JUMP_MULTIPLIER = 3.5;
const size_t JETPACK_BUFFER = 30;

// bullet constants
const double BULLET_SPEED = 1500;

// blackhole constants
const size_t BLACK_HOLE_BUFFER = 150;

// monster constants
const size_t MONSTER_BUFFER = 60;

// score constants
const size_t MONSTER_BONUS = 200;
const size_t MAX_DIGITS = 20;
const size_t SCORE_SCALER = 30;

//text constants
const SDL_Color RED = {200, 0, 0};
const SDL_Color BLACK = {0, 0, 0};
const size_t MAX_TEXTS = 10;

// image constants
SDL_Rect IMAGE_RECT = {333, 0, 334, 500};
SDL_Rect TITLE_RECT = {0, 0, 330, 500};
SDL_Rect SCORE_RECT = {720, 170, 200, 60};
SDL_Rect HIGH_SCORE_RECT = {720, 250, 200, 40};

// game management constants
const size_t SCREEN_INDEX = 2;

typedef struct state {
  scene_t *scene;
  body_t *player;
  double score;
  size_t high_score;
  double scrolled_since_last_spawn;
  bool game_over;
  list_t *images;
  list_t *texts;
  TTF_Font *score_font;
  bool start_screen;
} state_t;

// bounces body1 off of body2 upon top border collision on body2
void vertical_collision_handler(body_t *body1, body_t *body2,
                                vector_t axis, void *aux) {
  size_t *id = (size_t *)body_get_info(body1);                       
  if (*id == PLAYER_WITH_JETPACK) {
    return;
  }
  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  double *mag = (double *)aux;
  collision_info_t col_info = find_collision(shape1, shape2);
  free(list_get_data(shape1));
  free(list_get_data(shape2));
  free(shape1);
  free(shape2);
  if (body_get_velocity(body1).y < 0 && col_info.collided) {
    vector_t curr_velocity = body_get_velocity(body1);
    if (col_info.axis.y > 0) {
      body_set_velocity(body1, vec_multiply(*mag,
                        (vector_t){curr_velocity.x, PLAYER_JUMP_SPEED}));
    }
  }
}

// handles settings for when player gets a jetpack boost
void jetpack_collision_handler(body_t *body1, body_t *body2,
                               vector_t axis, void *aux) {
  size_t *id = (size_t *)body_get_info(body1);                        
  if (*id == PLAYER_WITH_JETPACK) {
    return;
  }
  list_t *shape1 = body_get_shape(body1);
  list_t *shape2 = body_get_shape(body2);
  double *mag = (double *)aux;
  collision_info_t col_info = find_collision(shape1, shape2);
  free(list_get_data(shape1));
  free(list_get_data(shape2));
  free(shape1);
  free(shape2);
  if (col_info.collided) {
    vector_t curr_velocity = body_get_velocity(body1);
    body_set_velocity(body1, vec_multiply(*mag,
                      (vector_t){curr_velocity.x, PLAYER_JUMP_SPEED}));
    free(body_get_info(body1));
    size_t *id = malloc(sizeof(size_t));
    assert(id);
    *id = PLAYER_WITH_JETPACK;
    body_set_info(body1, id);
    body_remove(body2);
    image_t *image = body_get_image(body1);
    SDL_FreeSurface(image->image);
    image->image = SDL_LoadBMP("assets/red_alien.bmp");
  }
}

void display_end_screen(state_t *state) {
  for(size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_remove(scene_get_body(state->scene, i));
  }
  image_t *losing_screen_img = malloc(sizeof(image_t));
  assert(losing_screen_img);
  losing_screen_img->image = SDL_LoadBMP("assets/losing_screen.bmp");
  losing_screen_img->rect = IMAGE_RECT;
  list_add(state->images, losing_screen_img);
  state->game_over = true;
}

void loss_collision_handler(body_t *body1, body_t *body2,
                            vector_t axis, void *aux){
  size_t *id = (size_t *)body_get_info(body1);
  // death disabled when player has jetpack                    
  if (*id == PLAYER_WITH_JETPACK) {
    return;
  }
  state_t *state = (state_t *)aux;
  display_end_screen(state);
}

size_t check_off_screen(body_t *body) {
  list_t *polygon = body_get_shape(body);
  bool left = true;
  bool right = true;
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t vec = *(vector_t *)(list_get(polygon, i));
    if (vec.y < 0) {
      list_free(polygon);
      return BOTTOM_BOUNDARY;
    }
    else if (vec.y > WINDOW.y) {
      list_free(polygon);
      return TOP_BOUNDARY;
    }
    else if (left && vec.x > 0) {
      left = false;
    }
    else if (right && vec.x < WINDOW.x) {
      right = false;
    }
  }
  if (left) {
    list_free(polygon);
    return LEFT_BOUNDARY;
  }
  if (right) {
    list_free(polygon);
    return RIGHT_BOUNDARY;
  }
  list_free(polygon);
  return NO_BOUNDARY;
}

void wrap_player(body_t *player, boundary_type_t boundary) {
  vector_t curr_centroid = body_get_centroid(player);
  vector_t new_centroid = curr_centroid;
  if (boundary == LEFT_BOUNDARY) {
    new_centroid = (vector_t){WINDOW.x - PLAYER_SIZE.x / 2, curr_centroid.y};
  } else if (boundary == RIGHT_BOUNDARY) {
    new_centroid = (vector_t){1.0 * PLAYER_SIZE.x / 2, curr_centroid.y};
  }
  body_set_centroid(player, new_centroid);
}

void bullet_init(state_t *state, vector_t velo) {
  body_t *bullet = generate_bullet(body_get_centroid(state->player));
  body_set_velocity(bullet, velo);
  for (size_t i = 1; i < scene_bodies(state->scene); i++) {
    body_t *curr_body = scene_get_body(state->scene, i);
    size_t *info = (size_t *)body_get_info(curr_body);
    if (*info == MONSTER) {
      create_destructive_collision(state->scene, bullet, curr_body);
    }
  }
  scene_add_body(state->scene, bullet);
}

size_t clean_check(body_t *body) {
  list_t *shape = body_get_shape(body);
  bool below_bottom = true;
  for (size_t i = 0; i < list_size(shape); i++) {
    vector_t *point = list_get(shape, i);
    if (point->x < 0) {
      list_free(shape);
      return LEFT_BOUNDARY;
    }
    if (point->x > WINDOW.x) {
      list_free(shape);
      return RIGHT_BOUNDARY;
    }
    if (point->y > 0) {
      below_bottom = false;
    }
    if (point->y > WINDOW.y) {
      list_free(shape);
      return TOP_BOUNDARY;
    }
  }
  list_free(shape);
  if (below_bottom) {
    return BOTTOM_BOUNDARY;
  }
  return NO_BOUNDARY;
}

void clean_scene(scene_t *scene) {
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    size_t coll = clean_check(curr_body);
    if (coll != NO_BOUNDARY) {
      size_t *id = (size_t *)body_get_info(curr_body);
      if (!(coll == TOP_BOUNDARY && *id != PLAYER && *id != BULLET)) {
        body_remove(curr_body);       
      }
    }
  }
}

void scroll_scene(scene_t *scene, double offset) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    vector_t centroid = body_get_centroid(body);
    centroid.y += offset;
    body_set_centroid(body, centroid);
  }
}

size_t count_monsters(scene_t *scene) {
  size_t num_monsters = 0;
  for (size_t i = 1; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    num_monsters += (size_t)(*(size_t *)body_get_info(curr_body) == MONSTER);
  }
  return num_monsters;
}

void spawn_platforms(state_t *state, size_t configuration, bool init, size_t additional_offset) {
  scene_t *scene = state->scene;
  body_t *player = state->player;
  for (size_t i = 0; i < NUM_CONFIGURATIONS; i++) {
    size_t move = rand();
    body_t *curr_platform = NULL;
    if (init) {
      curr_platform = generate_platform((vector_t){PLATFORM_CONFIGURATIONS[configuration][i].x, 
      PLATFORM_CONFIGURATIONS[configuration][i].y + additional_offset});
    }
    else {
      if (move % MOVING_PROPORTION == 0) {
        curr_platform = generate_blue_platform((vector_t){PLATFORM_CONFIGURATIONS[configuration][i].x,
        PLATFORM_CONFIGURATIONS[configuration][i].y + SPAWN_PLATFORM_THRESHOLD});
        create_horizontal_motion(scene, WINDOW, PLATFORM_SPEED, PLATFORM_BUFFER, curr_platform);
      }
      else {
        curr_platform = generate_platform((vector_t){PLATFORM_CONFIGURATIONS[configuration][i].x,
        PLATFORM_CONFIGURATIONS[configuration][i].y + SPAWN_PLATFORM_THRESHOLD});
      }
    }
    scene_add_body(scene, curr_platform);
    double *mult = malloc(sizeof(double));
    assert(mult);
    *mult = PLATFORM_JUMP_MULTIPLIER;
    create_collision(scene, player, curr_platform, vertical_collision_handler, mult, free);
  }
}

body_t *get_highest_platform(state_t *state) {
  scene_t *scene = state->scene;
  for (size_t i = scene_bodies(scene); i > 0; i--) {
    body_t *curr_body = scene_get_body(scene, i - 1);
    size_t *id = (size_t *) body_get_info(curr_body);
    if (*id == PLATFORM || *id == MOVING_PLATFORM) {
      return curr_body;
    }
  }
  return NULL;
}

bool should_spawn_platforms(state_t *state) {
  if (body_get_centroid(get_highest_platform(state)).y < SPAWN_PLATFORM_THRESHOLD) {
    return true;
  }
  return false;
}

void spawn_object(state_t *state){
  body_t *platform = get_highest_platform(state);
  vector_t platform_pos = body_get_centroid(platform);
  sprite_type_t type = OBJECT_TYPES[rand() % (sizeof(OBJECT_TYPES) / sizeof(OBJECT_TYPES[0]))];
  body_t *body;
  switch (type) {
    case SPRING:
      body = generate_spring((vector_t){.x = platform_pos.x, .y = platform_pos.y + SPRING_BUFFER});
      scene_add_body(state->scene, body);
      double *mult = malloc(sizeof(double));
      assert(mult);
      *mult = SPRING_JUMP_MULTIPLIER;
      create_collision(state->scene, state->player, body, vertical_collision_handler, mult, free);
      break;
    case MONSTER:
      body = generate_monster((vector_t){.x = platform_pos.x, .y = platform_pos.y + MONSTER_BUFFER});
      scene_add_body(state->scene, body);
      create_collision(state->scene, state->player, body, loss_collision_handler, state, NULL);
      break;
    case BLACKHOLE:
      if (platform_pos.x > CENTER.x) {
        body = generate_blackhole((vector_t){.x = BLACK_HOLE_BUFFER, .y = platform_pos.y});
      } else {
        body = generate_blackhole((vector_t){.x = WINDOW.x - BLACK_HOLE_BUFFER, .y = platform_pos.y});
      }
      scene_add_body(state->scene, body);
      create_collision(state->scene, state->player, body, loss_collision_handler, state, NULL);
      break;
    case JETPACK:
      body = generate_jetpack((vector_t){.x = platform_pos.x, .y = platform_pos.y + JETPACK_BUFFER});
      scene_add_body(state->scene, body);
      double *jet_mult = malloc(sizeof(double));
      assert(jet_mult);
      *jet_mult = JETPACK_JUMP_MULTIPLIER;
      create_collision(state->scene, state->player, body, jetpack_collision_handler, jet_mult, free);
      break;
    default:
      break;
  }
}

void reset_game(state_t *state) {
  
  state->scene = scene_init();
  state->score = 0;

  body_t *player = generate_player(PLAYER_INIT_LOCATION);
  state->player = player;
  create_downward_gravity(state->scene, GRAVITY_CONST, player);
  scene_add_body(state->scene, player);

  state->scrolled_since_last_spawn = SPAWN_OBJECT_THRESHOLD;

  //Initialization using spawn_platforms instead of harcoding initialization
  //Adds 3 platform configurations of 600 pixels each to beginning scene
  for (size_t i = 0; i < 3; i++) {
    //used to ensure that the starting configuration always has platform underneath doodler,
    //prevents instant loss
    size_t config = 0;
    if (i != 0) {
      config = rand() % NUM_CONFIGURATIONS;
    }
    if (i == 1) {
      config = 2;
    }
    if (i == 2) {
      config = 2;
    }
    spawn_platforms(state, config, true, PLATFORM_CONFIG_SIZE * i);
  }

  state->game_over = false;
}

void on_key(char key, key_event_type_t type, double held_time, void *status) {
  state_t *state = (state_t *)status;
  if (type == KEY_PRESSED && key == ' ') {
    if (state->start_screen) {
      state->start_screen = false;
      list_remove(state->images, SCREEN_INDEX);
      reset_game(state);
    } else if (state->game_over) {
      list_remove(state->images, SCREEN_INDEX);
      reset_game(state);
    }
    return;
  }
  
  if (!state->start_screen && !state->game_over) {
    body_t *player = state->player;
    vector_t player_velocity = body_get_velocity(player);
    if (type == KEY_PRESSED) {
      switch (key) {
        case LEFT_ARROW:  
          // move player left
          player_velocity.x = -PLAYER_CONTROL_SPEED;
          break;
        case RIGHT_ARROW:
          // move player right
          player_velocity.x = PLAYER_CONTROL_SPEED;
          break;
        case 'a':
          // shoot bullet up and to left
          bullet_init(state, (vector_t){-BULLET_SPEED/sqrt(2), BULLET_SPEED/sqrt(2)});
          break;
        case 'd':
          // shoot bullet up and to right
          bullet_init(state, (vector_t){BULLET_SPEED/sqrt(2), BULLET_SPEED/sqrt(2)});
          break;
        case 'w':
          // shoot bullet up
          bullet_init(state, (vector_t){0, BULLET_SPEED});
          break;
        case 's':
          // shoot bullet down
          bullet_init(state, (vector_t){0, -BULLET_SPEED});
          break;
        default:
          break;
      }
    } else if (type == KEY_RELEASED) {
      if (key == LEFT_ARROW || key == RIGHT_ARROW) {
        player_velocity = (vector_t){.x = 0, .y = body_get_velocity(player).y};
      }
    }
    body_set_velocity(player, player_velocity);
  }
}

state_t *emscripten_init() {
  vector_t min = (vector_t){.x = 0, .y = 0};
  vector_t max = WINDOW;
  sdl_init(min, max);

  time_t t;
  srand((unsigned)time(&t));

  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = scene_init();
  state->texts = NULL;
  state->images = NULL;
  state->score = 0;
  state->high_score = 0;
  state->score_font = TTF_OpenFont("assets/GROCHES.ttf", 15);
  state->texts = list_init(2, text_free);
  state->images = list_init(2, image_free);
  state->start_screen = true;
  state->game_over = true;

  // background image
  image_t *background_img = malloc(sizeof(image_t));
  assert(background_img);
  background_img->image = SDL_LoadBMP("assets/background.bmp");
  background_img->rect = IMAGE_RECT;
  list_add(state->images, background_img);

  // side title
  image_t *title_image = malloc(sizeof(image_t));
  assert(title_image);
  title_image->image = SDL_LoadBMP("assets/sidetitle.bmp");
  title_image->rect = TITLE_RECT;
  list_add(state->images, title_image);

  // score text
  text_t *score_text = malloc(sizeof(text_t));
  assert(score_text);
  score_text->font = state->score_font;
  score_text->color = RED;
  char *text = malloc(sizeof(char) * MAX_DIGITS);
  assert(text);
  snprintf(text, MAX_DIGITS, "Score: %zu\n", (size_t)(state->score));
  score_text->message_rect = SCORE_RECT;
  score_text->text = text;
  list_add(state->texts, score_text);

  // high score text
  text_t *high_score_text = malloc(sizeof(text_t));
  assert(high_score_text);
  high_score_text->font = state->score_font;
  high_score_text->color = RED;
  char *hs_text = malloc(sizeof(char) * MAX_DIGITS);
  assert(hs_text);
  snprintf(hs_text, MAX_DIGITS, "High Score: %zu\n", (state->high_score));
  high_score_text->message_rect = HIGH_SCORE_RECT;
  high_score_text->text = hs_text;
  list_add(state->texts, high_score_text); 

  // start screen
  image_t *start_image = malloc(sizeof(image_t));
  assert(start_image);
  start_image->image = SDL_LoadBMP("assets/welcome_screen.bmp");
  start_image->rect = IMAGE_RECT;
  list_add(state->images, start_image);

  return state;
}

void emscripten_main(state_t *state) {
  scene_t *scene = state->scene;
  double dt = time_since_last_tick();
  sdl_on_key(on_key);

  if (state->score > state->high_score) {
    state->high_score = state->score;
  }
  text_t *score_text = list_get(state->texts, 0);
  snprintf(score_text->text, MAX_DIGITS, "Score: %zu\n", (size_t)(state->score));
  text_t *high_score_text = list_get(state->texts, 1);
  snprintf(high_score_text->text, MAX_DIGITS, "High Score: %zu\n", (state->high_score));
  
  if (!state->game_over) {
    body_t *player = state->player;
    boundary_type_t boundary_detection = check_off_screen(state->player);
    if (boundary_detection == BOTTOM_BOUNDARY) {
      display_end_screen(state);
    }
    else if (boundary_detection == LEFT_BOUNDARY || boundary_detection == RIGHT_BOUNDARY) {
      wrap_player(state->player, boundary_detection);
    }
    if (body_get_centroid(player).y >= SCROLL_THRESHOLD && body_get_velocity(player).y > 0) {
      double offset = body_get_velocity(player).y * dt;
      scroll_scene(scene, -offset);
      state->scrolled_since_last_spawn += offset;
      state->score += offset / SCORE_SCALER;
    }

    if (should_spawn_platforms(state)) {
      spawn_platforms(state, rand() % NUM_CONFIGURATIONS, false, 0);  
    }

    if (state->scrolled_since_last_spawn > SPAWN_OBJECT_THRESHOLD) {
      body_t *platform = get_highest_platform(state);
      vector_t platform_velo = body_get_velocity(platform);
      if (platform_velo.x == 0) {
        state->scrolled_since_last_spawn = 0;
        spawn_object(state);
      }
    }

    size_t *player_id = (size_t *)body_get_info(player);
    vector_t player_velo = body_get_velocity(player);
    if (*player_id == PLAYER_WITH_JETPACK && player_velo.y < PLAYER_JUMP_SPEED) {
      free(body_get_info(player));
      size_t *p_id = malloc(sizeof(size_t));
      assert(p_id);
      *p_id = PLAYER;
      body_set_info(player, p_id);
      image_t *image = body_get_image(player);
      SDL_FreeSurface(image->image);
      image->image = SDL_LoadBMP("assets/alien.bmp");
    }

    size_t prev_monst = count_monsters(scene);
    scene_tick(scene, dt);
    size_t after_monst = count_monsters(scene);
    state->score += (prev_monst - after_monst) * MONSTER_BONUS;
    
    clean_scene(scene);
  } else {
    scene_tick(scene, dt);    
  }
  sdl_render_scene(scene, state->texts, state->images);
}

// frees the memory associated with everything
void emscripten_free(state_t *state) {
  scene_free(state->scene);
  list_free(state->texts);
  list_free(state->images);
  TTF_CloseFont(state->score_font);
}
