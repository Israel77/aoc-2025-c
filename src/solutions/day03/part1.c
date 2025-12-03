#include "prelude.h"
#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#define PART_1_IMPL

#define P1_THREADS 1
#define CHARS_PER_LINE 101
#define LINE_COUNT 200

/* Shared data between threads */
struct p1_data {
    atomic_uint_fast64_t total_joltage;
};

static p1_data p1;

/* Functions for part 1 */
static inline void p1_calculate(struct part_context *ctx);

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    UNUSED(p1);
    UNUSED(input);

    p1_calculate(ctx);
    sync_all(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p1.total_joltage, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_calculate(struct part_context *ctx) {

    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;
    
    const size_t tasks_per_thread = LINE_COUNT / thread_count;
    const size_t remainders = LINE_COUNT % thread_count;

    const bool take_remainder = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start_line = thread_idx * tasks_per_thread + prev_remainders;
    const size_t end_line = start_line + tasks_per_thread + (take_remainder ? 1 : 0);

    u64 local_joltage = 0;
    for (size_t line_idx = start_line; line_idx < end_line; ++line_idx) {

        string_t line = {
            .chars = &input->chars[line_idx * CHARS_PER_LINE],
            .count = CHARS_PER_LINE
        };

        u64 first_digit  = 0;
        u64 first_idx    = 0;
        u64 second_digit = 0;
        u64 second_idx   = 0;
        u64 curr_idx     = 0;

    }

    atomic_fetch_add(&p1.total_joltage, local_joltage);
}
