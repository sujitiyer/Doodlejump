#include "sdl_wrapper.h"
#include <SDL2/SDL2_gfxPrimitives.h>
#include "image.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>

void image_free(void *image) {
    image_t *img = (image_t *)image;
    SDL_FreeSurface(img->image);
    free(image);
}

