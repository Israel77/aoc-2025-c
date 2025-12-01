#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 1

/* Shared data between threads */
struct p2_data {
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

    i16 result = 50;
    u16 count = 0;
    string_t parsed_input = *input;

    while (parsed_input.count > 0) {

        bool positive_rotation;
        if (parsed_input.chars[0] == 'R') {
            positive_rotation = true;
        } else {
            positive_rotation = false;
        }

        skip_n_chars(parsed_input, &parsed_input, 1);
        
        i16 number = parse_i16(parsed_input, &parsed_input);

        skip_whitespace(parsed_input, &parsed_input);

        i32 next_result = positive_rotation ? result + number : result - number;

        if (next_result > 0) {
            /* Count the number of rotations */
            count += next_result / 100;
        } else if (next_result < 0) {
            /* Add 1 to account to the extra pass */
            count += (-next_result / 100) + (result == 0 ? 0 : 1);
        } else {
            /* If landed in 0, count as 1 pass */
            count++;
        }

        result = (next_result + 100) % 100;
    }


    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(count, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}
