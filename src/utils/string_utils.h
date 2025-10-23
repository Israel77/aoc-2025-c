#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "da.h"
#include "allocator.h"
#include "error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    array_info_t array_info;
    char *items;
} string_builder_t;

// A string is an immutable character sequence. Don't assume null termination for all cases
// The string don't own the memory of *chars, its only a view into this buffer.
typedef struct {
    const char *chars;
    size_t count;
} string_t;

// Dynamic Array structure for sized strings
typedef struct {
    array_info_t array_info;
    string_t *items;
} string_array_t;


// Creates a new string builder from a C string
string_builder_t sb_from_cstr(const char *cstr, const allocator_t *allocator, void *alloc_ctx);
// Creates a new string builder with a given capacity
string_builder_t sb_with_capacity(const size_t capacity, const allocator_t *allocator, void *alloc_ctx);
// Creates a new string builder from a file
string_builder_t sb_read_file(FILE *file, const allocator_t *allocator, void *alloc_ctx);

// Append a character
void sb_append_char(string_builder_t *sb, const char ch);
// Append a null terminated string
void *sb_append_cstr(string_builder_t *sb, const char *cstr);
// Append a sized string
void sb_append_str(string_builder_t *sb, const string_t str);
// Append the contents of another string builder
void sb_append_sb(string_builder_t *sb, const string_builder_t *other);
// Build the string(view) from a string builder
string_t sb_build(string_builder_t *sb);

// Joins the array of strings to a string builder, separated by a delimiter (optional)
string_builder_t string_array_join_by_char(string_array_t array, char delimiter, const allocator_t *allocator, void *alloc_ctx);

// Builds a string directly from a cstr (chars will point to the same memory address as the cstr).
string_t string_from_cstr(const char *cstr);
// Compares two strings, character by character
bool string_equals(const string_t *str, const string_t *other);

// Split a string by a given delimiter
string_array_t string_split_by_char(const string_t *str, const char delimiter, const allocator_t *allocator, void *alloc_ctx);
string_array_t string_split_by_str(const string_t *str, const string_t *delimiter, const allocator_t *allocator, void *alloc_ctx);

string_t string_trim_left(const string_t *str);
string_t string_trim_right(const string_t *str);
string_t string_trim(const string_t *str);

void string_print(const string_t *str);
void string_println(const string_t *str);
void string_sprint(char *buffer, const string_t *str);
void string_sprintln(char *buffer, const string_t *str);

//////////////////////////////////////////////////////////
// Parse strings to integer types (only supports decimals). Safe versions do bound checks, while unsafe versions parse optmistically

// Parse string into a signed 64-bit integer. Returns an error if the string is not a valid number.
int64_t string_parse_i64_safe(const string_t *str, error_t *err);
// Parse string into a signed 64-bit integer. Never returns an error, the parameter is just for compatibility with safe versions.
int64_t string_parse_i64_unsafe(const string_t *str, error_t *err);

// Parse string into an unsigned 64-bit integer. Returns an error if the string is not a valid number.
uint64_t string_parse_u64_safe(const string_t *str, error_t *err);
// Parse string into an unsigned 64-bit integer. Never returns an error, the parameter is just for compatibility with safe versions.
uint64_t string_parse_u64_unsafe(const string_t *str, error_t *err);


#ifdef STRING_UTILS_IMPL

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

string_builder_t sb_from_cstr(const char *cstr, const allocator_t *allocator, void *alloc_ctx) {
    

    size_t count = strlen(cstr);
    string_builder_t sb = {
        .array_info = {
            .item_size = sizeof (char),
            .count = count,
            .capacity = count + 1,
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };


    sb.items = (char *) allocator->alloc(alloc_ctx, sb.array_info.capacity * sizeof(char));

    memcpy(sb.items, cstr, count);
    sb.items[count] = '\0';

    return sb;
}
string_builder_t sb_with_capacity(const size_t capacity, const allocator_t *allocator, void *alloc_ctx) {
    
    string_builder_t sb = {
        .array_info = {
            .item_size = sizeof (char),
            .count = 0,
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };

    sb.items = da_reserve(sb.items, &sb.array_info, capacity);

    if (sb.items) {
        sb.items[0] = '\0';
    }

    return sb;
}


void sb_append_char(string_builder_t *sb, const char ch) {
    sb->items = da_reserve(sb->items, &sb->array_info, sb->array_info.count + 1);
    sb->items[sb->array_info.count++] = ch;
    sb->items[sb->array_info.count] = '\0'; // Ensure null termination
}

void *sb_append_cstr(string_builder_t *sb, const char *cstr) {
    size_t length = strlen(cstr);

    sb->items = da_reserve(sb->items, &sb->array_info, sb->array_info.count + length);

    memcpy(sb->items + sb->array_info.count, cstr, length);
    sb->array_info.count += length;
    sb->items[sb->array_info.count] = '\0'; // Ensure null termination
    return sb->items;
}

void sb_append_str(string_builder_t *sb, const string_t str) {
    sb->items = da_reserve(sb->items, &sb->array_info, sb->array_info.count + str.count);
    memcpy(sb->items + sb->array_info.count, str.chars, str.count);
    sb->array_info.count += str.count;
    sb->items[sb->array_info.count] = '\0'; // Ensure null termination
}

void sb_append_sb(string_builder_t *sb, const string_builder_t *other) {
    sb->items = da_reserve(sb->items, &sb->array_info, sb->array_info.count + other->array_info.count + 1);
    memcpy(&sb->items[sb->array_info.count], other->items, other->array_info.count);
    sb->array_info.count += other->array_info.count;
    sb->items[sb->array_info.count] = '\0'; // Ensure null termination
}

string_t sb_build(string_builder_t *sb) {
    string_t result = {
        .chars = sb->items,
        .count = sb->array_info.count
    };

    return result;

}

string_t string_from_cstr(const char *cstr) {

    const char *ch = cstr;

    while (*ch) {
        ++ch;
    }

    const string_t result = {
        .chars = cstr,
        .count = ch - cstr
    };

    return result;
}

// O(n) amortized
bool string_equals(const string_t *str, const string_t *other) {

    if (str->count != other->count) {
        return false;
    }

    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] != other->chars[i]) {
            return false;
        }
    }

    return true;
}

