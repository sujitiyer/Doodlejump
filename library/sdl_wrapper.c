#include "sdl_wrapper.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>

const char WINDOW_TITLE[] = "CS 3";
const int SDL_WINDOW_WIDTH = 1000;
const int SDL_WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

// Text constants
// SDL_Color RED = {255, 0, 0};
// SDL_Color BLACK = {0, 0, 0};
size_t TEXT_X1 = 600;
size_t TEXT_Y1 = 0;
size_t TEXT_W1 = 50;
size_t TEXT_H1 = 25;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  TTF_Init();
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, state);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);

  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);
  SDL_RenderPresent(renderer);
}

vector_t convert_to_sdl_coords(vector_t coords){
  return (vector_t){.x = 334 * coords.x / 800 + 333, .y = 500 - coords.y * 500 / 1200};
}

vector_t scale_to_sdl_coords(vector_t size){
  return (vector_t){334 * size.x / 800, size.y * 5 / 12};
}

void sdl_render_scene(scene_t *scene, list_t *texts, list_t *images) {
  sdl_clear();
  
  // draw image not associated with bodies
  for(size_t i = 0; i < list_size(images); i++){
    image_t image = *(image_t *)list_get(images, i);
    SDL_Surface *surface = image.image;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &(image.rect));
    SDL_DestroyTexture(texture);
  }

  // draw texts
  for (size_t i = 0; i < list_size(texts); i++) {
    text_t *text = (text_t *)list_get(texts, i);
    SDL_Surface *surface_message = TTF_RenderText_Solid(text->font, text->text, text->color); 
    SDL_Texture *message = SDL_CreateTextureFromSurface(renderer, surface_message);
    SDL_RenderCopy(renderer, message, NULL, &(text->message_rect));
    SDL_FreeSurface(surface_message);
    SDL_DestroyTexture(message);
  }  

  // draw bodies
  size_t body_count = scene_bodies(scene);
  for (size_t i = body_count; i > 0; i--) {
    body_t *body = scene_get_body(scene, i - 1);
    list_t *shape = body_get_shape(body);
    image_t *image = body_get_image(body);
    if (image == NULL){
      sdl_draw_polygon(shape, body_get_color(body));
    } else {
      SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image->image);
      vector_t pos = convert_to_sdl_coords(body_get_centroid(body));
      vector_t size = scale_to_sdl_coords((vector_t){image->rect.w, image->rect.h});  
      SDL_Rect rect = (SDL_Rect){pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y};
      SDL_RenderCopy(renderer, texture, NULL, &(rect));
      SDL_DestroyTexture(texture);
    }
    list_free(shape);
  }
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}
