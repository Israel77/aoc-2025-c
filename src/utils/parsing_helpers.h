#ifndef PARSING_HELPERS_H
#define PARSING_HELPERS_H

#include "macros.h"
#include "string_utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*string_predicate)(const string_t *);

/* 
 * Parses the src string into an uint64_t integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 * Integers starting with a plus sign (e.g "+12345") are also valid.
 *
 * Notice that for numbers that are too large to fit into a 64-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * For example, in the string "123456789012345678901", the result will be
 * 12345678901234567890 and rest will become "1".
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 64-bit unsigned integer based on the start of the string
 */
internal uint64_t parse_u64(const string_t src, string_t *rest);

/* 
 * Parses the src string into an uint32_t integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 * Integers starting with a plus sign (e.g "+12345") are also valid.
 *
 * Notice that for numbers that are too large to fit into a 32-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 32-bit unsigned integer based on the start of the string
 */
internal uint32_t parse_u32(const string_t src, string_t *rest);

/* 
 * Parses the src string into an uint16_t integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 * Integers starting with a plus sign (e.g "+12345") are also valid.
 *
 * Notice that for numbers that are too large to fit into a 16-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 16-bit unsigned integer based on the start of the string
 */
internal uint16_t parse_u16(const string_t src, string_t *rest);

/* 
 * Parses the src string into an uint8_t integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 * Integers starting with a plus sign (e.g "+12345") are also valid.
 *
 * Notice that for numbers that are too large to fit into a 8-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 8-bit unsigned integer based on the start of the string
 */
internal uint8_t parse_u8(const string_t src, string_t *rest);

/* 
 * Parses the src string into an 64-bit signed integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 *
 * Notice that for numbers that are too large to fit into a 64-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * For example, in the string "-12345678901", the result will be
 * -1234567890 and rest will become "1".
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 64-bit integer based on the start of the string
 */
internal int64_t parse_i64(const string_t src, string_t *rest);

/* 
 * Parses the src string into an 32-bit signed integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 *
 * Notice that for numbers that are too large to fit into a 32-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * For example, in the string "-12345678901", the result will be
 * -1234567890 and rest will become "1".
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 32-bit integer based on the start of the string
 */
internal int32_t parse_i32(const string_t src, string_t *rest);

/* 
 * Parses the src string into an 16-bit signed integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 *
 * Notice that for numbers that are too large to fit into a 16-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * For example, in the string "-12345678901", the result will be
 * -1234567890 and rest will become "1".
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 16-bit integer based on the start of the string
 */
internal int16_t parse_i16(const string_t src, string_t *rest);

/* 
 * Parses the src string into an 8-bit signed integer and puts the non-parsed part in rest.
 * If the beginning of the string is not a valid integer it will return zero and rest will
 * be equal to src.
 *
 * Notice that for numbers that are too large to fit into a 8-bit integer, only the
 * first digits will become an integer and the remaining characters will be put in rest.
 *
 * For example, in the string "-12345678901", the result will be
 * -1234567890 and rest will become "1".
 *
 * src  - The string to be parsed.
 * rest - The remaining of the original string after what was parsed.
 * 
 * Returns:
 * 8-bit integer based on the start of the string
 */
internal int8_t parse_i8(const string_t src, string_t *rest);

/* 
 * Parses the src string into a single digit, which is expected to be a character between '0' and '9'.
 * It puts the non-parsed part in rest. 
 * If the first character in the string is not a digit, it will return zero and rest will
 * be equal to src.
 *
 * src  - The string to be parsed.
 * rest - The remaining part of the original string after what was parsed.
 * 
 * Returns:
 * An 8-bit unsigned integer that represents the parsed digit.
 */
internal uint8_t parse_digit(const string_t src, string_t *rest);

/* 
 * Parses the src string into a boolean value (true or false), in a case insensitive way.
 * It puts the non-parsed part in rest. 
 * If the start of the string  is not a valid boolean, it will default to returning false and 
 * rest will be equal to src.
 *
 * src  - The string to be parsed.
 * rest - The remaining part of the original string after what was parsed.
 * 
 * Returns:
 * An 8-bit unsigned integer that represents the parsed digit.
 */
