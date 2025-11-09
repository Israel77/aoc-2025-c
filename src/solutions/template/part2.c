#include "prelude.h"
//#define PART_2_IMPL

#define P2_THREADS 1

/* Shared data between threads */
struct p2_data {
};

static p2_data p2;

/* Functions for part 2 */

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    pthread_barrier_t *barrier = &ctx->common->barrier;
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    if (thread_idx == 0) {
        p2_setup(ctx);
    }

    pthread_barrier_wait(barrier);

    if (thread_idx == thread_count - 1) {
        ctx->common->output = string_from_cstr("Not implemented yet!");
    }

    return NULL;
}

static inline void p2_setup(struct part_context *ctx) {
}
