#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifndef STD_ALLOC_IMPL
#define STD_ALLOC_IMPL
#endif
#ifndef ALLOC_ARENA_IMPL
#define ALLOC_ARENA_IMPL
#endif
#include "../allocator.h"

#define STRING_UTILS_IMPL
#include "../string_utils.h"

#include "../error.h"
#include "../test.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void test_sb_from_cstr(void) {
    const char *src = "hello world";

    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t sb = sb_from_cstr(src, a, ctx);

    bool ok = true;
    ok &= (sb.array_info.count == strlen(src));
    ok &= (sb.array_info.capacity >= sb.array_info.count + 1);
    ok &= (memcmp(sb.items, src, sb.array_info.count) == 0);
    ok &= (sb.items[sb.array_info.count] == '\0');   /* builder is NUL‑terminated */

    if (ok) TEST_OK("sb_from_cstr basic functionality");
    else    TEST_FAIL("sb_from_cstr basic functionality");

    da_free(sb.items, &sb.array_info);
}

static void test_sb_with_capacity(void) {
    const size_t req_cap = 10;
    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t sb = sb_with_capacity(req_cap, a, ctx);

    bool ok = true;
    ok &= (sb.array_info.count == 0);
    ok &= (sb.array_info.capacity >= req_cap);
    ok &= (sb.items[0] == '\0');   /* empty builder is NUL‑terminated */

    if (ok) TEST_OK("sb_with_capacity basic functionality");
    else    TEST_FAIL("sb_with_capacity basic functionality");

    a->free(ctx, sb.items, sb.array_info.capacity * sizeof(char));
}

static void test_sb_append_char(void) {
    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t sb = sb_with_capacity(2, a, ctx);

    sb_append_char(&sb, 'A');
    sb_append_char(&sb, 'B');
    sb_append_char(&sb, 'C');   /* forces growth via da_reserve */

    bool ok = true;
    ok &= (sb.array_info.count == 3);
    ok &= (memcmp(sb.items, "ABC", 3) == 0);
    ok &= (sb.items[3] == '\0');

    if (ok) TEST_OK("sb_append_char with growth");
    else    TEST_FAIL("sb_append_char with growth");

    a->free(ctx, sb.items, sb.array_info.capacity * sizeof(char));
}

static void test_sb_append_cstr(void) {
    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t sb = sb_with_capacity(5, a, ctx);

    sb_append_cstr(&sb, "foo");
    sb_append_cstr(&sb, "barbaz");   /* longer than remaining capacity */

    bool ok = true;
    ok &= (sb.array_info.count == 9);
    ok &= (memcmp(sb.items, "foobarbaz", 9) == 0);
    ok &= (sb.items[9] == '\0');

    if (ok) TEST_OK("sb_append_cstr with reallocation");
    else    TEST_FAIL("sb_append_cstr with reallocation");

    a->free(ctx, sb.items, sb.array_info.capacity * sizeof(char));
}

static void test_sb_append_str_sized(void) {
    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t sb = sb_with_capacity(0, a, ctx);

    string_t part1 = string_from_cstr("Hello, ");
    string_t part2 = string_from_cstr("World!");

    sb_append_str(&sb, part1);
    sb_append_str(&sb, part2);

    bool ok = true;
    ok &= (sb.array_info.count == strlen("Hello, World!"));
    ok &= (memcmp(sb.items, "Hello, World!", sb.array_info.count) == 0);
    ok &= (sb.items[sb.array_info.count] == '\0');

    if (ok) TEST_OK("sb_append_str (sized string)");
    else    TEST_FAIL("sb_append_str (sized string)");

    a->free(ctx, sb.items, sb.array_info.capacity * sizeof(char));
}

static void test_sb_append_sb(void) {
    const allocator_t *a = &global_std_allocator;
    void *ctx = NULL;

    string_builder_t src = sb_from_cstr("src", a, ctx);
    string_builder_t dst = sb_with_capacity(1, a, ctx);   /* tiny start → forces growth */

    sb_append_sb(&dst, &src);

    bool ok = true;
    ok &= (dst.array_info.count == src.array_info.count);
    ok &= (memcmp(dst.items, "src", dst.array_info.count) == 0);
    ok &= (dst.items[dst.array_info.count] == '\0');

    if (ok) TEST_OK("sb_append_sb");
    else    TEST_FAIL("sb_append_sb");

    a->free(ctx, src.items, src.array_info.capacity * sizeof(char));
    a->free(ctx, dst.items, dst.array_info.capacity * sizeof(char));
}

