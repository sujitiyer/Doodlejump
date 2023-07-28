#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct image{
    SDL_Surface *image;
    SDL_Rect rect;
} image_t;

void image_free(void *image);

#endif // #ifndef __IMAGE_H__