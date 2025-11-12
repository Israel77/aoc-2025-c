#include "prelude.h"
// #define TEST_IMPL

static arena_context_t test_arena;

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

struct p1_test_data {};
struct p2_test_data {};

static inline void run_tests() {

    test_arena = arena_init(1024, ARENA_VIRTUAL_BACKEND | ARENA_FAST_ALLOC | ARENA_GROWABLE, NULL, NULL);

    test_p1();
    arena_reset(&test_arena);

    test_p2();

    arena_destroy(&test_arena);
}

static inline void test_p1(void) {
    string_t input = string_from_cstr(
            "7,4,9,5,11,17,23,2,0,14,21,24,10,16,13,6,15,25,12,22,18,20,8,19,3,26,1\n"
            "\n"
            "22 13 17 11  0\n"
            " 8  2 23  4 24\n"
            "21  9 14 16  7\n"
            " 6 10  3 18  5\n"
            " 1 12 20 15 19\n"
            "\n"
            " 3 15  0  2 22\n"
            " 9 18 13 17  5\n"
            "19  8  7 25 23\n"
            "20 11 10 24  4\n"
            "14 21 16 12  6\n"
            "\n"
            "14 21 17 24  4\n"
            "10 16 15  9 19\n"
            "18  8 23 26 20\n"
            "22 11 13  6  5\n"
            " 2  0 12  3  7\n"
            );

    struct p1_test_data td = {
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

    p1_solve(&test_ctx);

    string_t expected = string_from_cstr("4512");

    if(string_equals(&expected, &test_common.output)) {
        TEST_OK("Day 4 part 1");
    } else {
        TEST_FAIL("Day 4 part 1");
        printf("Expected %.*s, got %.*s\n", (int)expected.count, expected.chars,
                (int)test_common.output.count, test_common.output.chars);
    }
}

static inline void test_p2(void) {

    string_t input = string_from_cstr(
            "7,4,9,5,11,17,23,2,0,14,21,24,10,16,13,6,15,25,12,22,18,20,8,19,3,26,1\n"
            "\n"
            "22 13 17 11  0\n"
            " 8  2 23  4 24\n"
            "21  9 14 16  7\n"
            " 6 10  3 18  5\n"
            " 1 12 20 15 19\n"
            "\n"
            " 3 15  0  2 22\n"
            " 9 18 13 17  5\n"
            "19  8  7 25 23\n"
            "20 11 10 24  4\n"
            "14 21 16 12  6\n"
            "\n"
            "14 21 17 24  4\n"
            "10 16 15  9 19\n"
            "18  8 23 26 20\n"
            "22 11 13  6  5\n"
            " 2  0 12  3  7\n"
            );

    struct p2_test_data td = {
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

    string_t expected = string_from_cstr("1924");

    if(string_equals(&expected, &test_common.output)) {
        TEST_OK("Day 4 part 2");
    } else {
        TEST_FAIL("Day 4 part 2");
        printf("Expected %.*s, got %.*s\n", (int)expected.count, expected.chars,
                (int)test_common.output.count, test_common.output.chars);
    }
}
