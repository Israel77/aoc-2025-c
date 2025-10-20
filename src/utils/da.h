#ifndef DA_H
#define DA_H

#include "allocator.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif /* ifndef DA_INIT_CAP */

typedef struct {
    size_t item_size;
    size_t count;
    size_t capacity;
    size_t min_capacity;
    allocator_t *allocator;
    void *alloc_ctx;
} array_info_t;

static void *da_reserve(void *array, array_info_t *info, size_t expected_capacity) {
    assert(info->item_size != 0 && "You forgot to set the size");
    void *result = array;

    if ((expected_capacity) > info->capacity) {
        size_t old_size = info->count * info->item_size;

        if (info->min_capacity == 0) {
            info->min_capacity = DA_INIT_CAP;
        }

        if (info->capacity == 0) {
            info->capacity = info->min_capacity;
        }

        while ((expected_capacity) > info->capacity) {
            info->capacity *= 2;
        }

        result = info->allocator->realloc(info->alloc_ctx, array, old_size, info->capacity * info->item_size);
        assert(result != NULL && "Error on memory allocation.");
    }

     return result;
}

static void *da_append(void *array, array_info_t *info, const void *item) {
    void *result = array;

    size_t offset = info->count * info->item_size;
    result = da_reserve(array, info, info->count + 1);

    for (size_t i = 0; i < info->item_size; ++i) {
        *((char*)result + offset + i) = *((char*)item + i);
    }
    info->count++;

    return result;
}

static void da_reverse(void *array, const array_info_t *info) {
    uint8_t temp[info->item_size];

    uint8_t *start = (uint8_t*) array;
    uint8_t *end   = start + (info->count - 1) * info->item_size;

    for (size_t i = 0; i < info->count / 2; ++i) {
        size_t offset = i * info->item_size;
        // memcpy(temp          , start + offset, info->item_size);
        // memcpy(start + offset, end - offset  , info->item_size);
        // memcpy(end - offset  , temp          , info->item_size);
        for (size_t j = 0; j < info->item_size; ++j) {
            temp[j] = start[offset + j];
            start[offset +  j] = *(end - offset + j);
            *(end - offset + j) = temp[j];
        }
    }
}

static void da_free(void *array, array_info_t *info) {
    info->allocator->free(info->alloc_ctx, array, info->capacity * info->item_size);
}

#endif /* ifndef DA_H */
