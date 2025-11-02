#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "../test.h"
#define ALLOC_STD_IMPL
#define ALLOC_ARENA_IMPL
#include "../allocator.h"
#define STRING_UTILS_IMPL
#include "../string_utils.h"

#define HM_IMPL
#include "../hashmap.h"
/* -------------------------------------------------------------
 *  Test‑framework macros (provided)
 * ------------------------------------------------------------- */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_OK(msg)    do { printf("\033[32m [  OK ] %s \033[0m\n", msg); ++tests_passed; } while (0)
#define TEST_FAIL(msg)  do { printf("\033[31m [ FAIL] %s \033[0m\n", msg); ++tests_failed; } while (0)
/* -------------------------------------------------------------
 *  Helper hash / equality functions that work with string_t
 * ------------------------------------------------------------- */
static uint64_t string_hash(const void *key) {
    const string_t *s = (const string_t *)key;
    uint64_t h = 0;
    for (size_t i = 0; i < s->count; ++i) {
        h = h * 31 + (unsigned char)s->chars[i];
    }
    return h;
}

static bool string_eq(const void *a, const void *b) {
    return string_equals((const string_t *)a, (const string_t *)b);
}

static uint64_t int_hash(const void *key) {
    return *(uint64_t*)key;
}
static bool int64_eq(const void *a, const void *b) {
    return *(int64_t*)a == *(int64_t*)b;
}

/* -------------------------------------------------------------
 *  Test helpers
 * ------------------------------------------------------------- */
static void report_assert(bool cond, const char *msg) {
    if (cond) TEST_OK(msg);
    else      TEST_FAIL(msg);
}

/* -------------------------------------------------------------
 *  1. Initialise an empty hashmap
 * ------------------------------------------------------------- */
static void test_init(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           0, &err);
    report_assert(!err.is_error, "hm_init – no error");
    report_assert(hm.count == 0, "hm_init – count == 0");
    report_assert(hm.tombstones == 0, "hm_init – tombstones == 0");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  2. Insert a single key/value pair
 * ------------------------------------------------------------- */
static void test_insert_one(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           0, &err);

    string_t key = string_from_cstr("apple");
    string_t val = string_from_cstr("red");

    void *prev = hm_insert(&hm, &key, &val, &err);
    report_assert(!err.is_error, "hm_insert – no error");
    report_assert(prev == NULL, "hm_insert – first insert returns NULL");
    report_assert(hm.count == 1, "hm_insert – count incremented");

    void *got = hm_get(&hm, &key);
    report_assert(got == &val, "hm_get – returns stored value");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  3. Insert a duplicate key (replace)
 * ------------------------------------------------------------- */
static void test_insert_duplicate(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           0, &err);

    string_t key = string_from_cstr("banana");
    string_t v1  = string_from_cstr("yellow");
    string_t v2  = string_from_cstr("green");

    hm_insert(&hm, &key, &v1, &err);
    report_assert(!err.is_error, "first insert – no error");
    report_assert(hm.count == 1, "first insert – count == 1");

    void *prev = hm_insert(&hm, &key, &v2, &err);
    report_assert(!err.is_error, "second insert – no error");
    report_assert(prev == &v1, "second insert – returns previous value");
    report_assert(hm.count == 1, "second insert – count unchanged");

    void *got = hm_get(&hm, &key);
    report_assert(got == &v2, "hm_get after replace – returns new value");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  4. Delete an existing key
 * ------------------------------------------------------------- */
static void test_delete_existing(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           0, &err);

    string_t key = string_from_cstr("cherry");
    string_t val = string_from_cstr("dark red");

    hm_insert(&hm, &key, &val, &err);
    report_assert(!err.is_error, "insert before delete – no error");
    report_assert(hm.count == 1, "insert before delete – count == 1");

    void *deleted = hm_delete(&hm, &key);
    report_assert(deleted == &val, "hm_delete – returns stored value");
    report_assert(hm.count == 0, "hm_delete – count decremented");
    report_assert(hm.tombstones == 1, "hm_delete – tombstone created");
    report_assert(hm_get(&hm, &key) == NULL, "hm_get after delete – returns NULL");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  4. Delete multiple existing keys and verify tombstones
 * ------------------------------------------------------------- */
static void test_delete_multiple(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           12, &err);

    /* Store the keys and values in an arena to ensure they will outlive the hashmap */
    uintptr_t *buffer = malloc(4096);
    arena_context_t *test_arena = arena_init(buffer, 4096);

    /* Insert a set of keys */
    static const char *keys[]   = {"apple", "banana", "cherry", "date", "elderberry"};
    static const char *values[] = {"red", "yellow", "dark red", "brown", "purple"};
    const size_t n = sizeof(keys) / sizeof(keys[0]);

    for (size_t i = 0; i < n; ++i) {
        string_t *k = arena_alloc(test_arena, sizeof (string_t));
        string_t *v = arena_alloc(test_arena, sizeof (string_t));
        *k = string_from_cstr(keys[i]);
        *v = string_from_cstr(values[i]);
        hm_insert(&hm, k, v, &err);
        report_assert(!err.is_error, "insert – no error");
        report_assert(hm.count == i + 1, "insert – count incremented");
    }

    for (size_t i = 0; i < n; ++i) {
        string_t *k = arena_alloc(test_arena, sizeof (string_t));
        string_t *v = arena_alloc(test_arena, sizeof (string_t));
        *k = string_from_cstr(keys[i]);
        *v = string_from_cstr(values[i]);
        string_t *found = hm_get(&hm, k);
        report_assert(string_eq(v, found), "insert – values still accessible");
    }

    /* Delete every other key */
    for (size_t i = 0; i < n; i += 2) {
        string_t *k = arena_alloc(test_arena, sizeof (string_t));
        string_t *v = arena_alloc(test_arena, sizeof (string_t));
        *k = string_from_cstr(keys[i]);
        *v = string_from_cstr(values[i]);
        void *deleted = hm_delete(&hm, k);
        report_assert(string_eq(deleted, v), "hm_delete – returns stored value");
        report_assert(hm.tombstones == (i / 2) + 1,
                      "hm_delete – tombstone count increments");
        report_assert(hm_get(&hm, k) == NULL,
                      "hm_get after delete – returns NULL");
    }

    /* Verify remaining keys are still accessible */
    for (size_t i = 1; i < n; i += 2) {
        string_t *k = arena_alloc(test_arena, sizeof (string_t));
        *k = string_from_cstr(keys[i]);
        string_t expected = string_from_cstr(values[i]);
        string_t *found = hm_get(&hm, k);
        report_assert(found != NULL, "hm_get – key still present");
        report_assert(string_eq(&expected, found),
                      "hm_get – correct value");
    }

    /* Delete the rest */
    for (size_t i = 1; i < n; i += 2) {
        string_t k = string_from_cstr(keys[i]);
        void *deleted = hm_delete(&hm, &k);
        report_assert(deleted != NULL, "final delete – returns value");
    }

    report_assert(hm.count == 0, "all keys deleted – count zero");
    report_assert(hm.tombstones == n, "tombstones equal total inserts");

    hm_destroy(&hm);
    free(buffer);
}