string_array_t string_split_by_char(const string_t *str, const char delimiter, const allocator_t *allocator, void *alloc_ctx) {
    string_array_t result = {
        .array_info = {
            .item_size = sizeof (string_t),
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };

    const char *segment_start = &str->chars[0];
    size_t current_count = 0;
    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] == delimiter) {
            string_t current_segment = {
                .chars = segment_start,
                .count = current_count
            };
            result.items = da_append(result.items, &result.array_info, &current_segment);
            // Reset the variable for the next segment
            segment_start = &str->chars[i+1];
            current_count = 0;
        } else {
            ++current_count;
        }
    }
    // Append the last string segment
    string_t last_segment = {
        .chars = segment_start,
        .count = current_count
    };

    result.items = da_append(result.items, &result.array_info, &last_segment);

    return result;
}

string_array_t string_split_by_str(const string_t *str, const string_t *delimiter, const allocator_t *allocator, void *alloc_ctx) {
    string_array_t result = {
        .array_info = {
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };

    const char *segment_start = &str->chars[0];
    size_t current_count = 0;

    // Worst case: O(str->count * delimiter->count)
    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] == delimiter->chars[i]) {
            string_t test_str = {
                .chars = &str->chars[i],
                .count = delimiter->count
            };

            // Worst case: O(delimiter->count)
            if (string_equals(&test_str, delimiter)) {
                string_t current_segment = {
                    .chars = segment_start,
                    .count = current_count
                };
                result.items = da_append(result.items, &result.array_info, &current_segment);
                // Reset the variable for the next segment
                segment_start = &str->chars[i+1];
                current_count = 0;
            } else {
                ++current_count;
            }
        } else {
            ++current_count;
        }
    }
    // Append the last string segment
    string_t last_segment = {
        .chars = segment_start,
        .count = current_count
    };

    result.items = da_append(result.items, &result.array_info, &last_segment);

    return result;
}

int64_t string_parse_i64_safe(const string_t *str, error_t *err) {

    err->is_error = false;

    int64_t result = 0;
    // 0 is +, 1 is -
    const char PLUS = 0;
    const char MINUS = 1;
    char sign;

    if (str->chars[0] != '+' && str->chars[0] != '-' && (str->chars[0] < '0' || str->chars[0] > '9')) {
        sprintf(err->error_msg, "Invalid initial character. Expected digit, found: %c", str->chars[0]);
        err->is_error = true;
        return result;
    }

    size_t i = 0;
    if (str->chars[0] == '+') {
        sign = PLUS;
        ++i;
    } else if (str->chars[0] == '-'){
        sign = MINUS;
        ++i;
    } else {
        sign = PLUS;
    }

    if (i == str->count) {
        sprintf(err->error_msg, "No digits were found in the string");
        err->is_error = true;
        return result;
    }

    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] < '0' || str->chars[i] > '9') {
            sprintf(err->error_msg, "Error while parsing integer. Invalid digit at position %ld", i);
            err->is_error = true;
            return result;
        }
        char digit = str->chars[i] - '0';

        // Positive conversion
        if (sign == PLUS) {
            if (result > (INT64_MAX - digit) / 10) {
                sprintf(err->error_msg, "Integer overflow");
                err->is_error = true;
                return result;
            } else {
                result = result * 10 + digit;
            }
        }
        // Negative conversion
        else {
            if (result > (INT64_MAX + digit) / 10) {
                sprintf(err->error_msg, "Integer overflow");
                err->is_error = true;
                return result;
            } else {
                result = result * 10 - digit;
            }
        }
    }

    return result;
}

