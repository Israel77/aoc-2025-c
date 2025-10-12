#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define ALLOC_STD_IMPL
#define ALLOC_ARENA_IMPL
#include "../allocator.h"
#include "../test.h"

#define STRING_UTILS_IMPL
#include "../string_utils.h"

#define BIGINT_IMPL
#include "../bigint.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void test_encoding_decoding() {
    error_t err = {0};

    char *values[] = {
        "6154021310635472942320286609226814949822194572662423616250",
        "-1",
        "28491",
        "16",
        "4294967296",
        "123456789123456789123456789",
        "18446744073709551617",
        "120000000000000000000000000",
        "0",
    };

    bool ok = true;
    for (size_t i = 0; i < sizeof(values)/sizeof(char*); ++i) {

        const char *value = values[i];
        string_t sized_value = string_from_cstr(value);

        bigint_t num = bigint_from_cstr(value, &global_std_allocator, NULL, &err);

        if (err.is_error) {
            TEST_FAIL("");
            fprintf(stderr, "Error while parsing%s",err.error_msg);
        }
        
        string_builder_t sb = bigint_to_sb(&num, &global_std_allocator, NULL, &global_std_allocator, NULL);

        string_t str = sb_build(&sb);

        // string_println(&str);

        ok &= string_equals(&str, &sized_value); 

        if (!ok) {
            fprintf(stderr, "Expected: %s, got %*.s\n", value, (int)str.count, str.chars);
        }

        da_free(sb.items, &sb.array_info);
        da_free(num.items, &num.array_info);
    }

    if (ok) {
        TEST_OK("Creating and decoding big integers from strings");
    } else {
        TEST_FAIL("Creating and decoding big integers from strings");
    }
}

static void test_addition() {

    error_t err;
    bigint_t result = bigint_from_cstr("0", &global_std_allocator, NULL, &err);
    bigint_t expected_result;

    bigint_t small_positive = bigint_from_cstr("256", &global_std_allocator, NULL, &err);
    bigint_t small_negative = bigint_from_cstr("-255", &global_std_allocator, NULL, &err);
    bigint_t big_positive = bigint_from_cstr("100000000000", &global_std_allocator, NULL, &err);
    bigint_t big_negative = bigint_from_cstr("-10000000000", &global_std_allocator, NULL, &err);
    

    expected_result = bigint_from_cstr("256", &global_std_allocator, NULL, &err);
    bigint_add_in(&result, &small_positive);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add positive integers");
    } else {
        TEST_FAIL("Add positive integers");
        printf("%s\n", err.error_msg);
    }

    // Reset result for the next test
    da_free(expected_result.items, &expected_result.array_info);

    // Test adding a small negative integer
    expected_result = bigint_from_cstr("1", &global_std_allocator, NULL, &err); // 256 + (-255) = 1
    bigint_add_in(&result, &small_negative);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add positive and negative integers");
    } else {
        TEST_FAIL("Add positive and negative integers");
        printf("%s\n", err.error_msg);
    }
    
    // Reset result for the next test
    result.array_info.count = 0;
    bigint_normalize(&result);
    da_free(expected_result.items, &expected_result.array_info);

    // Test adding a big positive integer
    expected_result = bigint_from_cstr("100000000000", &global_std_allocator, NULL, &err); // 0 + 100000000000 = 100000000000
    bigint_add_in(&result, &big_positive);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add big positive integer");
    } else {
        TEST_FAIL("Add big positive integer");
        printf("%s\n", err.error_msg);
    }
    
    // Reset result for the next test
    result.array_info.count = 0;
    bigint_normalize(&result);
    da_free(expected_result.items, &expected_result.array_info);

    // Test adding a big negative integer
    expected_result = bigint_from_cstr("-10000000000", &global_std_allocator, NULL, &err); // 0 + (-10000000000) = -10000000000
    bigint_add_in(&result, &big_negative);
    if (bigint_equals(&expected_result, &result)) {
        TEST_OK("Add big negative integer");
    } else {
        TEST_FAIL("Add big negative integer");
        printf("%s\n", err.error_msg);
    }

    da_free(big_negative.items, &big_negative.array_info);
    da_free(big_positive.items, &big_positive.array_info);
    da_free(small_negative.items, &small_negative.array_info);
    da_free(small_positive.items, &small_positive.array_info);
    da_free(result.items, &result.array_info);
    da_free(expected_result.items, &expected_result.array_info);
}

