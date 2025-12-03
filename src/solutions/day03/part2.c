#include "prelude.h"
#include <stdatomic.h>
#include <stddef.h>
#define PART_2_IMPL

#define P2_THREADS 1
#define CHARS_PER_LINE 101
#define LINE_COUNT 200

/* Shared data between threads */
struct p2_data {
    atomic_uint_fast64_t total_joltage;
};

static p2_data p2;
static inline void p2_calculate(struct part_context *ctx);

/* Functions for part 2 */
static inline void p2_find_earliest_biggest_digit(string_t input, size_t pos, u8 *digit, u8 *index);

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    UNUSED(p2);
    UNUSED(input);

    p2_calculate(ctx);
    sync_all(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p2.total_joltage, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}


static inline void p2_calculate(struct part_context *ctx) {

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

        u64 line_joltage = 0;

        string_t line = {
            .chars = &input->chars[line_idx * CHARS_PER_LINE],
            .count = CHARS_PER_LINE
        };

        u64 pow_10 =  10l * 10 * 10 * 10
                    * 10l * 10 * 10 * 10
                    * 10l * 10 * 10;

        for (u8 pos = 12; pos > 0; --pos) {
            u8 digit;
            u8 index;

            p2_find_earliest_biggest_digit(line, pos, &digit, &index);
            skip_n_chars(line, &line, index + 1);

            line_joltage += pow_10 * digit;
            pow_10 /= 10;
        }

        local_joltage += line_joltage;
    }

    atomic_fetch_add(&p2.total_joltage, local_joltage);
}

static inline void p2_find_earliest_biggest_digit(string_t input, size_t pos, u8 *digit, u8 *index) {

    *digit = 0;
    *index = 0;

    size_t curr_index = 0;
    while (input.count > pos) {
        u8 curr_digit = parse_digit(input, &input);
        if (curr_digit > *digit) {
            *digit = curr_digit;
            *index = curr_index;
        }
        curr_index++;
    }
}
