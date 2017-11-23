/*
 * Noted by zelbrium
 * MIT License
 *
 * array.h: Dynamically allocating arrays.
 */

#ifndef array_h
#define array_h

#include <stdio.h>
#include <stdbool.h>

typedef void (*FreeNotify)(void *element);

/*
 * Returns an array that can be casted to the desired
 * element type. For example, if it is an array of 32-bit
 * its, elementSize = 4, and the return can be casted
 * to int32_t *.
 *
 * Only free this with array_free.
 */
void * array_new(size_t elementSize, FreeNotify freeElement);

/*
 * Frees an array returned by array_new.
 * Calls freeElement on each element.
 */
void array_free(void *array);

/*
 * Appends element to array, and returns a pointer
 * to the array. This should always be called as
 * a = array_append(a, elem);
 */
void * array_append(void *array, void *element);

/*
 * Inserts element to array at index, and returns a
 * pointer to the array. This should always be called as
 * a = array_append(a, elem);
 */
//void * array_insert(void *array, void *element, size_t index);

/*
 * Removes the element at index in array, reducing the
 * array's size by 1. free = true to call freeElement
 * on the removed element.
 */
void array_remove(void *array, size_t index, bool free);

/*
 * Returns the size of the array.
 */
size_t array_size(void *array);

#endif /* array_h */
