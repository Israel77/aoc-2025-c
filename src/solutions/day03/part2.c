#include "prelude.h"
// #define PART_2_IMPL

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
    UNUSED(input);
    UNUSED(thread_idx);
    UNUSED(thread_count);

    ctx->common->output = string_from_cstr("Not implemented yet!");

    return NULL;
}
