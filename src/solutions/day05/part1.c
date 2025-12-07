#include "prelude.h"
#include <stddef.h>
#include <string.h>
#define PART_1_IMPL

#define P1_THREADS 1

#define MAX_RANGE_COUNT 200
#define MAX_ID_COUNT 2000

/* Shared data between threads */
struct p1_data {
    range_inclusive_t ranges[MAX_RANGE_COUNT];
    size_t range_count;
    u64 ids[MAX_ID_COUNT];
    size_t id_count;
};

static p1_data p1;

/* Functions for part 1 */


void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    if (thread_idx == 0) {
        p1_setup(ctx);
    }

    parallel_partial_sort(ctx, p1.ranges, p1.range_count);

    sync_all(ctx);

    /* Single threaded merging step */
    if (thread_idx == 0) {
        size_t step = p1.range_count / thread_count;
        for (size_t i = 1; i < thread_count; ++i) {
            size_t left = 0;
            size_t mid = i * step + 1;
            size_t right = (i + 1) * step + 1 < p1.range_count ? (i + 1) * step + 1: p1.range_count;
            merge_array(p1.ranges, left, mid, right);
        }
        merge_ranges(p1.ranges, &p1.range_count);
    }

    sync_all(ctx);

    u64 result = 0;
    for (size_t i = 0; i < p1.id_count; ++i) {

        u64 id = p1.ids[i];
        bool is_fresh = false;

        for (size_t j = 0; j < p1.range_count; ++j) {
            range_inclusive_t r = p1.ranges[j];
            if (r.start > id) break;
            if (r.start <= id && id <= r.end) {
                is_fresh = true;
                break;
            }
        }

        if (is_fresh) ++result;
    }

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(result, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_setup(struct part_context *ctx) {

    const string_t *input = ctx->common->input;
    string_t to_parse = *input;

    /* Parse ranges */
    while (to_parse.chars[0] != '\n') {
        range_inclusive_t range;

        range.start = parse_u64(to_parse, &to_parse);

        skip_char(to_parse, &to_parse, '-');

        range.end = parse_u64(to_parse, &to_parse);

        p1.ranges[p1.range_count++] = range;
        skip_char(to_parse, &to_parse, '\n');
    }

    skip_char(to_parse, &to_parse, '\n');

    /* Parse ids */
    while (to_parse.count > 0) {
        u64 id = parse_u64(to_parse, &to_parse);

        p1.ids[p1.id_count++] = id;
        skip_whitespace(to_parse, &to_parse);
    }
}
