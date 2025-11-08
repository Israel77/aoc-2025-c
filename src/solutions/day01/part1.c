#include "prelude.h"
#define PART_1_IMPL

#define P1_THREADS 1

typedef struct {
    array_info_t array_info;
    uint32_t *items;
} u32_array_t;

struct p1_data {
    u32_array_t nums;
    string_array_t strings;
    atomic_uint_fast16_t result;
};

static struct p1_data p1;

/* Functions for part 1 */
static string_array_t p1_split_input(struct part_context *ctx);

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    bool is_multithreaded = ctx->common->thread_count > 1;

    if (ctx->thread_idx == 0) {
        p1.strings = p1_split_input(ctx);
    }

    sync(ctx);

    const size_t tasks_per_thread = p1.strings.array_info.count / ctx->common->thread_count;
    const size_t remaining = p1.strings.array_info.count % ctx->common->thread_count;

    const size_t start = tasks_per_thread * ctx->thread_idx;
    const size_t end   = start + tasks_per_thread + (ctx->thread_idx < remaining);

    for (size_t i = start; i < end; ++i) {
        p1.nums.items[i] = string_parse_u64_unsafe(&p1.strings.items[i], NULL);   
    }

    uint16_t local_incr = 0;
    for (size_t i = start; i < end; ++i) {
        if (i!=0 && (p1.nums.items[i] > p1.nums.items[i-1])) {
            local_incr++;
        }
    }

    if (is_multithreaded) {
        atomic_fetch_add(&p1.result, local_incr);
    } else {
        p1.result = local_incr;
    }

    sync(ctx);

    if (ctx-> thread_idx == 0) {
        string_builder_t sb = sb_from_u64(p1.result, &arena_allocator, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}


static string_array_t p1_split_input(struct part_context *ctx) {

    array_info_t p1_arr_info = {
        .item_size = sizeof (*p1.nums.items),
        .allocator = &arena_allocator,
        .alloc_ctx = ctx->common->arena,
    };

    p1.nums.array_info = p1_arr_info;

    string_array_t str = string_split_by_char(ctx->common->input, '\n', &arena_allocator, ctx->common->arena);
    p1.nums.items = da_reserve(p1.nums.items, &p1.nums.array_info, str.array_info.count);

    assert(p1.nums.items && "Could not allocate memory");

    return str;
}
