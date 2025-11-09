#include "prelude.h"
#define TEST_IMPL

static arena_context_t test_arena;

static inline void test_p2();

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

static inline void run_tests() {

    test_arena = arena_init(1024, ARENA_MALLOC_BACKEND | ARENA_FAST_ALLOC | ARENA_GROWABLE, NULL, NULL);

    test_p2();

    arena_destroy(&test_arena);
}

static inline void test_p2(void) {
    string_t input = string_from_cstr(
            "00100\n"
            "11110\n"
            "10110\n"
            "10111\n"
            "10101\n"
            "01111\n"
            "00111\n"
            "11100\n"
            "10000\n"
            "11001\n"
            "00010\n"
            "01010\n");

    struct test_data td = {
        .p2_bits = 5,
        .p2_lines = 12
    };

    struct part_context_common test_common = {
        .is_test      = true,
        .test_data    = &td,
        .arena        = &test_arena,
        .input        = &input,
        .thread_count = 1,
    };

    struct part_context test_ctx = {
        .common = &test_common,
        .thread_idx = 0
    };

    pthread_barrier_init(&test_common.barrier, NULL, 1);

    p2_solve(&test_ctx);

    string_t expected = string_from_cstr("230");

    if(string_equals(&expected, &test_common.output)) {
        TEST_OK("Day 3 part 2");
    } else {
        TEST_FAIL("Day 3 part 2");
        printf("Expected %.*s, got %.*s\n", (int)expected.count, expected.chars,
                (int)test_common.output.count, test_common.output.chars);
    }
}
