#ifndef DA_H
#define DA_H

#include "allocator.h"
#include <stdbool.h>
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
    const allocator_t *allocator;
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

static inline bool da_equals(const void *array1, const array_info_t *info1,
        const void *array2, const array_info_t *info2) {

    if (info1->count != info2->count) {
        return false;
    }

    if (info1->item_size != info2->item_size) {
        return false;
    }

    /* memcmp(array1, array2, info->item_size); */
    
    /* Calculate how many 64-bit registers fit into each item */
    size_t word_size = sizeof (uint64_t);

    size_t words_per_item = info1->item_size / word_size;
    size_t tail_bytes = info1->item_size % word_size;

    for (size_t i = 0; i < (info1->count * info1->item_size); i += info1->item_size) {

        const uint8_t *p1 = (uint8_t*)array1 + i;
        const uint8_t *p2 = (uint8_t*)array2 + i;

        /* Optimized check for 64-bit architecture */
        const uint64_t *w1 = (uint64_t *)p1;
        const uint64_t *w2 = (uint64_t *)p2;
        for (size_t word = 0; word < words_per_item; ++word) {
            if (w1[word] != w2[word]) {
                return false;
            }
        }

        /* Check the remaining bytes */
        const uint8_t *b1 = p1 + words_per_item * sizeof (uint64_t);
        const uint8_t *b2 = p2 + words_per_item * sizeof (uint64_t);
        for (size_t byte = 0; byte < tail_bytes; ++byte) {
            if (b1[byte] != b2[byte]) {
                return false;
            }
        }
    }

    return true;
}

static void da_free(void *array, array_info_t *info) {
    info->allocator->free(info->alloc_ctx, array, info->capacity * info->item_size);
}

#endif /* ifndef DA_H */
