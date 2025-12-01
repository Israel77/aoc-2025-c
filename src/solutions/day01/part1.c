#include "prelude.h"
#define PART_1_IMPL

#define P1_THREADS 1

/* Shared data between threads */
struct p1_data {
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


    i16 result = 50;
    u16 count = 0;
    string_t parsed_input = *input;

    while (parsed_input.count > 0) {

        bool positive;
        if (parsed_input.chars[0] == 'R') {
            positive = true;
        } else {
            positive = false;
        }

        skip_n_chars(parsed_input, &parsed_input, 1);
        
        i16 number = parse_i16(parsed_input, &parsed_input);

        skip_whitespace(parsed_input, &parsed_input);

        if (positive) {
            result += number;
        } else {
            result -= number;
        }

        result = (result + 100) % 100;

        if (result == 0) count++;
    }


    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(count, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_setup(struct part_context *ctx) {
    UNUSED(ctx);
}
