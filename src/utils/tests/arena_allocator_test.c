#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#include <stddef.h>
#include <stdint.h>
#endif
#define ALLOC_ARENA_IMPL
#include "../allocator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../macros.h"

static int tests_passed = 0;
static int tests_failed = 0;

/* Test buffer for single region arenas */
static uint8_t test_buffer[8192];

/* -------------------------------------------------------------------------
 * Test primitive allocation
 * ------------------------------------------------------------------------- */
static void test_arena_primitive(void)
{
    arena_context_t *a = arena_from_buf(test_buffer, 8192);
    if (!a) { TEST_FAIL("arena init"); return; }

    int *p = (int *)arena_alloc(a, sizeof(int));
    if (!p) { TEST_FAIL("arena alloc int"); }
    else    { TEST_OK ("arena alloc int"); }

    *p = 1234;
    if (*p != 1234) TEST_FAIL("arena write/read int");
    else            TEST_OK ("arena write/read int");

    arena_free(a, p, sizeof(int));   /* no‑op */
    arena_reset(a);
    arena_free(a, NULL, 0);           /* still safe */
}

/* -------------------------------------------------------------------------
 * Test array allocation and realloc
 * ------------------------------------------------------------------------- */
static void test_arena_array(void) {
    arena_context_t *arena = arena_from_buf(test_buffer, 8192);
    if (!arena) { TEST_FAIL("arena init"); return; }

    /* Allocate 32‑element double array */
    const size_t n_initial = 32;
    double *a1 = (double *)arena_alloc(arena, n_initial * sizeof(double));
    if (!a1)  TEST_FAIL("arena alloc array");
    else      TEST_OK ("arena alloc array");

    /* Fill and verify */
    for (size_t i = 0; i < n_initial; ++i)
        a1[i] = (double)i * 0.75;

    bool ok = true;
    for (size_t i = 0; i < n_initial; ++i)
        ok &= (a1[i] == (double)i * 0.75);
    if (ok) TEST_OK ("arena write array");
    else    TEST_FAIL("arena write array");

    /* Grow to 64 elements – forces a new block */
    const size_t n_grown = 64;
    double *a2 = (double *)arena_realloc(arena,
                                         a1,
                                         n_initial * sizeof(double),
                                         n_grown   * sizeof(double));
    if (!a2) TEST_FAIL("arena realloc larger");
    else    TEST_OK ("arena realloc larger");

    /* Verify original data survived */
    ok = true;
    for (size_t i = 0; i < n_initial; ++i)
        ok &= (a2[i] == (double)i * 0.75);
    if (ok) TEST_OK ("arena realloc preserve data");
    else    TEST_FAIL("arena realloc preserve data");

    /* Initialise new elements and check */
    for (size_t i = n_initial; i < n_grown; ++i)
        a2[i] = (double)i * 0.5;

    ok = true;
    for (size_t i = n_initial; i < n_grown; ++i)
        ok &= (a2[i] == (double)i * 0.5);
    if (ok) TEST_OK ("arena new region write");
    else    TEST_FAIL("arena new region write");

    /* Clean up */
    arena_free(arena, a2, n_grown * sizeof(double));
    arena_reset(arena);
}


/* -------------------------------------------------------------------------
 * Test 3 – struct allocation
 * ------------------------------------------------------------------------- */
typedef struct {
    int   id;
    float f;
    char  name[12];
} test_struct_t;

static void test_arena_struct(void)
{
    arena_context_t *a= arena_from_buf(test_buffer, 8192);
    if (!a) { TEST_FAIL("arena init"); return; }

    test_struct_t *s = (test_struct_t *)arena_alloc(a, sizeof(test_struct_t));
    if (!s) { TEST_FAIL("arena alloc struct"); }
    else    { TEST_OK ("arena alloc struct"); }

    s->id = 42;
    s->f  = 3.14f;
    memcpy(s->name, "single_arena", 12);

    if (s->id != 42 || s->f != 3.14f ||
        memcmp(s->name, "single_arena", 12) != 0)
        TEST_FAIL("arena struct fields");
    else
        TEST_OK ("arena struct fields");

    arena_reset(a);
}

