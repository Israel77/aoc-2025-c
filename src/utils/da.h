#ifndef DA_H
#define DA_H

/*
 * Macros for manipulating dynamic arrays, mostly yanked from nob.h (courtesy of Alexey Kutepov)
 */
#include "allocator.h"
#include <stddef.h>
#include <string.h>
#include <assert.h>

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif /* ifndef DA_INIT_CAP */

typedef struct {
    size_t item_size;
    size_t count;
    size_t capacity;
    allocator_t *allocator;
    void *alloc_ctx;
} array_info_t;

void *da_reserve(void *array, array_info_t *info, size_t expected_capacity) {
    assert(info->item_size != 0 && "You forgot to set the size");
    void *result = array;

    if ((expected_capacity) > info->capacity) {
        size_t old_size = info->count * info->item_size;

        if (info->capacity == 0) {
            info->capacity = DA_INIT_CAP;
        }

        while ((expected_capacity) > info->capacity) {
            info->capacity *= 2;
        }

        result = info->allocator->realloc(info->alloc_ctx, array, old_size, info->capacity * info->item_size);
        assert(result != NULL && "Error on memory allocation.");
    }

     return result;
}

void *da_append(void *array, array_info_t *info, const void *item) {
    void *result = array;

    size_t offset = info->count * info->item_size;
    result = da_reserve(array, info, info->count + 1);

    for (size_t i = 0; i < info->item_size; ++i) {
        *((char*)result + offset + i) = *((char*)item + i);
    }
    info->count++;

    return result;
}


void da_free(void *array, array_info_t *info) {
    info->allocator->free(info->alloc_ctx, array, info->capacity * info->item_size);
}

#endif /* ifndef DA_H */