int64_t string_parse_i64_unsafe(const string_t *str, error_t *err) {

    // This function never actually errors but the parameters is maintained
    // for ease of refactoring after using the safe version
    err->is_error = false;

    int64_t result = 0;
    // 0 is +, 1 is -
    const char PLUS = 0;
    const char MINUS = 1;
    char sign;

    size_t i = 0;
    while (str->chars[i] != '+' && str->chars[i] != '-' && (str->chars[i] < '0' || str->chars[i] > '9')) {
        ++i;
    }

    if (str->chars[i] == '+') {
        sign = PLUS;
        ++i;
    } else if (str->chars[i] == '-'){
        sign = MINUS;
        ++i;
    } else {
        sign = PLUS;
    }

    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] < '0' || str->chars[i] > '9') {
            continue;
        }

        char digit = str->chars[i] - '0';

        // Positive conversion
        if (sign == PLUS) {
            result = result * 10 + digit;
        }
        // Negative conversion
        else {
            result = result * 10 - digit;
        }
    }

    return result;
}
uint64_t string_parse_u64_safe(const string_t *str, error_t *err) {

    err->is_error = false;

    uint64_t result = 0;

    if (str->chars[0] != '+' && str->chars[0] != '-' && (str->chars[0] < '0' || str->chars[0] > '9')) {
        sprintf(err->error_msg, "Invalid initial character. Expected digit, found: %c", str->chars[0]);
        err->is_error = true;
        return result;
    }

    size_t i = 0;
    if (str->chars[0] == '+') {
        ++i;
    }

    if (i == str->count) {
        sprintf(err->error_msg, "No digits were found in the string");
        err->is_error = true;
        return result;
    }

    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] < '0' || str->chars[i] > '9') {
            sprintf(err->error_msg, "Error while parsing integer. Invalid digit at position %ld", i);
            err->is_error = true;
            return result;
        }
        char digit = str->chars[i] - '0';

        if (result > (UINT64_MAX - digit) / 10) {
            sprintf(err->error_msg, "Integer overflow");
            err->is_error = true;
            return result;
        } else {
            result = result * 10 + digit;
        }
    }

    return result;
}

uint64_t string_parse_u64_unsafe(const string_t *str, error_t *err) {

    // This function never actually errors but the parameters is maintained
    // for ease of refactoring after using the safe version
    err->is_error = false;

    uint64_t result = 0;

    size_t i = 0;
    while (str->chars[i] != '+' && (str->chars[i] < '0' || str->chars[i] > '9')) {
        ++i;
    }

    for (size_t i = 0; i < str->count; ++i) {
        if (str->chars[i] < '0' || str->chars[i] > '9') {
            continue;
        }

        char digit = str->chars[i] - '0';

        result = result * 10 + digit;
    }

    return result;
}

// Creates a new string builder from a file
string_builder_t sb_read_file(FILE *file, const allocator_t *allocator, void *alloc_ctx) {
    string_builder_t sb = {
        .array_info = {
            .item_size = sizeof (char),
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };

    string_builder_t temp_sb = {
        .array_info = {
            .item_size = sizeof (char),
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        }
    };
    temp_sb.items = da_reserve(temp_sb.items, &temp_sb.array_info, 1024);

    while (fgets(temp_sb.items, temp_sb.array_info.capacity, file)) {
        temp_sb.array_info.count = strlen(temp_sb.items);
        sb_append_sb(&sb, &temp_sb);
    }
    allocator->free(temp_sb.items, alloc_ctx, temp_sb.array_info.capacity * sizeof (*sb.items));

    return sb;
}

string_t string_trim_left(const string_t *str) {

    string_t result = {
        .chars = str->chars,
        .count = str->count
    };

    // Trim left
    char first_char = result.chars[0];
    while (result.count > 0 && 
            (first_char == ' ' || first_char == '\n' || first_char == '\t')) {
        result.chars = &result.chars[1];
        result.count--;

        first_char = result.chars[0];
    }

    return result;
}

string_t string_trim_right(const string_t *str) {

    string_t result = {
        .chars = str->chars,
        .count = str->count
    };

    // Trim right
    char last_char;
    if (result.count > 0) {
        last_char = result.chars[result.count - 1];
    }
    while (result.count > 0 && 
            (last_char == ' ' || last_char == '\n' || last_char == '\t')) {
        last_char = result.chars[result.count - 1];
        result.count--;

        result.chars = &result.chars[result.count - 1];
    }

    return result;
}

string_t string_trim(const string_t *str) {

    string_t result = string_trim_left(str);
    result = string_trim_right(&result);

    return result;
}

void string_print(const string_t *str) {
    printf("%.*s", (int)str->count, str->chars);
}
void string_println(const string_t *str) {
    printf("%.*s\n", (int)str->count, str->chars);
}

void string_sprint(char *buffer, const string_t *str) {
    sprintf(buffer, "%.*s", (int)str->count, str->chars);
}
void string_sprintln(char *buffer, const string_t *str) {
    sprintf(buffer, "%.*s\n", (int)str->count, str->chars);
}


#endif /*#ifdef STRING_UTILS_IMPL */

#endif /* #ifndef STRING_UTILS_H */
