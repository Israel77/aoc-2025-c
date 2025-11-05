#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include "string_utils.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Implementation of common equality and hash functions for use with
 * hashmaps.
 */

/* Works for any integer type with rank 64-bit or less */
static inline bool int64_eq(const void *a, const void *b) {
    return *(uint64_t *)a == *(uint64_t *)b;
}

static inline uint64_t int64_hash(const void *key) {
    return *(uint64_t*)key;
}

#ifdef STRING_UTILS_IMPL
static inline bool string_eq(const void *a, const void *b) {
    return string_equals((string_t *)a, (string_t *)b);
}

static inline uint64_t string_hash(const void *key) {
    const string_t *s = (const string_t *)key;
    uint64_t h = 0;
    for (size_t i = 0; i < s->count; ++i) {
        h = h * 31 + (unsigned char)s->chars[i];
    }
    return h;
}
#endif /* #ifdef STRING_UTILS_IMPL */

#ifdef BIGINT_IMPL
static inline bool bigint_eq(void *a, void *b) {
    return bigint_equals((bigint_t *)a, (bigint_t *)b);
}

static inline uint64_t bigint_hash(const void *key) {

    static const uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
    static const uint64_t FNV_PRIME = 0x100000001b3ULL;

    const bigint_t *bigint = key;
    uint64_t h = FNV_OFFSET;
    h ^= bigint->is_negative;

    for (size_t i = 0; i < bigint->array_info.count; ++i) {
        uint64_t value = bigint->items[i];
        for (size_t b = 0; b < 8; ++b) {
            h = (value >> (8 * i)) & 0xFF;
            h *= FNV_PRIME;
        }
    }

    return h;
}
#endif /* #ifdef BIGINT_IMPL */

#endif /* #ifndef HASH_UTILS_H */

