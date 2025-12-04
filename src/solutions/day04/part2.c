#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 1

#define GRID_ROWS 135
#define GRID_COLS 135

/* Shared data between threads */
struct p2_data {
    u8 grid[GRID_ROWS][GRID_COLS];
    atomic_uint_least16_t accessible_rolls;
};

static p2_data p2;

/* Functions for part 2 */

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    UNUSED(p2);
    UNUSED(input);

    p2_setup(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p2.accessible_rolls, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline u8 p2_count_neighbors(size_t x, size_t y) {

    u8 result = 0;

    for (i8 dx = -1; dx <= 1; ++dx) {
        for (i8 dy = -1; dy <= 1; ++dy) {
            i16 check_x = x + dx;
            i16 check_y = y + dy;

            if ((dx != 0 || dy != 0) 
                    && (check_x >= 0 && check_x < GRID_COLS)
                    && (check_y >= 0 && check_y < GRID_ROWS)
                    && (p2.grid[check_y][check_x] == PAPER_ROLL)) {
                ++result;
            }
        }
    }

    return result;

}

static inline void p2_setup(struct part_context *ctx) {

    string_t *input = ctx->common->input;

    size_t start_row;
    size_t end_row;
    split_by_thread(ctx, GRID_ROWS, &start_row, &end_row);

    for (size_t y = start_row; y < end_row; ++y) {

        for (size_t x = 0; x < GRID_COLS; ++x) {

            size_t char_idx = (GRID_COLS + 1) * y + x;

            switch (input->chars[char_idx]){
            case '@':
                p2.grid[y][x] = PAPER_ROLL;
                break;
            case '.': 
            default:
                p2.grid[y][x] = EMPTY;
                break;
            }
        
        }
    }

    u32 locally_accessible;
    do {
        locally_accessible = 0;
        for (size_t y = start_row; y < end_row; ++y) {
            for (size_t x = 0; x < GRID_COLS; ++x) {
                if ((p2.grid[y][x]) && p2_count_neighbors(x, y) < 4) {
                    ++locally_accessible;
                    p2.grid[y][x] = EMPTY;
                }
            }
        }

        atomic_fetch_add(&p2.accessible_rolls, locally_accessible);
    } while (locally_accessible > 0);
}
