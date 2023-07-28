#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>

/**
 * A growable array of pointers.
 * Can store values of any pointer type (e.g. vector_t*, body_t*).
 * The list automatically grows its internal array when more capacity is needed.
 */
typedef struct list list_t;

/**
 * A function that can be called on list elements to release their resources.
 * Examples: free, body_free
 */
typedef void (*free_func_t)(void *);

/**
 * Allocates memory for a new list with space for the given number of elements.
 * The list is initially empty.
 * Asserts that the required memory was allocated.
 *
 * @param initial_size the number of elements to allocate space for
 * @param freer if non-NULL, a function to call on elements in the list
 *   in list_free() when they are no longer in use
 * @return a pointer to the newly allocated list
 */
list_t *list_init(size_t initial_size, free_func_t freer);

/**
 * Releases the memory allocated for a list.
 *
 * @param list a pointer to a list returned from list_init()
 */
void list_free(list_t *list);

/**
 * Gets the size of a list (the number of occupied elements).
 * Note that this is NOT the list's capacity.
 *
 * @param list a pointer to a list returned from list_init()
 * @return the number of elements in the list
 */
size_t list_size(list_t *list);

/**
 * Returns true if the list has reached capacity
 * False otherwise
 *
 * @param list a pointer to a list returned from list_init()
 * @return whether the list is full or not
 */
bool list_full(list_t *list);

/**
 * Gets the element at a given index in a list.
 * Asserts that the index is valid, given the list's current size.
 *
 * @param list a pointer to a list returned from list_init()
 * @param index an index in the list (the first element is at 0)
 * @return the element at the given index, as a void*
 */
void *list_get(list_t *list, size_t index);

/**
 * Removes the element at a given index in a list and returns it,
 * moving all subsequent elements towards the start of the list.
 * Asserts that the index is valid, given the list's current size.
 *
 * @param list a pointer to a list returned from list_init()
 * @return the element at the given index in the list
 */
void *list_remove(list_t *list, size_t index);

/**
 * Appends an element to the end of a list.
 * If the list is filled to capacity, resizes the list to fit more elements
 * and asserts that the resize succeeded.
 * Also asserts that the value being added is non-NULL.
 *
 * @param list a pointer to a list returned from list_init()
 * @param value the element to add to the end of the list
 */
void list_add(list_t *list, void *value);

/**
 * Replaces the element at the specific index with the specified value
 * Asserts there is already an element at the index specified
 *
 * @param list a pointer to a list list returned from list_init()
 * @param index an index in the list
 * @param value an element to insert into the list
 */
void list_replace(list_t *list, size_t index, void *value);

/**
 * Gives the index corresponding to the first index of a certain value
 *
 * @param list the list to get the index from
 * @param value the value to get the index of
 */
size_t list_get_index(list_t *list, void *value);

/**
 * Returns a newly alloced copy of a given list
 *
 * @param list list to be copied
 */
list_t *list_copy(list_t *list);

/**
 * Returns the data inside a list
 *
 * @param list the list for which the data should be returned
 */
void **list_get_data(list_t *list);

void list_ensure_capacity(list_t *list);

bool list_contains(list_t *list, void *element);

#endif // #ifndef __LIST_H__
