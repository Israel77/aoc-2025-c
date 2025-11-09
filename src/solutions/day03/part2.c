#include "prelude.h"
#define PART_2_IMPL

#define P2_THREADS 1

/* Shared data between threads */
struct p2_data {
    uint16_t num_lines;
    uint16_t num_bits;
    uint16_t oxygen_generator_rating;
    uint16_t co2_scrubber_rating;
};

static p2_data p2;

/* Functions for part 2 */
static inline void calculate_oxygen_rating(string_t *input);
static inline void calculate_co2_rating(string_t *input);
static inline void p2_setup(struct part_context *ctx);

void *p2_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    string_t *input = ctx->common->input;
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    /* Single threaded solution */

    if (thread_idx == 0) {
        p2_setup(ctx);
        calculate_oxygen_rating(input);
    }

    if (thread_count < 2 || thread_idx == 1) {
        calculate_co2_rating(input);
    }


    pthread_barrier_wait(&ctx->common->barrier);

    if (thread_idx == thread_count-1) {
        uint32_t result = p2.co2_scrubber_rating * p2.oxygen_generator_rating;

        string_builder_t sb = sb_from_u64(result, &arena_allocator, ctx->common->arena);

        ctx->common->output = sb_build(&sb);
    }

    return NULL;
}

static inline int8_t get_bit(string_t *input, size_t line, size_t bit) {
    return input->chars[(p2.num_bits + 1) * line + bit] - '0';
}

static inline void p2_setup(struct part_context *ctx) {
    if (ctx->common->is_test) {
        struct test_data *t = (struct test_data*)(ctx->common->test_data);
        p2.num_bits  = t->p2_bits;
        p2.num_lines = t->p2_lines;
    }
    else {
        p2.num_bits  = 12;
        p2.num_lines = 1000;
    }
}

static inline void calculate_oxygen_rating(string_t *input) {


    uint16_t oxy_lines[p2.num_lines];
    uint16_t oxy_count = p2.num_lines;

    /* Fill the line indices */
    for (size_t i = 0; i < p2.num_lines; ++i) {
        oxy_lines[i] = i;
    }

    uint8_t current_bit = 0;
    while (oxy_count > 1) {
        uint16_t count_bits[2] = {0};
        size_t next_count = 0;

        for (size_t i = 0; i < oxy_count; ++i) {
            count_bits[get_bit(input, oxy_lines[i], current_bit)]++;
        }

        uint8_t most_common_bit = count_bits[0] > count_bits[1] ? 0 : 1;

        for (size_t i = 0; i < oxy_count; ++i) {
            if (get_bit(input, oxy_lines[i], current_bit) == most_common_bit) {
                oxy_lines[next_count++] = oxy_lines[i];
            }
        }

        oxy_count = next_count;
        ++current_bit;
    }

    p2.oxygen_generator_rating = 0;
    for (uint8_t b = 0; b < p2.num_bits; ++b) {
        uint16_t shift_by = (p2.num_bits - b - 1);
        p2.oxygen_generator_rating |= get_bit(input, oxy_lines[0], b) << shift_by;
    }
}

static inline void calculate_co2_rating(string_t *input) {

    uint16_t co2_lines[p2.num_lines];
    uint16_t co2_count = p2.num_lines;

    for (size_t i = 0; i < p2.num_lines; ++i) {
        co2_lines[i] = i;
    }

    uint8_t current_bit = 0;
    while (co2_count > 1) {
        uint16_t count_bits[2] = {0};
        size_t next_count = 0;

        for (size_t i = 0; i < co2_count; ++i) {
            count_bits[get_bit(input, co2_lines[i], current_bit)]++;
        }

        uint8_t least_common_bit = count_bits[0] <= count_bits[1] ? 0 : 1;

        for (size_t i = 0; i < co2_count; ++i) {
            if (get_bit(input, co2_lines[i], current_bit) == least_common_bit) {
                co2_lines[next_count++] = co2_lines[i];
            }
        }

        co2_count = next_count;
        ++current_bit;
    }


    p2.co2_scrubber_rating = 0;
    for (uint8_t b = 0; b < p2.num_bits; ++b) {
        uint16_t shift_by = (p2.num_bits - b - 1);
        p2.co2_scrubber_rating |= get_bit(input, co2_lines[0], b) << shift_by;
    }
}
