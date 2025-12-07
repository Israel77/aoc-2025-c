#ifndef PRELUDE_H
#define PRELUDE_H
/* ----------------------------------------
 * Time measurement
 * ---------------------------------------- */
#define _POSIX_C_SOURCE 200112L
#include <time.h>

/* ----------------------------------------
 * Standard headers
 * ---------------------------------------- */
#include <stdlib.h> // IWYU pragma: export
#include <stdio.h> // IWYU pragma: export
#include <stdint.h> // IWYU pragma: export
#include <stddef.h> // IWYU pragma: export
#include <assert.h> // IWYU pragma: export
#include <threads.h> // IWYU pragma: export
#include <stdatomic.h> // IWYU pragma: export
#include <locale.h> // IWYU pragma: export
#include <pthread.h> // IWYU pragma: export
#include <stdatomic.h> // IWYU pragma: export

/* Allocators */
#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#ifndef ALLOC_ARENA_IMPL
#define ALLOC_ARENA_IMPL
#endif
#include "../../utils/allocator.h" // IWYU pragma: export

/* Dynamic arrays */
#include "../../utils/da.h" // IWYU pragma: export

/* Split stuff evenly */
#include "../../utils/splits.h" // IWYU pragma: export

/* Strings */
#define STRING_UTILS_IMPL
#include "../../utils/string_utils.h" // IWYU pragma: export
#include "../../utils/parsing_helpers.h" // IWYU pragma: export


/* Structure for testing */
typedef struct p1_test_data p1_test_data;
typedef struct p2_test_data p2_test_data;

static inline void test_p1();
static inline void test_p2();

/* Infrastructure for each part */
struct part_context_common {
    pthread_barrier_t barrier;
    size_t            thread_count;
    string_t          output;
    string_t         *input;
    allocator_t      *arena;
    void             *test_data;
    bool             is_test;
};

struct part_context {
    size_t thread_idx;
    struct part_context_common *common;
};

/* Data shared between threads for each part */
typedef struct p1_data p1_data;
typedef struct p2_data p2_data;


static inline void p1_setup(struct part_context *ctx);
void *p1_solve(void *ctx);

static inline void p2_setup(struct part_context *ctx);
void *p2_solve(void *ctx);

/* Common utilities */
internal inline void sync_all(struct part_context *ctx) {
    if (ctx->common->thread_count > 1) pthread_barrier_wait(&ctx->common->barrier);
}

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

typedef struct {
    u64 start;
    u64 end;
} range_inclusive_t;

/* Merges splitted arrays, where  each split is already sorted */
/* full array indices     = [left, right) */
/* left  subarray indices = [left, mid) */
/* right subarray indices = [mid, right) */
internal void merge_array(range_inclusive_t *array, size_t left, size_t mid, size_t right) {

    range_inclusive_t aux[right - left];

    size_t i = left;
    size_t j = mid;

    for (size_t k = left; k < right; k++) {
        // If left run head exists and is <= existing right run head.
        if (i < mid && (j >= right || array[i].start <= array[j].start)) {
            aux[k] = array[i];
            i = i + 1;
        } else {
            aux[k] = array[j];
            j = j + 1;
        }
    }

    for (size_t i = left; i < right; ++i)
        array[i] = aux[i];
}

/* Sorts a subset of the array */
internal void sort_array(range_inclusive_t *array, size_t begin_idx, size_t end_idx) {
    /* Selection sort is fine for small amount of items */

    for (size_t i = begin_idx; i < end_idx; ++i) {
        size_t min_idx = i;
        for (size_t j = i+1; j < end_idx; ++j) {
            if (array[j].start < array[min_idx].start) {
                min_idx = j;
            }
        }

        if (min_idx != i) {
            range_inclusive_t temp = array[i];
            array[i] = array[min_idx];
            array[min_idx] = temp;
        }
    }
}

internal void parallel_partial_sort(struct part_context *ctx, range_inclusive_t *array, size_t array_count) {
    size_t begin_idx;
    size_t end_idx;
    split_by_thread(ctx, array_count, &begin_idx, &end_idx);

    sort_array(array, begin_idx, end_idx);
}

/* Unrelated to mergesort (but assumes the array is sorted to work) */
internal void merge_ranges(range_inclusive_t *ranges, size_t *range_count) {

    range_inclusive_t merged_ranges[*range_count];

    size_t merged_count = 0;
    for (size_t i = 0; i < *range_count; ++i) {
        /* Skip empty ranges */
        if (ranges[i].start == 0 && ranges[i].end == 0) continue;

        /* First valid range */

        /* Merge with last range */
        if (unlikely(merged_count == 0) || ranges[i].start > (merged_ranges[merged_count - 1].end + 1)) {
            merged_ranges[merged_count++] = ranges[i];
        } else {
            merged_ranges[merged_count - 1].end = max(ranges[i].end, merged_ranges[merged_count - 1].end);
        }
    }

    *range_count = merged_count;
    for (size_t i = 0; i < *range_count; ++i)
        ranges[i] = merged_ranges[i];
}

#endif /* ifndef PRELUDE_H */
