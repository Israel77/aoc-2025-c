
#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#define ALLOC_FIXED_POOL_IMPL
#include "../allocator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../test.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void test_primitive(void) {
    /* Initialise a pool that can hold 10 ints */
    fixed_pool_context_t pool = fixed_pool_init(sizeof(int), 10,
                                                &global_std_allocator, NULL);

    int *p = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    if (p) { 
        TEST_OK("Fixed pool alloc int");
    } else {
        TEST_FAIL("Fixed pool alloc int");
    };
    *p = 777;
    if (*p == 777) {
        TEST_OK("Fixed pool write/read int");
    } else {
        TEST_FAIL("Fixed pool write/read int");
    }

    fixed_pool_allocator.free(&pool, p, sizeof(int));


    /* Clean up */
    fixed_pool_free_all(&pool);
}

typedef struct {
    int   id;
    float f;
    char  tag[8];
} test_struct_t;

static void test_struct(void) {
    const size_t pool_len = 5;

    fixed_pool_context_t pool = fixed_pool_init(sizeof(test_struct_t), pool_len,
                                                &global_std_allocator, NULL);

    test_struct_t *s = (test_struct_t *)fixed_pool_allocator.alloc(&pool,
                                                                    sizeof(test_struct_t));
    if (s) {
        TEST_OK("fixed pool alloc struct");
    } else {
        TEST_FAIL("fixed pool alloc struct");
    }
    s->id = 42;
    s->f  = 3.14f;
    memcpy(s->tag, "POOL", 5);
    if (s->id != 42 || s->f != 3.14f || memcmp(s->tag, "POOL", 5) != 0) {
        TEST_FAIL("fixed pool struct fields");
    } else {
        TEST_OK("fixed pool struct fields");
    }

    fixed_pool_allocator.free(&pool, s, sizeof(test_struct_t));
    fixed_pool_free_all(&pool);
}

static void test_exhaustion_and_reuse(void) {
    const size_t pool_len = 3;

    fixed_pool_context_t pool = fixed_pool_init(sizeof(int), pool_len,
                                                &global_std_allocator, NULL);

    int *a = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    int *b = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    int *c = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    if (!a || !b || !c) {
        TEST_FAIL("fixed pool allocate up to capacity");
    } else {
        TEST_OK("fixed pool allocate up to capacity");
    }

    /* Pool should now be exhausted */
    int *d = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    if (d) {
        TEST_FAIL("fixed pool try to allocate beyond capacity");
    } else {
        TEST_OK("fixed pool try to allocate beyond capacity");
    }

    /* Free one slot and allocate again – should succeed */
    fixed_pool_allocator.free(&pool, b, sizeof(int));
    int *e = (int *)fixed_pool_allocator.alloc(&pool, sizeof(int));
    if (!e) {
        TEST_FAIL("fixed pool reuse after free");
    } else {
        TEST_OK("fixed pool reuse after free");
    }

    /* Clean up remaining allocations */
    fixed_pool_allocator.free(&pool, a, sizeof(int));
    fixed_pool_allocator.free(&pool, c, sizeof(int));
    fixed_pool_allocator.free(&pool, e, sizeof(int));

    fixed_pool_free_all(&pool);
}

int main(void) {

    printf("--- Start tests: Fixed pool allocator ---\n");

    test_primitive();
    test_struct();
    test_exhaustion_and_reuse();

    printf("--- Summary: Fixed pool allocator ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return 0;
}