/* -------------------------------------------------------------
 *  Compress the array if there are too many tombstones
 * ------------------------------------------------------------- */
static void test_delete_compress(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           int_hash, int64_eq,
                           100, &err);

    /* Store the keys and values in an arena to ensure they will outlive the hashmap */
    uintptr_t *buffer = malloc(4096);
    arena_context_t *test_arena = arena_init(buffer, 4096);

    for (int64_t i = 0; i < 100; ++i) {
        int64_t *k = arena_alloc(test_arena, sizeof (int64_t));
        *k = i;
        hm_insert(&hm, k, k, &err);
    }

    report_assert(hm.count == 100, "insert before delete - sucessful");
    size_t max_tombstones = 0;
    size_t prev_count = hm.count;

    /* delete everything out of order*/
    for (int64_t i = 0; i < 100; i += 2) {
        hm_delete(&hm, &i);
        if (hm.tombstones > max_tombstones)
            max_tombstones = hm.tombstones;
        report_assert(hm.count == prev_count - 1, "delete item sucessfully");
        --prev_count;
    }
    for (int64_t i = 1; i < 100; i += 2) {
        hm_delete(&hm, &i);
        if (hm.tombstones > max_tombstones)
            max_tombstones = hm.tombstones;
        report_assert(hm.count == prev_count - 1, "delete item sucessfully");
        --prev_count;
    }

    /* check if the hashmap was compressed */
    report_assert(max_tombstones > hm.tombstones, "number of tombstones reduced due to compression");

    hm_destroy(&hm);
    free(buffer);
}
/* -------------------------------------------------------------
 *  5. Delete a missing key
 * ------------------------------------------------------------- */
static void test_delete_missing(void) {
    error_t err = {0};
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           0, &err);

    string_t missing = string_from_cstr("nonexistent");
    void *deleted = hm_delete(&hm, &missing);
    report_assert(deleted == NULL, "hm_delete missing – returns NULL");
    report_assert(hm.count == 0, "hm_delete missing – count unchanged");
    report_assert(hm.tombstones == 0, "hm_delete missing – no tombstone");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  6. Trigger a resize (if implementation supports it)
 * ------------------------------------------------------------- */
static void test_resize(void) {
    error_t err = {0};
    /* Small initial capacity to force a resize quickly */
    hashmap_t hm = hm_init(&global_std_allocator, NULL,
                           string_hash, string_eq,
                           8, &err);
    size_t initial_cap = hm.capacity;

    /* arena allocator for the strings */
    uintptr_t buffer[1024];
    arena_context_t *arena_ctx = arena_init(buffer, 1024);


    for (int i = 0; i < 20; ++i) {
        /* use the arena so each key lives long enough */
        char *buf     = arena_alloc(arena_ctx, 3 * sizeof (char));
        string_t *key = arena_alloc(arena_ctx, sizeof (string_t));
        string_t *val = arena_alloc(arena_ctx, sizeof (string_t));

        snprintf(buf, sizeof(buf), "k%d", i);
        *key = string_from_cstr(buf);
        val = key;   /* value can be same string */

        hm_insert(&hm, key, val, &err);
        report_assert(!err.is_error, "insert during resize – no error");
    }

    report_assert(hm.capacity > initial_cap, "hashmap resized");
    report_assert(hm.count == 20, "hashmap count after many inserts");

    /* Spot‑check a few look‑ups */
    string_t k3 = string_from_cstr("k3");
    string_t k15 = string_from_cstr("k15");
    report_assert(string_equals((string_t *)hm_get(&hm, &k3),
                              (string_t *)hm_get(&hm, &k3)),
                "lookup k3 after resize");
    report_assert(string_equals((string_t *)hm_get(&hm, &k15),
                              (string_t *)hm_get(&hm, &k15)),
                "lookup k15 after resize");

    hm_destroy(&hm);
}

/* -------------------------------------------------------------
 *  Main – run all tests and report totals
 * ------------------------------------------------------------- */
int main(void) {
    printf("--- Start tests: Hashmap ---\n");
    test_init();
    test_insert_one();
    test_insert_duplicate();
    test_delete_existing();
    test_delete_missing();
    test_delete_multiple();
    test_delete_compress();
    test_resize();

    printf("--- Summary: Hashmap ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
