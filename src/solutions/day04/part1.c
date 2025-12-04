#include "prelude.h"
#include <stdatomic.h>
#include <stddef.h>
#include <threads.h>
#define PART_1_IMPL

#define P1_THREADS 1

#define GRID_ROWS 135
#define GRID_COLS 135

/* Shared data between threads */
struct p1_data {
    u8 grid[GRID_ROWS][GRID_COLS];
    atomic_uint_least16_t accessible_rolls;
};

static p1_data p1;

/* Functions for part 1 */

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    UNUSED(p1);
    UNUSED(input);

    p1_setup(ctx);

    sync_all(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p1.accessible_rolls, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline u8 p1_count_neighbors(size_t x, size_t y) {

    u8 result = 0;

    for (i8 dx = -1; dx <= 1; ++dx) {
        for (i8 dy = -1; dy <= 1; ++dy) {
            i16 check_x = x + dx;
            i16 check_y = y + dy;

            if ((dx != 0 || dy != 0) 
                    && (check_x >= 0 && check_x < GRID_COLS)
                    && (check_y >= 0 && check_y < GRID_ROWS)
                    && (p1.grid[check_y][check_x] == PAPER_ROLL)) {
                ++result;
            }
        }
    }

    return result;

}

static inline void p1_setup(struct part_context *ctx) {

    string_t *input = ctx->common->input;

    size_t start_row;
    size_t end_row;
    split_by_thread(ctx, GRID_ROWS, &start_row, &end_row);

    for (size_t y = start_row; y < end_row; ++y) {

        for (size_t x = 0; x < GRID_COLS; ++x) {

            size_t char_idx = (GRID_COLS + 1) * y + x;

            switch (input->chars[char_idx]){
            case '@':
                p1.grid[y][x] = PAPER_ROLL;
                break;
            case '.': 
            default:
                p1.grid[y][x] = EMPTY;
                break;
            }
        
        }
    }

    u32 locally_accessible = 0;
    for (size_t y = start_row; y < end_row; ++y) {
        for (size_t x = 0; x < GRID_COLS; ++x) {
            if ((p1.grid[y][x]) && p1_count_neighbors(x, y) < 4) {
                ++locally_accessible;
            }
        }
    }

    atomic_fetch_add(&p1.accessible_rolls, locally_accessible);
}
