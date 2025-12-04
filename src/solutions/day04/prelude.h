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

internal inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

enum point_type {
    EMPTY = 0,
    PAPER_ROLL,
    COUNT
};

#endif /* ifndef PRELUDE_H */
