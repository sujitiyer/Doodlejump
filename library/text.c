#include "text.h"

void text_free(void *text) {
    text_t *txt = (text_t *)text;
    free(txt->text);
    free(text);
}