#include "prelude.h"
#include <immintrin.h>
#include <pthread.h>
#include <stddef.h>

#define P1_THREADS 1

/* Shared data between threads */
struct p1_data {
    int64_t xs[P1_THREADS];
    int64_t ys[P1_THREADS];
    atomic_int_fast64_t total_xs;
    atomic_int_fast64_t total_ys;
    string_array_t strings;
};


static p1_data p1;

/* Functions for part 1 */
static inline void p1_fill_xs_and_ys(size_t thread_idx, size_t thread_count);
static inline void p1_sum_xs(size_t thread_idx, size_t thread_count);
static inline void p1_sum_ys(size_t thread_idx, size_t thread_count);

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    multiarena_context_t *arena = ctx->common->arena;
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    p1.total_xs = 0;
    p1.total_ys = 0;

    /* Parse lines */
    if (thread_idx == 0) {
        p1.strings = string_split_by_char(input, '\n', &multiarena_allocator, arena);
    }

    pthread_barrier_wait(&ctx->common->barrier);

    /* Fill xs and ys */
    p1_fill_xs_and_ys(thread_idx, thread_count);

    pthread_barrier_wait(&ctx->common->barrier);

    /* Sum xs */
    p1_sum_xs(thread_idx, thread_count);

    /* Sum ys */
    p1_sum_ys(thread_idx, thread_count);

    pthread_barrier_wait(&ctx->common->barrier);

    if (thread_idx == 0) {
        uint64_t result = p1.total_xs * p1.total_ys;
        string_builder_t sb = sb_from_u64(result, &multiarena_allocator, arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_sum_xs(size_t thread_idx, size_t thread_count) {
    if (thread_count > 1 && thread_idx % 2 != 0) return;

    uint64_t sum = 0;
    sum += p1.xs[thread_idx];

    if (thread_idx + 1 < thread_count)
        sum += p1.xs[thread_idx + 1];

    atomic_fetch_add(&p1.total_xs, sum);
}

static inline void p1_sum_ys(size_t thread_idx, size_t thread_count) {
    if (thread_count > 1 && thread_idx % 2 == 0) return;

    uint64_t sum = 0;
    if (thread_idx > 0)
        sum += p1.ys[thread_idx - 1];
    
    if (thread_idx < thread_count)
        sum += p1.ys[thread_idx];

    atomic_fetch_add(&p1.total_ys, sum);
}


static inline void p1_fill_xs_and_ys(size_t thread_idx, size_t thread_count) {

    const size_t tasks_per_thread = p1.strings.array_info.count / thread_count;
    const size_t remainders = p1.strings.array_info.count % thread_count;

    const bool take_remainder = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * tasks_per_thread + prev_remainders;
    const size_t end = start + tasks_per_thread + (take_remainder ? 1 : 0);

    for (size_t i = start; i < end; ++i) {
        string_t current_line = p1.strings.items[i];
        if (current_line.count == 0) break;
        char id = current_line.chars[0];

        size_t j = 0;
        do {
            ++j;
            assert(j < current_line.count);
        } while (current_line.chars[j] != ' ');

        ++j;

        switch (id) {
        case 'f':
            p1.xs[thread_idx] += current_line.chars[j] - '0';
            break;
        case 'u':
            p1.ys[thread_idx] -= current_line.chars[j] - '0';
            break;
        case 'd':
            p1.ys[thread_idx] += current_line.chars[j] - '0';
            break;
        }
    }
}
