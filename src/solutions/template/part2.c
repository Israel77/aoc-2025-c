#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 2

/* Shared data between threads */
struct p2_data {
};

static p2_data p2;

/* Functions for part 2 */

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    pthread_barrier_t barrier = ctx->common->barrier;
    string_t *input = ctx->common->input;
    string_t output = ctx->common->output;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    printf("Not implemented yet!");

    return NULL;
}
