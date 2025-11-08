#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 1

struct p2_data {
    string_array_t strings;
    uint32_t  nums[2000];
    uint32_t  sums[2000];
    atomic_uint_fast16_t num_count;
    atomic_uint_fast16_t result;
};

static struct p2_data p2;

/* Functions for part 2 */
static string_array_t p2_split_input(struct part_context *ctx);

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    bool is_multithreaded = ctx->common->thread_count > 1;

    if (ctx->thread_idx == 0) {
        p2.strings = p2_split_input(ctx);
    }

    sync(ctx);

    const size_t tasks_per_thread = p2.num_count / ctx->common->thread_count;
    const size_t remaining = p2.num_count % ctx->common->thread_count;

    const size_t start = tasks_per_thread * ctx->thread_idx;
    const size_t end   = start + tasks_per_thread + (ctx->thread_idx < remaining);

    for (size_t i = start; i < end; ++i) {
        p2.nums[i] = string_parse_u64_unsafe(&p2.strings.items[i], NULL);   
    }

    for (size_t i = start; i < end; ++i) {
        p2.sums[i] = p2.nums[i]
            + (i + 1 < p2.num_count) * p2.nums[i+1]
            + (i + 2 < p2.num_count) * p2.nums[i+2];   
    }

    sync(ctx);

    uint16_t local_incr = 0;
    for (size_t i = start; i < end; ++i) {

        if (i > 0 && (p2.sums[i] > p2.sums[i-1]))
            local_incr++;
    }

    if (is_multithreaded) {
        atomic_fetch_add(&p2.result, local_incr);
    } else {
        p2.result = local_incr;
    }

    sync(ctx);

    if (ctx-> thread_idx == 0) {
        string_builder_t sb = sb_from_u64(p2.result, &arena_allocator, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}


static string_array_t p2_split_input(struct part_context *ctx) {

    string_array_t str = string_split_by_char(ctx->common->input, '\n', &arena_allocator, ctx->common->arena);

    p2.num_count = str.array_info.count;

    return str;
}
