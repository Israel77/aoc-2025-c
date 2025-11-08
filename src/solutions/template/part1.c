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
    pthread_barrier_t barrier = ctx->common->barrier;
    string_t *input = ctx->common->input;
    string_t output = ctx->common->output;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    printf("Not implemented yet!");

    return NULL;
}

