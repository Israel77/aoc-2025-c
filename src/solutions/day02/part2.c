#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 1

/* Shared data between threads */
struct p2_data {
};

static p2_data p2;

/* Functions for part 2 */

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input     = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    /* Multithreading does not make sense for part 2 */
    if (thread_idx > 0) return NULL;

    string_array_t strings = string_split_by_char(input, '\n', &multiarena_allocator, ctx->common->arena);

    uint64_t aim = 0;
    uint64_t x = 0;
    uint64_t y = 0;

    for (size_t i = 0; i < strings.array_info.count; ++i) {
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
            y += aim * (current_line.chars[j] - '0');
            x += current_line.chars[j] - '0';
            break;
        case 'u':
            aim -= current_line.chars[j] - '0';
            break;
        case 'd':
            aim += current_line.chars[j] - '0';
            break;
        }
    }


    uint64_t prod = x * y;
    string_builder_t sb = sb_from_u64(prod, &multiarena_allocator, ctx->common->arena);
    ctx->common->output = sb_build(&sb);

    return NULL;
}
