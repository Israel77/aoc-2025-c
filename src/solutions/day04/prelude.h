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
    arena_context_t  *arena;
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
static inline void sync(struct part_context *ctx) {
    if (ctx->common->thread_count > 1) pthread_barrier_wait(&ctx->common->barrier);
}

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Common structures for both parts */

#define MAX_BINGO_NUMS 100
#define BINGO_BOARD_ROWS 5
#define BINGO_BOARD_COLS 5

typedef struct {
    uint8_t value;
    bool    marked;
} board_t[BINGO_BOARD_COLS][BINGO_BOARD_ROWS];

typedef struct {
    array_info_t array_info;
    board_t *items;
} board_array_t;

typedef struct {
    uint8_t results[MAX_BINGO_NUMS];
    uint8_t result_count;
    board_array_t boards;
} bingo_t;

typedef struct {
    array_info_t array_info;
    uint8_t *items;
} u8_array_t;

static inline void mark_result(board_t board, uint8_t result) {

    for (size_t row = 0; row < BINGO_BOARD_ROWS; ++row) {
        for (size_t col = 0; col < BINGO_BOARD_COLS; ++col) {
            if (board[row][col].value == result)
                board[row][col].marked = true;
        }
    }
}

static inline bool is_winner(board_t board) {

    /* Check rows */
    for (size_t row = 0; row < BINGO_BOARD_ROWS; ++row) {
        bool winning_row = true;

        for (size_t col = 0; col < BINGO_BOARD_COLS; ++col) {
            winning_row &= board[row][col].marked;
        }

        if (winning_row) return true;
    }

    /* Check columns */
    for (size_t col = 0; col < BINGO_BOARD_COLS; ++col) {
        bool winning_col = true;

        for (size_t row = 0; row < BINGO_BOARD_ROWS; ++row) {
            winning_col &= board[row][col].marked;
        }

        if (winning_col) return true;
    }

    return false;
}


#endif /* ifndef PRELUDE_H */