internal bool parse_bool(const string_t src, string_t *rest);

/* 
 * Skips the first character in the src string if it is also present in the skip string.
 * The remainder of the original string after the skipped characters will be placed in rest.
 *
 * src        - The string to be examined.
 * rest       - The remaining part of the original string after characters have been skipped.
 * skip       - Array containing the characters to be skipped.
 * skip_count - The number of characters in the skip array.
 */
internal void skip_any_of(const string_t src, string_t *rest, const char *skip, const size_t skip_count);

/* 
 * Skips all characters in the beginning of src string that are also present in the skip string.
 * The remainder of the original string after the skipped characters will be placed in rest.
 *
 * src        - The string to be examined.
 * rest       - The remaining part of the original string after characters have been skipped.
 * skip       - Array containing the characters to be skipped.
 * skip_count - The number of characters in the skip array.
 */
internal void skip_all_of(const string_t src, string_t *rest, const char *skip, const size_t skip_count);

/* 
 * Skips the first occurrence of a specified character in the src string.
 * If the character is found, rest will point to the string following the skipped character.
 * If the character is not found, rest will be equal to src.
 *
 * src  - The string to be examined.
 * rest - The remaining part of the original string after the character has been skipped.
 * ch   - The character to be skipped.
 */
internal void skip_char(const string_t src, string_t *rest, char ch);
/* 
 * Skips the first n chars in the src string, regardless of what the are.
 * If count is equal to 0, rest will be equal to src.
 * If count is larger than src->count, rest will be empty.
 *
 * src   - The string to be examined.
 * rest  - The remaining part of the original string after the characters have been skipped.
 * count - Number of chars to be skipped.
 */
internal void skip_n_chars(const string_t src, string_t *rest, size_t count);

/* 
 * Skips all whitespace characters in the src string.
 * The remainder of the original string after the skipped whitespace will be placed in rest.
 * If no whitespace is present, rest will be equal to src.
 *
 * src  - The string to be examined.
 * rest - The remaining part of the original string after whitespaces has been skipped.
 */
internal void skip_whitespace(const string_t src, string_t *rest);

/* 
 * Skips the specified string in the src string.
 * If the string is found at the beginning, it will be skipped and the rest will point to the string that follows.
 * If not found, rest will be equal to src.
 *
 * src     - The string to be examined.
 * rest    - The remaining part of the original string after the specified string has been skipped.
 * pattern - The string to be skipped.
 */
internal void skip_string(const string_t src, string_t *rest, const string_t *pattern);

/* 
 * Skips the specified C-style string (null-terminated) in the src string.
 * If the C-string is found at the beginning, it will be skipped and the rest will point to the string that follows.
 * If not found, rest will be equal to src.
 *
 * src     - The string to be examined.
 * rest    - The remaining part of the original string after the null-terminated string has been skipped.
 * pattern - The C-string to be skipped.
 *
 * Time complexity: O(n * T) where
 *     - n is the number of characters in src
 *     - T is the time complexity for running the predicate.
 */
internal void skip_cstr(const string_t src, string_t *rest, const char *pattern);

/* 
 * Skips characters in the src string while the provided predicate function returns true.
 * It will consume substrings matching the given condition, and make rest equal to the first
 * substring that doesn't. For exemple, if the predicate is true for strings starting with either
 * 'A' or 'B', then after applying this function to the string "ABCDA", rest will equal "CDA".
 *
 * src  - The string to be examined.
 * rest - The remaining part of the original string after characters have been skipped.
 * test - A predicate function pointer that defines the skipping condition.
 */
internal void skip_while(const string_t src, string_t *rest, string_predicate test);

/* 
 * Skips characters in the src string until the provided predicate function returns true.
 * It will consume substrings that don't matchthe given condition, and make rest equal to the first
 * substring that does. For exemple, if the predicate is true for strings starting with the letter
 * 'C' , then after applying this function to the string "ABCDC", rest will equal "CDC".
 *
 * src  - The string to be examined.
 * rest - The remaining part of the original string after characters have been skipped.
 * test - A predicate function pointer that defines when to stop skipping.
 */
