#include "prelude.h"
#define PART_1_IMPL

#define P1_THREADS 2

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
    UNUSED(thread_idx);
    UNUSED(thread_count);

    ctx->common->output = string_from_cstr("Not implemented yet!");

    return NULL;
}
