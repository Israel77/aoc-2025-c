#include "prelude.h"
#include <stddef.h>
#define PART_1_IMPL

#define P1_THREADS 1
#define MAX_STACKS 1000
#define MAX_LINES 5

enum __attribute__((packed)) operators {
    ADD = '+',
    MUL = '*'
};

/* Shared data between threads */
struct p1_data {
    u64 values[MAX_STACKS][MAX_LINES];
    /* Single operator per stack */
    enum operators ops[MAX_STACKS];
    u16 stack_count;
    u8 line_count;
};

static p1_data p1;

/* Functions for part 1 */

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;


    UNUSED(p1);
    UNUSED(input);

    if (thread_idx == 0) {
        p1_setup(ctx);
    }

    sync_all(ctx);

    u64 total = 0;
    for (size_t i = 0; i < p1.stack_count; ++i) {
        size_t j = p1.line_count;

        u8 operator = p1.ops[i];
        u64 result;

        switch (operator) {
        case ADD:
            result = 0;
            break;
        case MUL:
            result = 1;
            break;
        }

        while (j > 0) {
            size_t row = j - 1;
            switch (operator) {
            case ADD:
                result += p1.values[i][row];
                break;
            case MUL:
                result *= p1.values[i][row];
                break;
            }
            --j;
        }

        total += result;
    }

    if (thread_idx == thread_count - 1) {
        string_builder_t sb = sb_from_u64(total, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline void p1_setup(struct part_context *ctx) {

    string_t *input = ctx->common->input;

    string_t to_parse = *input;

    size_t stack_count = 0;
    size_t line_count = 0;
    while (to_parse.count > 0) {
        stack_count = 0;
        while (to_parse.chars[0] == MUL || to_parse.chars[0] == ADD) {
            p1.ops[stack_count++] = to_parse.chars[0];       
            skip_n_chars(to_parse, &to_parse, 1);
            skip_whitespace(to_parse, &to_parse);

        }
        while (to_parse.count > 0 && to_parse.chars[0] != '\n') {
            p1.values[stack_count++][line_count] = parse_u64(to_parse, &to_parse);       
            skip_all_of(to_parse, &to_parse, " ", 1);
        }
        ++line_count;
        skip_whitespace(to_parse, &to_parse);
    }
    p1.stack_count = stack_count;
    /* Don't count the operator line */
    p1.line_count = line_count-1;
}