internal void skip_until(const string_t src, string_t *rest, string_predicate test);

internal inline uint64_t parse_unsigned(const string_t src, string_t *rest, uint64_t max_value) {

    uint64_t result = 0;

    *rest = src;

    if (src.chars[0] != '+' && (src.chars[0] < '0' || src.chars[0] > '9')) {
        return result;
    }

    /* Copy the values to avoid breaking invariants in case src and rest are the same pointer */
    const size_t src_count = src.count;
    const char *src_chars = src.chars;

    size_t i = 0;
    if (src_chars[0] == '+') {
        ++i;
        if(rest->count > 1)rest->count--;
    }

    if (i == src_count) {
        return result;
    }

    while (i < src_count) {

        char digit = src_chars[i] - '0';
        rest->chars = &src_chars[i];

        if ('0' > rest->chars[0] || rest->chars[0] > '9') {
            return result;
        }

        if (unlikely(result > (max_value - digit) / 10)) {
            return result;
        } else {
            result = result * 10 + digit;
        }

        rest->count--;
        i++;
    }

    return result;

}

internal inline int64_t parse_signed(const string_t src, string_t *rest, int64_t min_value, int64_t max_value) {

    int64_t result = 0;
    bool is_negative = false;

    *rest = src;

    if ((src.chars[0] != '+' && src.chars[0] != '-') && (src.chars[0] < '0' || src.chars[0] > '9')) {
        return result;
    }

    size_t i = 0;
    if (src.chars[0] == '+') {

        ++i;
        if(rest->count > 1) rest->count--;

    } else if (src.chars[0] == '-') {

        is_negative = true;
        ++i;
        if(rest->count > 1) rest->count--;
    }

    if (i == src.count) {
        return result;
    }

    while (i < src.count) {

        char digit = src.chars[i] - '0';
        rest->chars = &src.chars[i];

        if ('0' > rest->chars[0] || rest->chars[0] > '9') {
            return result;
        }

        if (is_negative) {

            if (unlikely(result < (min_value + digit) / 10)) {
                return result;
            } else {
                result = result * 10 - digit;
            }

        } else {

            if (unlikely(result > (max_value - digit) / 10)) {
                return result;
            } else {
                result = result * 10 + digit;
            }
        }

        rest->count--;
        ++i;
    }

    return result;

}

internal uint64_t parse_u64(const string_t src, string_t *rest) {

    return parse_unsigned(src, rest, UINT64_MAX);

}

internal uint32_t parse_u32(const string_t src, string_t *rest) {

    return parse_unsigned(src, rest, UINT32_MAX);
}

internal uint16_t parse_u16(const string_t src, string_t *rest) {

    return parse_unsigned(src, rest, UINT16_MAX);
}

internal uint8_t parse_u8(const string_t src, string_t *rest) {
    
    return parse_unsigned(src, rest, UINT8_MAX);
}

internal int64_t parse_i64(const string_t src, string_t *rest) {

    return parse_signed(src, rest, INT64_MIN, INT64_MAX);
}

internal int32_t parse_i32(const string_t src, string_t *rest) {
    
    return parse_signed(src, rest, INT32_MIN, INT32_MAX);
}

internal int16_t parse_i16(const string_t src, string_t *rest) {

    return parse_signed(src, rest, INT16_MIN, INT16_MAX);
}

internal int8_t parse_i8(const string_t src, string_t *rest) {

    return parse_signed(src, rest, INT8_MIN, INT8_MAX);
}

internal uint8_t parse_digit(const string_t src, string_t *rest) {

    int8_t result = 0;

    *rest = src;

    if (rest->count > 0 && ('0' <= src.chars[0] && src.chars[0] <= '9')) {
        result = src.chars[0] - '0';
        rest->chars = &rest->chars[1];
        rest->count--;
    }

    return result;
}

