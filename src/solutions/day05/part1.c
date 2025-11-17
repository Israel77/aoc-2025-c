#include "prelude.h"
#include <pthread.h>
#include <stddef.h>
#define PART_1_IMPL

#define P1_THREADS 1

#define MAX_GRID_ROWS 1000
#define MAX_GRID_COLS 1000

struct vec2 {
    uint16_t x;
    uint16_t y;
};

struct line {
    struct vec2 begin;
    struct vec2 end;
};

/* Shared data between threads */
struct p1_data {
    string_array_t file_lines;

    struct line lines[500];
    atomic_size_t line_count;

    arena_context_t *common_arena;

    uint64_t marked_points_once[(MAX_GRID_ROWS * MAX_GRID_COLS) / 64];
    uint64_t marked_points_many[(MAX_GRID_ROWS * MAX_GRID_COLS) / 64];
    atomic_uint_fast16_t overlap_count;
};

static p1_data p1;

/* Functions for part 1 */
static inline void p1_count_overlaps(struct part_context *ctx);

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    // if (thread_idx > 0) return NULL;

    // PROF_START("setup")
    p1_setup(ctx);
    // PROF_END

    sync(ctx);

    p1_count_overlaps(ctx);

    sync(ctx);

    if (thread_idx == thread_count - 1) {
        
        string_builder_t sb = sb_from_u64(p1.overlap_count, &arena_allocator, ctx->common->arena);

        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline bool is_marked_once(uint16_t x, uint16_t y) {

    uint64_t offset = MAX_GRID_COLS * y + x;
    uint64_t idx = offset / 64;
    uint64_t bit = 1ull << (offset % 64);

    return p1.marked_points_once[idx] & bit;
}

static inline bool is_marked_many(uint16_t x, uint16_t y) {

    uint64_t offset = MAX_GRID_COLS * y + x;
    uint64_t idx = offset / 64;
    uint64_t bit = 1ull << (offset % 64);

    return p1.marked_points_many[idx] & bit;
}

static inline void mark_point(uint16_t x, uint16_t y) {

    uint64_t offset = MAX_GRID_COLS * y + x;
    uint64_t idx = offset / 64;
    uint64_t bit = 1ull << (offset % 64);

    p1.marked_points_many[idx] |= p1.marked_points_once[idx] & bit;
    p1.marked_points_once[idx] |= bit;
}

static inline void p1_count_overlaps(struct part_context *ctx) {

    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx = ctx->thread_idx;

    const size_t tasks_per_thread = p1.line_count / thread_count;
    const size_t remaining = p1.line_count % thread_count;

    const size_t start = tasks_per_thread * thread_idx;
    const size_t end   = start + tasks_per_thread + (thread_idx < remaining);

    /* @parallelizable */
    for (size_t i = start; i < end; ++i) {

        struct line line = p1.lines[i];

        uint16_t start_coord = 0;
        uint16_t end_coord   = 0;
        uint16_t x;
        uint16_t y;

        if (line.begin.x == line.end.x) {

            x = line.begin.x;

            start_coord = min(line.begin.y, line.end.y);
            end_coord   = max(line.begin.y, line.end.y);

            for (y = start_coord; y <= end_coord; ++y) {

                if (is_marked_once(x, y) && !is_marked_many(x,y)) {
                    /* @shared_write */
                    atomic_fetch_add(&p1.overlap_count, 1);
                }

                /* @shared_write */
                mark_point(x, y);
            }
        }
        else if (line.begin.y == line.end.y) {

            y = line.begin.y;

            start_coord = min(line.begin.x, line.end.x);
            end_coord   = max(line.begin.x, line.end.x);

            for (x = start_coord; x <= end_coord; ++x) {
                if (is_marked_once(x, y) && !is_marked_many(x,y)) {
                    /* @shared_write */
                    atomic_fetch_add(&p1.overlap_count, 1);
                }

                /* @shared_write */
                mark_point(x, y);
            }
        }
    }
}


static inline void p1_setup(struct part_context *ctx) {

    string_t *const input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx = ctx->thread_idx;

    if (thread_idx == 0) {
        memset(&p1, 0, sizeof (p1));
        p1.common_arena = ctx->common->arena;
    }

    sync(ctx);

    const size_t tasks_per_thread = input->count / thread_count;
    const size_t remaining = input->count % thread_count;

    const size_t start = tasks_per_thread * thread_idx;
    const size_t end   = start + tasks_per_thread + (thread_idx < remaining);

    /* Parse the input */
    string_t cursor;
    cursor.chars = &input->chars[start];
    cursor.count = input->count - start;

    while (cursor.chars < input->chars + end) {

        struct vec2 v1, v2;

        /* Synchronize starting point */
        while (thread_idx != 0 && (cursor.chars[-1] != '\n')) {
            skip_n_chars(&cursor, &cursor, 1);
        }

        /* Baseline: 957,596 -> 957,182 */

        /* [957],596 -> 957,182 */
        v1.x = parse_u16(&cursor, &cursor);

        /* [,]596 -> 957,182 */
        skip_n_chars(&cursor, &cursor, 1);

        /* [596] -> 957,182 */
        v1.y = parse_u16(&cursor, &cursor);

        /* [ -> ]957,182 */
        skip_n_chars(&cursor, &cursor, 4);

        /* [957],182 */
        v2.x = parse_u16(&cursor, &cursor);

        /* [,]182 */
        skip_n_chars(&cursor, &cursor, 1);

        /* [182] */
        v2.y = parse_u16(&cursor, &cursor);

        skip_char(&cursor, &cursor, '\n');

        if (atomic_load(&p1.line_count) < 500) {
        /* @shared_read */
        /* @shared_write */
        p1.lines[p1.line_count].begin = v1;
        p1.lines[p1.line_count].end = v2;

        /* @shared_write */
        atomic_fetch_add(&p1.line_count, 1);
        }
    }

}