/* -------------------------------------------------------------------------
 * Test 4 – reset and reuse within the same buffer
 * ------------------------------------------------------------------------- */
static void test_arena_reset_reuse(void)
{
    const size_t cap = 8192;
    uint8_t *buffer = malloc(cap * sizeof (uint8_t));
    arena_context_t *a = arena_from_buf(buffer, cap);
    if (!a) { TEST_FAIL("arena init"); return; }

    /* Allocate many small blocks to fill the buffer */
    const size_t block_sz = 256;
    const size_t blocks = cap / block_sz - 1;   /* exactly fills */
    void *ptrs[64];                         /* enough for the test */

    for (size_t i = 0; i < blocks; ++i) {
        ptrs[i] = arena_alloc(a, block_sz);
        if (!ptrs[i]) { TEST_FAIL("arena alloc many blocks"); break; }
        memset(ptrs[i], (int)i, block_sz);
    }

    /* Verify data */
    for (size_t i = 0; i < blocks; ++i) {
        unsigned char *b = (unsigned char *)ptrs[i];
        for (size_t j = 0; j < block_sz; ++j)
            if (b[j] != (unsigned char)i) { TEST_FAIL("arena block data"); break; }
    }

    /* Reset – buffer becomes reusable */
    arena_reset(a);

    /* Allocate again and ensure it works */
    int *x = (int *)arena_alloc(a, sizeof(int));
    if (!x) { TEST_FAIL("arena alloc after reset"); }
    else {
        *x = 777;
        if (*x != 777) TEST_FAIL("arena write after reset");
        else            TEST_OK ("arena write after reset");
    }

    /* Allocate a large block that forces the allocator to use the remaining space */
    size_t large_sz = cap / 2;
    void *large = arena_alloc(a, large_sz);
    if (!large) TEST_FAIL("arena large after reset");
    else {
        memset(large, 0xCC, large_sz);
        TEST_OK ("arena large after reset");
    }

}

static void test_primitive(void) {
    arena_context_t arena = arena_init(4096, ARENA_MALLOC_BACKEND | ARENA_GROWABLE | ARENA_FAST_ALLOC, NULL, NULL);

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

    arena_destroy(&arena);   /* clean up all chunks */
}

static void test_array(void) {
    arena_context_t arena = arena_init(4096, ARENA_MALLOC_BACKEND | ARENA_GROWABLE | ARENA_FAST_ALLOC, NULL, NULL);

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

    arena_destroy(&arena);
}

static void test_struct(void) {
    arena_context_t arena = arena_init(4096, ARENA_MALLOC_BACKEND | ARENA_GROWABLE | ARENA_FAST_ALLOC, NULL, NULL);

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

    arena_destroy(&arena);
}

static void test_chunks_reset(void) {
    arena_context_t arena = arena_init(4096, ARENA_MALLOC_BACKEND | ARENA_GROWABLE | ARENA_FAST_ALLOC, NULL, NULL);

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

    arena_destroy(&arena);
}

/* -------------------------------------------------------------------------
 * Main – run both arena families
 * ------------------------------------------------------------------------- */
int main(void)
{
    printf("--- Start tests: Single‑region arena ---\n");
    test_arena_primitive();
    test_arena_array();
    test_arena_struct();
    test_arena_reset_reuse();

    printf("--- Summary: Single‑region arena ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    printf("--- Start tests: Multi‑region arena ---\n");
    tests_failed = 0;
    tests_passed = 0;
    test_primitive();
    test_array();
    test_struct();
    test_chunks_reset();

    printf("--- Summary: Multi‑region arena ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    return 0;
}