internal bool parse_bool(const string_t src, string_t *rest) {

    bool result = false;

    *rest = src;

    if (src.count >= 4
            && (src.chars[0] == 't' || src.chars[0] == 'T')
            && (src.chars[1] == 'r' || src.chars[1] == 'R')
            && (src.chars[2] == 'u' || src.chars[2] == 'U')
            && (src.chars[3] == 'e' || src.chars[3] == 'E')) {

        result = true;

        rest->chars  = &rest->chars[4];
        rest->count -= 4;
    }

    if (src.count >= 5
            && (src.chars[0] == 'f' || src.chars[0] == 'F')
            && (src.chars[1] == 'a' || src.chars[1] == 'A')
            && (src.chars[2] == 'l' || src.chars[2] == 'L')
            && (src.chars[3] == 's' || src.chars[3] == 'S')
            && (src.chars[4] == 'e' || src.chars[4] == 'E')) {

        result = false;

        rest->chars  = &rest->chars[5];
        rest->count -= 5;
    }

    return result;
}

internal void skip_char(const string_t src, string_t *rest, char ch) {

    *rest = src;

    if (rest->count > 0 && rest->chars[0] == ch) {
        rest->chars = &rest->chars[1];
        rest->count--;
    }
}

internal void skip_n_chars(const string_t src, string_t *rest, size_t count) {

    *rest = src;

    if (count <= src.count) {
        rest->count -= count;
        rest->chars  = &rest->chars[count];
    } else {
        rest->count = 0;
    }

}

internal void skip_any_of(const string_t src, string_t *rest, const char *skip, const size_t skip_count) {

    *rest = src;

    for (size_t i = 0; i < skip_count; ++i) {
        /* Exit when the input matches the char */
        if (rest->chars[0] == skip[i]) {
            skip_char(*rest, rest, skip[i]);
            break;
        }
    }

}

internal void skip_all_of(const string_t src, string_t *rest, const char *skip, const size_t skip_count) {

    bool stop = false;

    *rest = src;

    while (!stop) {

        const char *prev_ptr = rest->chars;

        skip_any_of(*rest, rest, skip, skip_count);

        /* Stop when it is not possible to advance anymore */
        stop = (rest->count == 0) || (rest->chars == prev_ptr);
    }
}

internal void skip_whitespace(const string_t src, string_t *rest) {

    const char whitespace[] = " \f\n\r\t\v";
    /* Subtract one to account for the null character at the end. */
    const size_t whitespace_count = (sizeof (whitespace) / sizeof (char)) - 1;

    skip_all_of(src, rest, whitespace, whitespace_count);
}

internal void skip_string(const string_t src, string_t *rest, const string_t *pattern) {

    *rest = src;

    if (rest->count > 0 && pattern->count > 0 && rest->count >= pattern->count) {

        string_t check = {
            .chars = rest->chars,
            .count = pattern->count
        };

        if (!string_equals(pattern, &check)) {
            rest->chars  = &rest->chars[pattern->count];
            rest->count -= pattern->count;
        }
    }
}
internal void skip_all_strings(const string_t src, string_t *rest, const string_t *pattern) {

    *rest = src;

    bool stop = false;

    while (!stop) {
        const char *prev_ptr = rest->chars;

        skip_string(*rest, rest, pattern);

        stop = (prev_ptr == rest->chars);
    }
}

internal void skip_cstr(const string_t src, string_t *rest, const char *pattern) {

    string_t string = string_from_cstr(pattern);

    skip_string(src, rest, &string);
}

internal void skip_while(const string_t src, string_t *rest, string_predicate test) {
    
    *rest = src; 

    while (rest->count > 0 && test(rest)) {
        rest->chars = &rest->chars[1];
        rest->count--;
    }
}

internal void skip_until(const string_t src, string_t *rest, string_predicate test) {
    
    *rest = src; 

    while (rest->count > 0 && !test(rest)) {
        rest->chars = &rest->chars[1];
        rest->count--;
    }
}

#endif /* #ifndef PARSING_HELPERS_H */
