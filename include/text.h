#ifndef __TEXT_H__
#define __TEXT_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct text {
    char *text;
    TTF_Font *font;
    SDL_Color color;
    SDL_Rect message_rect;
} text_t;

void text_free(void *text);

#endif // #ifndef __TEXT_H__