static void test_multiplication() {

    error_t err = {0};
    arena_context_t test_ctx = { .inner_alloc = &global_std_allocator };

    bigint_t num1 = bigint_from_cstr("4294967296", &arena_allocator, &test_ctx, &err);
    bigint_t num2 = bigint_from_cstr("2", &arena_allocator, &test_ctx, &err);
    bigint_t expected_result = bigint_from_cstr("8589934592", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive numbers");
    } else {
        TEST_FAIL("Multiply positive numbers");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive numbers with (binary) carry");
    } else {
        TEST_FAIL("Multiply positive numbers with (binary) carry");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("-100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply negative numbers");
    } else {
        TEST_FAIL("Multiply negative numbers");
    }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("100000000000", &arena_allocator, &test_ctx, &err);
    expected_result = bigint_from_cstr("-1000000000000", &arena_allocator, &test_ctx, &err);

    bigint_mul_in(&num1, &num2);

    if (bigint_equals(&expected_result, &num1)) {
        TEST_OK("Multiply positive and negative numbers");
    } else {
        TEST_FAIL("Multiply positive and negative numbers");
    }

    arena_reset(&test_ctx);

    arena_free_all(&test_ctx);
}

static void test_division() {

    error_t err = {0};
    arena_context_t test_ctx = { .inner_alloc = &global_std_allocator };

    bigint_t num1 = bigint_from_cstr("8589934592", &arena_allocator, &test_ctx, &err);
    bigint_t num2 = bigint_from_cstr("2", &arena_allocator, &test_ctx, &err);
    divmod_t expected = {
        .quotient = bigint_from_cstr("4294967296", &arena_allocator, &test_ctx, &err),
        .remainder = bigint_from_cstr("0", &arena_allocator, &test_ctx, &err)
    };

    divmod_t actual = bigint_divmod(&num1, &num2, &arena_allocator, &test_ctx, &err);
    //
    // if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
    //     TEST_OK("Divide positive numbers");
    // } else {
    //     TEST_FAIL("Divide positive numbers");
    // }

    arena_reset(&test_ctx);

    num1 = bigint_from_cstr("10", &arena_allocator, &test_ctx, &err);
    num2 = bigint_from_cstr("3", &arena_allocator, &test_ctx, &err);
    expected.quotient = bigint_from_cstr("3", &arena_allocator, &test_ctx, &err);
    expected.remainder = bigint_from_cstr("1", &arena_allocator, &test_ctx, &err);

    actual = bigint_divmod(&num1, &num2, &arena_allocator, &test_ctx, &err);

    if (bigint_equals(&expected.quotient, &actual.quotient) && bigint_equals(&expected.remainder, &actual.remainder)) {
        TEST_OK("Divide positive numbers with remainder");
    } else {
        TEST_FAIL("Divide positive numbers with remainder");
    }

    arena_reset(&test_ctx);
    //
    // num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    // num2 = bigint_from_cstr("-2", &arena_allocator, &test_ctx, &err);
    // expected_result = bigint_from_cstr("5", &arena_allocator, &test_ctx, &err);
    // remainder = 0;
    //
    // bigint_divmod_in(&num1, &num2, &remainder, &err);
    //
    // if (bigint_equals(&expected_result, &num1) && remainder == 0) {
    //     TEST_OK("Divide negative numbers");
    // } else {
    //     TEST_FAIL("Divide negative numbers");
    // }
    //
    // arena_reset(&test_ctx);
    //
    // num1 = bigint_from_cstr("-10", &arena_allocator, &test_ctx, &err);
    // num2 = bigint_from_cstr("3", &arena_allocator, &test_ctx, &err);
    // expected_result = bigint_from_cstr("-3", &arena_allocator, &test_ctx, &err);
    // remainder = 0;
    //
    // bigint_divmod_in(&num1, &num2, &remainder, &err);
    //
    // if (bigint_equals(&expected_result, &num1) && remainder == 1) {
    //     TEST_OK("Divide negative by positive");
    // } else {
    //     TEST_FAIL("Divide negative by positive");
    // }

    arena_reset(&test_ctx);

    arena_free_all(&test_ctx);
}

int main(void)
{

    printf("--- Start tests: Bigint ---\n");
    test_encoding_decoding();
    test_addition();
    test_multiplication();
    // test_division();

    printf("--- Summary: Bigint ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return EXIT_SUCCESS;
}
