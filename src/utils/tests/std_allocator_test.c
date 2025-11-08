#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#include "../allocator.h"
#include "../macros.h"
#include <stdio.h>
#include <string.h>

/* Simple struct for testing */
typedef struct {
    int   a;
    float b;
    char  c[8];
} test_struct_t;

static int tests_passed = 0;
static int tests_failed = 0;

/* Verify that a pointer returned by the allocator is non‑NULL and usable */
static void test_primitive(void) {
    int *p = (int *)global_std_allocator.alloc(NULL, sizeof(int));
    if (p) {
        TEST_OK("alloc int");
    } else {
        TEST_FAIL("alloc int");
    }
    *p = 42;
    if (*p == 42) {
        TEST_OK("write/read int");
    } else {
        TEST_FAIL("write/read int");
    }
    global_std_allocator.free(NULL, p, sizeof(int));
}

/* Verify allocation of an array */
static void test_array(void) {
    size_t n = 10;
    double *arr = (double *)global_std_allocator.alloc(NULL, n * sizeof(double));
    if (arr) {
        TEST_OK("alloc array");
    } else {
        TEST_FAIL("alloc array");
    }
    for (size_t i = 0; i < n; ++i) arr[i] = (double)i * 1.5;
    bool content_ok = true;
    for (size_t i = 0; i < n; ++i)
        if (arr[i] != (double)i * 1.5) content_ok = false;

    if (content_ok) {
        TEST_OK("array contents");
    } else {
        TEST_FAIL("array contents");
    }
    global_std_allocator.free(NULL, arr, n * sizeof(double));
}

/* Verify allocation of a struct */
static void test_struct(void) {
    test_struct_t *s = (test_struct_t *)global_std_allocator.alloc(NULL, sizeof(test_struct_t));
    if (s) {
        TEST_OK("alloc struct");
    } else {
        TEST_FAIL("alloc struct");
    }
    s->a = -7;
    s->b = 3.14f;
    memcpy(s->c, "hello", 6);

    if (s->a != -7 || s->b != 3.14f || memcmp(s->c, "hello", 6) != 0) {
        TEST_FAIL("struct fields");
    } else {
        TEST_OK("struct fields");
    }
    global_std_allocator.free(NULL, s, sizeof(test_struct_t));
}

/* Verify realloc works (grow and shrink) */
static void test_realloc(void) {
    size_t initial = 5;
    int *p = (int *)global_std_allocator.alloc(NULL, initial * sizeof(int));
    if (p) {
        TEST_OK("alloc for realloc");
    } else {
        TEST_FAIL("alloc for realloc");
    }
    for (size_t i = 0; i < initial; ++i) p[i] = (int)i;

    size_t larger = 12;
    p = (int *)global_std_allocator.realloc(NULL, p, initial * sizeof(int),
                                            larger * sizeof(int));
    if (p) {
        TEST_OK("realloc grow");
    } else {
        TEST_FAIL("realloc grow");
    }

    bool preserve_data = true;
    for (size_t i = 0; i < initial; ++i)
        preserve_data &= (p[i] == (int)i);

    if (preserve_data) {
        TEST_OK("realloc preserve data (grow)");
    } else {
        TEST_FAIL("realloc preserve data (grow)");
    }

    size_t smaller = 3;
    p = (int *)global_std_allocator.realloc(NULL, p, larger * sizeof(int),
                                            smaller * sizeof(int));
    if (p) {
        TEST_OK("realloc shrink");
    } else {
        TEST_FAIL("realloc shrink");
    }

    preserve_data = true;
    for (size_t i = 0; i < smaller; ++i)
        preserve_data &= (p[i] == (int)i);

    if (preserve_data) {
        TEST_OK("realloc preserve data (shrink)");
    } else {
        TEST_FAIL("realloc preserve data (shrink)");
    }

    global_std_allocator.free(NULL, p, smaller * sizeof(int));
}

/* Main driver */
int main(void) {

    printf("--- Start tests: Global STD Allocator ---\n");
    test_primitive();
    test_array();
    test_struct();
    test_realloc();


    printf("--- Summary: Global STD Allocator ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");



    return 0;
}

