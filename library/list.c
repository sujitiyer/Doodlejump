#include "list.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

const size_t LIST_GROWTH_FACTOR = 2;

typedef struct list {
  size_t length;
  size_t capacity;
  void **data;
  free_func_t freer;
} list_t;

list_t *list_init(size_t initial_size, free_func_t freer) {
  list_t *init = malloc(sizeof(list_t));
  assert(init);
  init->length = 0;
  init->capacity = initial_size;
  init->data = malloc(sizeof(void *) * initial_size);
  assert(init->data);
  init->freer = freer;
  return init;
}

void list_free(list_t *list) {
  for (size_t i = 0; i < list->length; i++) {
    if (list->freer != NULL) {
      list->freer(list->data[i]);
    }
  }
  free(list->data);
  free(list);
}

size_t list_size(list_t *list) { return list->length; }

bool list_full(list_t *list) { return list->length == list->capacity; }

void *list_get(list_t *list, size_t index) {
  assert(index < list->length && index >= 0);
  return list->data[index];
}

void list_add(list_t *list, void *value) {
  assert(value != NULL);
  list_ensure_capacity(list);
  list->data[list->length] = value;
  list->length++;
}

void list_ensure_capacity(list_t *list) {
  if (list_full(list)) {
    size_t new_capacity = list->length * LIST_GROWTH_FACTOR + 1;
    list->capacity = new_capacity;
    list->data = realloc(list->data, new_capacity * sizeof(void *));
  }
}

void list_replace(list_t *list, size_t index, void *value) {
  assert(index < list->length && index >= 0);
  list->data[index] = value;
}

size_t list_get_index(list_t *list, void *value) {
  for (int i = 0; i < list->length; i++) {
    if (value == list->data[i]) {
      return i;
    }
  }
  assert(false);
}

void *list_remove(list_t *list, size_t index) {
  assert(index <= list->capacity);
  void *val = list->data[index];
  for (size_t i = index; i < list->length - 1; i++) {
    list->data[i] = list->data[i + 1];
  }
  list->length--;
  return val;
}

list_t *list_copy(list_t *list) {
  list_t *copy = list_init(list_size(list), NULL);
  for (size_t i = 0; i < list_size(list); i++) {
    list_add(copy, list_get(list, i));
  }
  return copy;
}

void **list_get_data(list_t *list) { return list->data; }

bool list_contains(list_t *list, void *element) {
  for (size_t i = 0; i < list_size(list); i++) {
    if (list_get(list, i) == element) {
      return true;
    }
  }
  return false;
}
