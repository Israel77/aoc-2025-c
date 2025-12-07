#include "prelude.h"
#define TEST_IMPL

static arena_context_t test_arena;

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

struct p1_test_data {};
struct p2_test_data {};

static inline void run_tests() {

    test_arena = arena_init(1024, ARENA_MALLOC_BACKEND | ARENA_FAST_ALLOC | ARENA_GROWABLE, NULL, NULL);

    test_p1();
    arena_reset(&test_arena);

    test_p2();

    arena_destroy(&test_arena);
}

static inline void test_p1(void) {
    string_t input = string_from_cstr("");

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

    string_t expected = string_from_cstr("");

    if(string_equals(&expected, &test_common.output)) {
        TEST_OK("Day X part 1");
    } else {
        TEST_FAIL("Day X part 1");
        printf("Expected %.*s, got %.*s\n", (int)expected.count, expected.chars,
                (int)test_common.output.count, test_common.output.chars);
    }
}

static inline void test_p2(void) {
    string_t input = string_from_cstr("");

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

    string_t expected = string_from_cstr("");

    if(string_equals(&expected, &test_common.output)) {
        TEST_OK("Day X part 2");
    } else {
        TEST_FAIL("Day X part 2");
        printf("Expected %.*s, got %.*s\n", (int)expected.count, expected.chars,
                (int)test_common.output.count, test_common.output.chars);
    }
}
