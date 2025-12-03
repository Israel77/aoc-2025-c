#include "prelude.h"
#include <assert.h>
#include <stdatomic.h>
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#define PART_1_IMPL

#define P1_THREADS 16

#define P1_MAX_RANGES 500

/* Shared data between threads */
struct p1_data {
    range_inclusive_t ranges[P1_MAX_RANGES];
    u16 range_count;
    atomic_uint_fast64_t invalid_total;
};

static p1_data p1;

/* Functions for part 1 */
internal void p1_count_invalid_ids(struct part_context*);

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    if (thread_idx == 0) {
        p1_setup(ctx);
    }

    sync_all(ctx);
    
    p1_count_invalid_ids(ctx);

    sync_all(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p1.invalid_total, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_setup(struct part_context *ctx) {

    string_t *input = ctx->common->input;

    string_t to_parse = *input;

    while (to_parse.count > 0) {

        range_inclusive_t new_range;

        new_range.start = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, '-');

        new_range.end   = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, ',');
        skip_whitespace(to_parse, &to_parse);

        p1.ranges[p1.range_count++] = new_range;
    }
}

internal void p1_count_invalid_ids(struct part_context *ctx) {

    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;
    
    const size_t tasks_per_thread = p1.range_count / thread_count;
    const size_t remainders = p1.range_count % thread_count;

    const bool take_remainder = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * tasks_per_thread + prev_remainders;
    const size_t end = start + tasks_per_thread + (take_remainder ? 1 : 0);


    for (size_t i = start; i < end; ++i) {
        u8 local_buffer[256];
        allocator_t local_arena = {
            .interface = &arena_interface,
            .alloc_ctx = arena_from_buf(local_buffer, 256)
        };

        range_inclusive_t range = p1.ranges[i];

        for (u64 curr = range.start; curr <= range.end; ++curr) {
            arena_reset(local_arena.alloc_ctx);

            string_builder_t sb = sb_from_u64(curr, &local_arena);

            if (sb.array_info.count % 2 != 0) {
                continue;
            }

            u8 mid = sb.array_info.count/2;

            /* 128-bit = 16x8-bit, we expect less than 20 characters per number,
             * therefore less than 10 characters per half */
            __m128i a = {0};
            __m128i b = {0};
            memcpy(&a, sb.items, mid);
            memcpy(&b, &sb.items[mid], mid);

            __mmask32 mask = _mm_cmp_epu8_mask(a, b, _MM_CMPINT_EQ);
            if (mask == 0xFFFF)
                atomic_fetch_add(&p1.invalid_total, curr);
        }
    }
}