static void test_string_split_by_char(void) {
    /* Use the global standard allocator for simplicity */
    const allocator_t *a   = &global_std_allocator;
    void       *ctx  = NULL;

    /* Input string: "a,b,,c" – note the empty token between the two commas */
    const char *src_cstr = "a,b,,c";
    string_t src = string_from_cstr(src_cstr);

    /* Split on ',' */
    string_array_t parts = string_split_by_char(&src, ',', a, ctx);
    if (parts.array_info.count == 0) {
        TEST_FAIL("string_split returned empty array");
        return;
    }

    /* Expected tokens */
    const char *expected[] = { "a", "b", "", "c" };
    const size_t expected_cnt = sizeof(expected) / sizeof(expected[0]);

    bool ok = true;

    /* 1. Correct number of tokens */
    if (parts.array_info.count != expected_cnt) {
        ok = false;
        TEST_FAIL("string_split produced wrong token count: ");
        printf(" - Expected: %ld, Actual: %ld\n", expected_cnt, parts.array_info.count);
    }

    /* 2. Each token matches the expected string */
    for (size_t i = 0; ok && i < expected_cnt; ++i) {
        string_t tok = parts.items[i];
        if (tok.count != strlen(expected[i]) ||
            memcmp(tok.chars, expected[i], tok.count) != 0) {
            ok = false;
            TEST_FAIL("");
            printf("[ FAIL] token %zu mismatch: expected \"%s\", got \"", i, expected[i]);
            fwrite(tok.chars, 1, tok.count, stdout);
            printf("\"\n");
        }
    }

    if (ok) TEST_OK("string_split correctly splits string with empty token");

    a->free(ctx, parts.items, parts.array_info.capacity * sizeof(string_t));
}

static void test_sb_build(void) {
    string_builder_t sb = sb_with_capacity(0,
                                           &global_std_allocator,   /* allocator */
                                           NULL);                  /* alloc_ctx */

    sb_append_cstr(&sb, "test string");   /* sb.count becomes 11, NUL‑terminated */

    string_t view = sb_build(&sb);

    bool ok = true;
    ok &= (view.chars == sb.items);                                 /* same buffer */
    ok &= (view.count == sb.array_info.count);                                 /* same length */
    ok &= (memcmp(view.chars, "test string", view.count) == 0);      /* content */

    if (ok) TEST_OK("sb_build returns correct string view");
    else    TEST_FAIL("sb_build returned incorrect view");

    sb.array_info.allocator->free(sb.array_info.alloc_ctx,
                        sb.items,
                        sb.array_info.capacity * sizeof(char));
}

static void test_string_parse_u64_safe() {
    error_t  err = {0};
    string_t str = string_from_cstr("123456789");

    int64_t num = string_parse_i64_safe(&str, &err);

    bool ok = !err.is_error;

    ok &= (num == 123456789);

    if (ok) TEST_OK("string_parse_i64_safe parses correctly");
    else    TEST_FAIL("string_parse_i64_safe did not work");
}

static void test_string_parse_u64_safe_overflow() {
    error_t  err = {0};
    string_t str = string_from_cstr("123456789012345678912346579");

    string_parse_i64_safe(&str, &err);

    bool ok = err.is_error;

    if (ok) TEST_OK("string_parse_i64_safe rejects numbers that would overflow");
    else    TEST_FAIL("string_parse_i64_safe did not work correctly");
}

static void test_string_parse_u64_unsafe() {
    error_t  err = {0};
    string_t str = string_from_cstr("123456789");

    int64_t num = string_parse_i64_unsafe(&str, &err);

    bool ok = !err.is_error;

    ok &= (num == 123456789);

    if (ok) TEST_OK("string_parse_i64_unsafe parses correctly");
    else    TEST_FAIL("string_parse_i64_unsafe did not work");
}

static void test_sb_reversion() {

    const allocator_t *a = &arena_allocator;
    arena_context_t ctx = {.inner_alloc = &global_std_allocator, .inner_ctx = NULL};

    string_builder_t base = sb_from_cstr("123456789", a, &ctx);
    string_builder_t rev = sb_from_cstr("987654321", a, &ctx);
    da_reverse(base.items, &base.array_info);

    bool ok = da_equals(base.items, &base.array_info, rev.items, &rev.array_info);

    if (ok) TEST_OK("string buffer can be reversed");
    else    TEST_FAIL("string buffer reversion did not work");

    arena_free_all(&ctx);
}


/*------------------------------------------------------------------*
 *  Main – run all tests and report summary
 *------------------------------------------------------------------*/
int main(void) {
    test_sb_from_cstr();
    test_sb_with_capacity();
    test_sb_append_char();
    test_sb_append_cstr();
    test_sb_append_str_sized();
    test_sb_append_sb();
    test_string_split_by_char();
    test_sb_build();
    test_string_parse_u64_safe();
    test_string_parse_u64_safe_overflow();
    test_string_parse_u64_unsafe();
    test_sb_reversion();

    printf("--- Summary: String utilities ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return (tests_failed == 0) ? 0 : 1;
}

