#include "prelude.h"
#include <stddef.h>
#define PART_2_IMPL

#define P2_THREADS 1

/* Shared data between threads */
struct p2_data {
    range_inclusive_t ranges[MAX_RANGE_COUNT];
    size_t range_count;
};

static p2_data p2;

/* Functions for part 2 */

void *p2_solve(void *arg) {
    struct part_context *ctx = arg;

    /* IO and synchronization */
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    if (thread_idx == 0) {
        p2_setup(ctx);
    }

    parallel_partial_sort(ctx, p2.ranges, p2.range_count);

    sync_all(ctx);

    /* Single threaded merging step */
    if (thread_idx == 0) {
        size_t step = p2.range_count / thread_count;
        for (size_t i = 1; i < thread_count; ++i) {
            size_t left = 0;
            size_t mid = i * step + 1;
            size_t right = (i + 1) * step + 1 < p2.range_count ? (i + 1) * step + 1: p2.range_count;
            merge_array(p2.ranges, left, mid, right);
        }
        merge_ranges(p2.ranges, &p2.range_count);
    }

    sync_all(ctx);

    u64 result = 0;
    for (size_t i = 0; i < p2.range_count; ++i) {
        u64 length = p2.ranges[i].end - p2.ranges[i].start + 1;
        result += length;
    }

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(result, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p2_setup(struct part_context *ctx) {

    const string_t *input = ctx->common->input;
    string_t to_parse = *input;

    /* Parse ranges */
    while (to_parse.chars[0] != '\n') {
        range_inclusive_t range;

        range.start = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, '-');

        range.end = parse_u64(to_parse, &to_parse);

        p2.ranges[p2.range_count++] = range;
        skip_char(to_parse, &to_parse, '\n');
    }
}
