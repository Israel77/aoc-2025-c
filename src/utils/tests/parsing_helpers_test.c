#include "../macros.h"
#include <stddef.h>

#define STRING_UTILS_IMPL
#include "../string_utils.h"

#include "../parsing_helpers.h"

static int tests_passed = 0;
static int tests_failed = 0;

/* -*- String predicates -*- */
static bool starts_with_digit(const string_t *string) {
    return (string->count > 0) && ('0' <= string->chars[0] && string->chars[0] <= '9');
}

static bool starts_with_letter(const string_t *string) {
    return (string->count > 0)
         && (('a' <= string->chars[0] && string->chars[0] <= 'z')
         || ('A'<= string->chars[0] && string->chars[0] <= 'Z'));
}

static bool always_true(const string_t *string) {
    UNUSED(string);
    return true;
}

static bool always_false(const string_t *string) {
    UNUSED(string);
    return false;
}

/* -*- Tests -*- */

static void test_parse_u64(void) {

    char      *inputs[] = { "0", "123456", "+42", "abc", "-5", "18446744073709551615",     "18446744073709551616" };
    uint64_t  parsed[] = {  0 ,  123456 ,   42,      0,    0,  18446744073709551615ull,     1844674407370955161ull};
    char      *rests[]  = {  "",       "",   "",  "abc", "-5",                     "",                        "6" };

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        uint64_t actual_parsed = parse_u64(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "u64 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "u64 string remainder generated successfully");
    }
}


static void test_parse_u32(void) {

    char      *inputs[] = { "0", "123456", "+42", "xyz", "-5", "4294967295", "4294967296"};
    uint32_t  parsed[] = {  0 ,  123456 ,   42,      0,    0,   4294967295u,  429496729u };
    char      *rests[]  = {  "",       "",   "",  "xyz", "-5",           "",          "6"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        uint32_t actual_parsed = parse_u32(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "u32 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "u32 string remainder generated successfully");
    }
}

static void test_parse_u16(void) {

    char      *inputs[] = { "0", "12345", "+42", "+", "-5", "65535",  "65536"};
    uint16_t  parsed[] = {  0 ,   12345 ,   42,    0,    0,   65535 ,   6553  };
    char      *rests[]  = {  "",      "",   "",  "+", "-5",     "",       "6"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        uint16_t actual_parsed = parse_u16(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "u16 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "u16 string remainder generated successfully");
    }
}

static void test_parse_u8(void) {

    char      *inputs[] = { "0", "12", "+42", " 123", "-5", "255",  "256"};
    uint8_t   parsed[]  = {  0 ,  12 ,   42,      0,    0,   255 ,   25  };
    char      *rests[]  = {  "",   "",   "",  " 123", "-5",    "",    "6"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        uint8_t actual_parsed = parse_u8(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "u8 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "u8 string remainder generated successfully");
    }
}

static void test_parse_i64(void) {

    char      *inputs[] = { "0", "123456", "+42", "abc", "-", "-9223372036854775808"   ,    "9223372036854775807" , "9223372036854775808"};
    int64_t   parsed[]  = {  0 ,  123456 ,   42,      0,  0 ,  -9223372036854775807ll-1,     9223372036854775807ll,  922337203685477580ll};
    char      *rests[]  = {  "",       "",   "",  "abc", "-",                      ""  ,                        "",                   "8"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        int64_t actual_parsed = parse_i64(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "i64 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "i64 string remainder generated successfully");
    }
}


static void test_parse_i32(void) {

    char      *inputs[] = { "0", "-123456", "+42", "", "-2147483648", "2147483647", "-2147483649"};
    int32_t   parsed[]  = {  0 ,  -123456 ,   42 ,  0,  -2147483648 ,  2147483647 ,  -214748364  };
    char      *rests[]  = {  "",       "" ,    "", "",           "" ,           "",           "9"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        int32_t actual_parsed = parse_i32(&input, &actual_rest);


        TEST_ASSERT(actual_parsed == parsed[i], "i32 parsed from string successfully");

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "i32 string remainder generated successfully");
    }
}

static void test_skip_whitespace(void) {

    char      *inputs[] = {"    abc", " x yz", "\tfoo   ", "\r\nbar"};
    char      *rests[]  = {    "abc",  "x yz",   "foo   ",     "bar"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        skip_whitespace(&input, &actual_rest);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "whitespace skipped");
    }
}

static void test_skip_n_chars(void) {

    char      *inputs[] = {"Hello, world!", "!abc","wrong_test", "foo##bar"};
    size_t    skips[]   = { 0             ,   1   ,       6    ,       5   };
    char      *rests[]  = {"Hello, world!",  "abc",      "test",      "bar"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);
        size_t count = skips[i];

        string_t actual_rest;

        skip_n_chars(&input, &actual_rest, count);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "skip specified amount of chars");
    }
}

static void test_skip_while_1(void) {

    char      *inputs[] = {"abc123", "foo_bar", "XyZ", "A1B2C3"};
    char      *rests[]  = {   "123",    "_bar",    "",  "1B2C3"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        skip_while(&input, &actual_rest, starts_with_letter);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "skip while predicate is true");
    }

}

static void test_skip_while_2(void) {

    char      *inputs[] = {"123abc", "foo_bar", "123456789012345678901234567890", "   12345"};
    char      *rests[]  = {   "abc", "foo_bar",                               "", "   12345"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr(rests[i]);

        string_t actual_rest;

        skip_while(&input, &actual_rest, starts_with_digit);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "skip while predicate is true");
    }
}

static void test_skip_while_3(void) {

    char      *inputs[] = {"123abc", "foo_bar", "123456789012345678901234567890", "   12345"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = string_from_cstr("");

        string_t actual_rest;

        /* Skip everything */
        skip_while(&input, &actual_rest, always_true);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "skip while predicate is true");
    }
}

static void test_skip_while_4(void) {

    char      *inputs[] = {"123abc", "foo_bar", "123456789012345678901234567890", "   12345"};

    size_t len = sizeof (inputs) / sizeof (inputs[0]);

    for (size_t i = 0; i < len; ++i) {
    
        string_t input = string_from_cstr(inputs[i]);
        string_t expected_rest = input;

        string_t actual_rest;

        /* Skip nothing */
        skip_while(&input, &actual_rest, always_false);

        TEST_ASSERT(string_equals(&actual_rest, &expected_rest), "skip while predicate is true");
    }
}


int main(void) {

    printf("\n--- Start tests: Parsing helpers ---\n");
    test_parse_u64();
    test_parse_u32();
    test_parse_u16();
    test_parse_u8();

    test_parse_i64();
    test_parse_i32();

    test_skip_whitespace();
    test_skip_n_chars();

    test_skip_while_1();
    test_skip_while_2();
    test_skip_while_3();
    test_skip_while_4();

    printf("--- Summary: Parsing helpers ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n");

    return 0;
}
