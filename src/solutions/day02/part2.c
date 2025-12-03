#include "prelude.h"
#include <stddef.h>
#define PART_2_IMPL
#define range_inclusive_len(range) (range.end - range.start + 1)

#define P2_THREADS 8

#define P2_MAX_RANGES 500

/* Shared data between threads */
struct p2_data {
    range_inclusive_t ranges[P2_MAX_RANGES];
    u16 range_count;
    atomic_uint_fast64_t invalid_total;
};

static p2_data p2;

/* Functions for part 2 */
internal void p2_count_invalid_ids(struct part_context*);

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    UNUSED(input);

    if (thread_idx == 0) {
        p2_setup(ctx);
    }

    sync_all(ctx);

    p2_count_invalid_ids(ctx);

    sync_all(ctx);

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(p2.invalid_total, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

internal inline void p2_setup(struct part_context *ctx) {

    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;

    string_t to_parse = *input;

    range_inclusive_t preliminar_ranges[P2_MAX_RANGES];
    size_t preliminar_range_count = 0;

    while (to_parse.count > 0) {

        range_inclusive_t new_range;

        new_range.start = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, '-');

        new_range.end   = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, ',');
        skip_whitespace(to_parse, &to_parse);

        preliminar_ranges[preliminar_range_count++] = new_range;
    }

    /* Average range per thread */
    u64 avg_range_size = 0;
    for (size_t i = 0; i < preliminar_range_count; ++i) {
        avg_range_size += (preliminar_ranges[i].end - preliminar_ranges[i].start + 1) / (preliminar_range_count * thread_count);
    }

    /* Running difference between range sizes and average range size */
    u64 remaining_size = avg_range_size;
    /* Split ranges equally */
    for (size_t i = 0; i < preliminar_range_count; ++i) {
        /* Split the range if it is too big */
        range_inclusive_t current_range = preliminar_ranges[i];
        if (remaining_size < (current_range.end - current_range.start)) {
            range_inclusive_t lower_range = {
                .start = current_range.start,
                .end   = current_range.start + remaining_size
            };
            range_inclusive_t upper_range = {
                .start   = current_range.start + remaining_size + 1,
                .end     = current_range.end
            };
            p2.ranges[p2.range_count++] = lower_range;
            p2.ranges[p2.range_count++] = upper_range;

            if (range_inclusive_len(upper_range) > avg_range_size) {
                remaining_size = avg_range_size;
            } else {
                remaining_size = avg_range_size - range_inclusive_len(upper_range);
            }
        } else {
            /* If there is no need to split, just add the range length to the running difference */
            remaining_size -= range_inclusive_len(preliminar_ranges[i]);
            p2.ranges[p2.range_count++] = current_range;
        }

        /* If the remainder is too small, reset it immediately */
        if (remaining_size <= ctx->common->thread_count) {
            remaining_size = avg_range_size;
        }

    }
}

internal void p2_count_invalid_ids(struct part_context *ctx) {

    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;
    
    const size_t tasks_per_thread = p2.range_count / thread_count;
    const size_t remainders = p2.range_count % thread_count;

    const bool take_remainder = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * tasks_per_thread + prev_remainders;
    const size_t end = start + tasks_per_thread + (take_remainder ? 1 : 0);

    u64 local_sum = 0;
    for (size_t i = start; i < end; ++i) {

        range_inclusive_t range = p2.ranges[i];

        for (u64 curr = range.start; curr <= range.end; ++curr) {
            u8 local_buffer[256];
            allocator_t local_arena = {
                .interface = &arena_interface,
                .alloc_ctx = arena_from_buf(local_buffer, 256)
            };

            string_builder_t sb = sb_from_u64(curr, &local_arena);

            // Brute force
            bool is_invalid = false;
            for (u8 num_splits = 2; num_splits <= sb.array_info.count; ++num_splits) {
                if (sb.array_info.count % num_splits != 0) continue;

                u8 split_size = (u8) (sb.array_info.count / num_splits);

                bool all_splits_equal = true;
                for (u8 c_idx = 0; c_idx < split_size; ++c_idx) {
                    bool all_digits_equal = true;
                    for (u8 s_idx = 0; s_idx < num_splits; ++s_idx) {
                        if (sb.items[s_idx * split_size + c_idx] != sb.items[c_idx]) {
                            all_digits_equal = false; break;
                        }
                    }
                    all_splits_equal &= all_digits_equal;
                }
                is_invalid |= all_splits_equal;
            }
            
            if (is_invalid) {
                local_sum += curr;
            }
        }
    }
    atomic_fetch_add(&p2.invalid_total, local_sum);
}
