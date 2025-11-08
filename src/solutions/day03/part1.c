#include "prelude.h"
#include <pthread.h>
#define PART_1_IMPL

#define P1_THREADS 1

#define LINES 1000
#define BITS 12

/* Shared data between threads */
struct p1_data {
    uint16_t numbers[LINES];
    atomic_uint_fast16_t bits[BITS];
};

static p1_data p1 = {0};

/* Functions for part 1 */
static inline void process_numbers(string_t *input, size_t thread_idx, size_t thread_count);
static inline void update_bits();

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    /* Process the numbers */
    process_numbers(input, thread_idx, thread_count);
    pthread_barrier_wait(&ctx->common->barrier);

    if (thread_idx == 0) {
        /* Find the most common bits */
        update_bits();

        /* Assemble the final value */
        uint16_t gamma = 0;
        for (size_t i = 0; i < BITS; ++i) {
            gamma |= (p1.bits[i] & 1) << (BITS - i - 1);
        }

        // bit_count = 3
        // 1 << 3 = 1000
        // 1 << 3 - 1 = 0111;
        uint16_t epsilon = ~gamma & ((1 << BITS) - 1);
        uint64_t value = epsilon * gamma;

        string_builder_t sb = sb_from_u64(value, &arena_allocator, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void process_numbers(string_t *input, size_t thread_idx, size_t  thread_count) {

    const size_t tasks_per_thread = LINES / thread_count;
    const size_t remainders = LINES % thread_count;

    const bool take_remainder = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * tasks_per_thread + prev_remainders;
    const size_t end = start + tasks_per_thread + (take_remainder ? 1 : 0);

    uint16_t local_bits[BITS] = {0};

    for (size_t i = start; i < end; ++i) {
        size_t offset = (BITS + 1) * i;
        for (size_t j = 0; j < BITS; ++j) {
            local_bits[j] += input->chars[offset + j] - '0';
        }
    }
    for (size_t i = 0; i < BITS; ++i) {
        atomic_fetch_add_explicit(&p1.bits[i], local_bits[i], memory_order_relaxed);
    }

}

static inline void update_bits() {

    for (size_t i = 0; i < BITS; ++i) {
        if (p1.bits[i] > LINES / 2) p1.bits[i] = 1;
        else p1.bits[i] = 0;
    }
}
