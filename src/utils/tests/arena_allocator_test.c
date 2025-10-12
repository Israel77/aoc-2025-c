
#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#define ALLOC_ARENA_IMPL
#include "../allocator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../test.h"

static int tests_passed = 0;
static int tests_failed = 0;

static arena_context_t make_arena(void) {
    arena_context_t arena = {0};
    arena.inner_alloc = &global_std_allocator;   /* std allocator as backing */
    arena.inner_ctx   = NULL;
    return arena;
}

/* -------------------------------------------------------------------------
 * Test 1 – primitive allocation
 * ------------------------------------------------------------------------- */
static void test_primitive(void) {
    arena_context_t arena = make_arena();

    int *p = (int *)arena_allocator.alloc(&arena, sizeof(int));
    if (!p) {
        TEST_FAIL("arena alloc int");
    } else {
        TEST_OK("arena alloc int");
    }
    *p = 321;
    if (*p != 321) {
        TEST_FAIL("arena write/read int");
    } else {
        TEST_OK("arena alloc int");
    }

    arena_allocator.free(&arena, p, sizeof(int));   /* no‑op but allowed */

    arena_free_all(&arena);   /* clean up all chunks */
}

/* -------------------------------------------------------------------------
 * Test 2 – array allocation and realloc
 * ------------------------------------------------------------------------- */
static void test_array(void) {
    arena_context_t arena = make_arena();

    const size_t N = 32;
    double *arr = (double *)arena_allocator.alloc(&arena, N * sizeof(double));
    if (!arr) {
        TEST_FAIL("arena alloc array");
    } else {
        TEST_OK("arena alloc array");
    }

    bool arena_write_array = true;
    for (size_t i = 0; i < N; ++i) arr[i] = (double)i * 0.75;
    for (size_t i = 0; i < N; ++i) arena_write_array &= (arr[i] == (double)i * 0.75);
    if (arena_write_array) {
        TEST_OK("arena array contents");
    } else {
        TEST_FAIL("arena array contents");
    }

    /* Grow the array using arena_realloc (allocates a new block) */
    const size_t M = 64;
    double *arr2 = (double *)arena_allocator.realloc(&arena, arr,
                                                   N * sizeof(double),
                                                   M * sizeof(double));

    if (!arr2) {
        TEST_FAIL("arena realloc larger");
    } else {
        TEST_OK("arena realloc larger");
    }

    bool preserve_data = true;
    for (size_t i = 0; i < N * sizeof(double) / sizeof(double); ++i)
        preserve_data &= (arr2[i] == (double)i * 0.75);

    if (preserve_data) {
        TEST_OK("arena realloc preserve data");
    } else {
        TEST_FAIL("arena realloc preserve data");
    }

    bool arena_write_ok = true;
    for (size_t i = N; i < M; ++i) arr2[i] = (double)i * 0.5;
    for (size_t i = N; i < M; ++i) arena_write_ok &= (arr2[i] == (double)i * 0.5);
    if (arena_write_ok) {
        TEST_OK("arena new region write");
    } else {
        TEST_FAIL("arena new region write");
    }

    arena_allocator.free(&arena, arr2, M * sizeof(double));   /* no‑op */

    arena_free_all(&arena);
}

typedef struct {
    int   id;
    float f;
    char  name[12];
} test_struct_t;

static void test_struct(void) {
    arena_context_t arena = make_arena();

    test_struct_t *s = (test_struct_t *)arena_allocator.alloc(&arena,
                                                             sizeof(test_struct_t));
    if (!s) {
        TEST_FAIL("arena alloc struct");
    } else {
        TEST_OK("arena alloc struct");
    }

    s->id = 7;
    s->f  = 2.71f;
    memcpy(s->name, "arena_test", 0x0B);
    if (s->id != 7 || s->f != 2.71f || memcmp(s->name, "arena_test", 0x0B) != 0)
        TEST_FAIL("arena struct fields");

    arena_free_all(&arena);
}

/* -------------------------------------------------------------------------
 * Test 4 – multiple chunks, reset, and reuse
 * ------------------------------------------------------------------------- */
static void test_chunks_reset(void) {
    arena_context_t arena = make_arena();

    /* Force creation of several chunks by allocating many small blocks */
    const size_t block_sz = 256;
    const size_t blocks_needed = 20;   /* 20 * 256 = 5120 > default 4096 */
    void *ptrs[blocks_needed];

    for (size_t i = 0; i < blocks_needed; ++i) {
        ptrs[i] = arena_allocator.alloc(&arena, block_sz);
        if (!ptrs[i]) TEST_FAIL("arena alloc multiple chunks");
        memset(ptrs[i], (int)i, block_sz);
    }

    /* Verify data integrity */
    for (size_t i = 0; i < blocks_needed; ++i) {
        unsigned char *b = (unsigned char *)ptrs[i];
        for (size_t j = 0; j < block_sz; ++j)
            if (b[j] != (unsigned char)i) TEST_FAIL("arena chunk data");
    }

    /* Reset the arena – all previously allocated memory becomes reusable */
    arena_reset(&arena);

    /* After reset allocate new objects and ensure they work */
    int *x = (int *)arena_allocator.alloc(&arena, sizeof(int));
    if (!x) {
        TEST_FAIL("arena alloc after reset");
    }
    *x = 99;
    if (*x != 99) TEST_FAIL("arena write after reset");

    /* Allocate a larger block to ensure a new chunk can be created again */
    size_t large_sz = 8192;   /* larger than any existing chunk */
    void *large = arena_allocator.alloc(&arena, large_sz);
    if (!large) TEST_FAIL("arena alloc large after reset");
    memset(large, 0xAA, large_sz);

    arena_free_all(&arena);
}

/* -------------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------------- */
int main(void) {

    printf("--- Start tests: Arena allocator ---\n");

    test_primitive();
    test_array();
    test_struct();
    test_chunks_reset();

    printf("--- Summary: Arena allocator ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return 0;
}

