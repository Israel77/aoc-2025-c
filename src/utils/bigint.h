#ifndef BIGINT_H
#define BIGINT_H

#include "da.h"
#include "error.h"
#include "allocator.h"
#include "string_utils.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Base for the bigint representation.*/
/* By using 2^32 as the base, any u32 can be a "digit"*/
/* which simplifies some algorithms. (DO NOT MODIFY)*/
static const uint64_t BASE = (1ULL)<<32;
static const uint8_t LOG_10_BASE = 9;
/*
static const uint64_t BASE = 100;
static const uint8_t LOG_10_BASE = 2; //How many decimal digits
*/

/*
 * Internal representation of an arbitrary precision integer type.
 * Each "digit" is a 32-bit integer, stored in an dynamic array
 * in little-endian form (least significant to most significant).
 * Empty arrays represent 0.
 *
 * The array {a0, a1, a2...} represents a0 + a1 * BASE + a2 * BASE^2 ...
 */

typedef struct {
    array_info_t array_info;
    uint64_t *items;
    bool is_negative;
} bigint_t;

typedef struct {
    bigint_t quotient;
    bigint_t remainder;
} divmod_t;

/* Instantiation and copying */

/* Create a big integer with a given capacity on the underlying array of int64 */
bigint_t bigint_with_capacity(size_t capacity, const allocator_t *allocator, void *alloc_ctx);

/* Create a big integer from a single 64-bit integer */
bigint_t bigint_from_i64(const int64_t value, const allocator_t *allocator, void *alloc_ctx);
/* Create a big integer from a sized string */
bigint_t bigint_from_string(const string_t *value, const allocator_t *allocator, void *alloc_ctx, error_t *err);
/* Create a big integer from a null-terminated string */
bigint_t bigint_from_cstr(const char *value, const allocator_t *allocator, void *alloc_ctx, error_t *err);
/* Copy a big integer into another */
void bigint_copy(bigint_t *dst, const bigint_t *src);

/* Printing */
string_builder_t bigint_to_sb(const bigint_t *num,
        /* Allows the use of a different allocator */
        /* than the one used for the bigint itself */
        const allocator_t *str_allocator, void *str_ctx,
        /* The algorithm needs to allocate some temporary memory as well */
        const allocator_t *temp_allocator, void *temp_ctx
        );

/* Comparison operations */
static inline bool bigint_abs_eq(const bigint_t *num, const bigint_t *other);
static inline bool bigint_abs_lt(const bigint_t *num, const bigint_t *other);
static inline bool bigint_abs_gt(const bigint_t *num, const bigint_t *other);
bool bigint_eq(const bigint_t *num, const bigint_t *other);
bool bigint_lt(const bigint_t *num, const bigint_t *other);
bool bigint_gt(const bigint_t *num, const bigint_t *other);

/* Special functions */

/* Remove leading zeroes and convert -0 to 0 */
static inline void bigint_normalize(bigint_t *num);

/* Operations producing new integers */
bigint_t bigint_add(const bigint_t *num, const bigint_t *other, const allocator_t *allocator, void *alloc_ctx);
bigint_t bigint_sub(const bigint_t *num, const bigint_t *other, const allocator_t *allocator, void *alloc_ctx);
bigint_t bigint_mul(const bigint_t *num, const bigint_t *other, const allocator_t *allocator, void *alloc_ctx);

divmod_t bigint_divmod(const bigint_t *num, const bigint_t *other, const allocator_t *allocator, void *alloc_ctx, error_t *err);

/* In-place operations */
void bigint_increment(bigint_t *num);
void bigint_decrement(bigint_t *num);
void bigint_add_in(bigint_t *num, const bigint_t *other);
void bigint_sub_in(bigint_t *num, const bigint_t *other);
void bigint_mul_in(bigint_t *num, const bigint_t *other);
void bigint_mul_in_u64(bigint_t *num, const uint64_t other);


#ifdef BIGINT_IMPL
bigint_t bigint_with_capacity(size_t capacity, const allocator_t *allocator, void *alloc_ctx) {

    bigint_t result = {
        .array_info = {
            .item_size = sizeof (*result.items),
            .min_capacity = capacity,
            .allocator = allocator,
            .alloc_ctx = alloc_ctx
        },
        .is_negative = false,
    };

    result.items = da_reserve(result.items, &result.array_info, capacity);

    return result;
}

bigint_t bigint_from_i64(const int64_t value, const allocator_t *allocator, void *alloc_ctx) {

    /* Only one digit is needed, but start with capacity 16 to avoid too much overhead */
    /* when doing operations */
    bigint_t result = bigint_with_capacity(16, allocator, alloc_ctx);

    result.is_negative = value < 0;

    uint64_t u = (value < 0 ? -value : value);

    result.items = da_append(result.items, &result.array_info, &u);

    return result;
}

bigint_t bigint_from_string(const string_t *str, const allocator_t *allocator, void *alloc_ctx, error_t *err) {

    err->is_error = false;

    bigint_t result = bigint_with_capacity((str->count / LOG_10_BASE) + 1, allocator, alloc_ctx);

    /* If the string is empty return 0 */
    if (str->count == 0) {
        return result;
    }

    if (str->chars[0] != '+' && str->chars[0] != '-' && (str->chars[0] < '0' || str->chars[0] > '9')) {
        sprintf(err->error_msg, "Invalid initial character. Expected digit, found: %c", str->chars[0]);
        err->is_error = true;
        return result;
    }

    size_t i = 0;
    if (str->chars[i] == '-') {
        result.is_negative = true;
        ++i;
    } else if (str->chars[i] == '+') {
        ++i;
    }

    /* For small enough numbers, just parse as an u32 */
    if (str->count < LOG_10_BASE) {
        /* Advance the string view if necessary */
        const string_t updated_view = {
            .chars = &str->chars[i],
            .count = str->count - i,
        };
        /* This casting is safe because of the above check, as long as BASE <= UINT32_MAX */
        uint64_t value_u64 = (uint64_t)(string_parse_u64_safe(&updated_view, err) % BASE);

        result.items = da_append(result.items, &result.array_info, &value_u64);
        goto defer;
    }

    result.items = da_reserve(result.items, &result.array_info, str->count);

    result.items[0] = 0;
    for (; i < str->count; ++i) {

        if (str->chars[i] < '0' || str->chars[i] > '9') {
            sprintf(err->error_msg, "Error while parsing integer. Invalid digit at position %ld", i);
            err->is_error = true;
            return result;
        }

        char digit = str->chars[i] - '0';

        /* Multiply by 10 and propagate the carry */
        uint64_t carry = digit;
        for (size_t j=0; j < result.array_info.count; ++j) {
            uint64_t prod = ((uint64_t)result.items[j]) * 10 + carry;
            /* The lower bits become the new "digit" (on the internal base) */
            result.items[j] = prod % BASE; 
            carry = prod / BASE;
        }

        if (carry > 0) {
            result.items = da_append(result.items, &result.array_info, &carry);
        }
    }

defer:
    bigint_normalize(&result);

    return result;
}

bigint_t bigint_from_cstr(const char *value, const allocator_t *allocator, void *alloc_ctx, error_t *err) {
    string_t str = string_from_cstr(value);
    return bigint_from_string(&str, allocator, alloc_ctx, err);
}

void bigint_copy(bigint_t *dst, const bigint_t *src) {
    /* @sneaky_allocation */
    dst->items = da_reserve(dst->items, &dst->array_info, src->array_info.capacity);
    /* memcpy(dst->items, src->items, (src->array_info.count) * sizeof (*dst->items)); */
    for (size_t i = 0; i < src->array_info.count; ++i) {
        dst->items[i] = src->items[i];
    }
    dst->array_info.item_size = src->array_info.item_size;
    dst->array_info.count = src->array_info.count;
    dst->is_negative = src->is_negative;
}

string_builder_t bigint_to_sb(const bigint_t *num, 
        /* Allocator for the memory of the returned string */
        const allocator_t *str_alloc, void *str_ctx,
        /* Allocator for temporary memory used in the subroutine */
        const allocator_t *temp_alloc, void *temp_ctx
        ) {
    string_builder_t sb = {
        .array_info = {
            .item_size = sizeof (*sb.items),
            .allocator = str_alloc,
            .alloc_ctx = str_ctx
        }
    };

    if (num->array_info.count == 0) {
        sb_append_char(&sb, '0');
        return sb;
    }

    /* Upper bound: 9 digits per segment on the internal array */
    sb.items = da_reserve( sb.items, &sb.array_info, num->array_info.count * 9);

    bigint_t working_copy = bigint_with_capacity(num->array_info.count, temp_alloc, temp_ctx);
    bigint_copy(&working_copy, num);

    /* Revert the working_copy, as it is easier to make the conversion if it is in big endian */
    da_reverse(working_copy.items, &working_copy.array_info);

    bigint_t quotient = bigint_with_capacity(working_copy.array_info.capacity, working_copy.array_info.allocator, working_copy.array_info.alloc_ctx);
    while (working_copy.array_info.count > 0) {
        quotient.array_info.count = 0;
        uint64_t carry = 0;

        for (size_t i = 0; i < working_copy.array_info.count; ++i) {
            /* a0 * b^n + a1 * b^(n-1) = (a0 * b + a1) * b^(n-1) */
            /* uint64_t temp = (carry << 32) | working_copy.items[i]; */
            uint64_t temp = carry * BASE + working_copy.items[i];
            uint64_t q = temp / 10;
            carry = temp % 10;
            if (quotient.array_info.count != 0 || q != 0) {
                quotient.items = da_append(quotient.items, &quotient.array_info, &q);
            }
        }

        sb_append_char(&sb, (char)carry + '0');
        /* @TODO: Simplify the copy logic (maybe get rid of quotient) */
        if (working_copy.items != quotient.items) {
            da_free(working_copy.items, &working_copy.array_info);
        }
        working_copy.items = quotient.items;
        working_copy.array_info.count = quotient.array_info.count;
    }


    if (num->is_negative) {
        sb_append_char(&sb, '-');
    }

    /* The decimal number representation is inverted */
    da_reverse(sb.items, &sb.array_info);

    da_free(quotient.items, &quotient.array_info);

    return sb;
}

static inline bool bigint_abs_eq(const bigint_t *num, const bigint_t *other) {

    if (num->array_info.count != other->array_info.count) {
        return false;
    }

    for (size_t i = 0; i < num->array_info.count; ++i) {
        if (num->items[i] != other->items[i]) {
            return false;
        }
    }

    return true;
}

bool bigint_eq(const bigint_t *num, const bigint_t *other) {

    if (num->is_negative != other->is_negative) {
        return false;
    }

    return bigint_abs_eq(num, other);
}

/* Compares the absolute value of two numbers */
static inline bool bigint_abs_lt(const bigint_t *num, const bigint_t *other) {

    /* If the magnitudes are different there is no need to compare the digits */
    if (num->array_info.count < other->array_info.count) {
        return true;
    }

    if (num->array_info.count > other->array_info.count) {
        return false;
    }

    /* Iterate from most significant to least significant */
    for (size_t i = num->array_info.count; i > 0; --i) {
        /* As soon as the digits are different, return the comparison of digits */
        if (num->items[i-1] != other->items[i-1]) {
            return (num->items[i-1] < other->items[i-1]);
        }
    }

    /* This would mean all digits are equal */
    return false;
}

static inline bool bigint_abs_gt(const bigint_t *num, const bigint_t *other) {
    /* If the magnitudes are different there is no need to compare the digits */
    if (num->array_info.count > other->array_info.count) {
        return true;
    }

    if (num->array_info.count < other->array_info.count) {
        return false;
    }

    /* Iterate from most significant to least significant */
    for (size_t i = num->array_info.count; i > 0; --i) {
        /* As soon as the digits are different, return the comparison of digits */
        if (num->items[i-1] != other->items[i-1]) {
            return (num->items[i-1] > other->items[i-1]);
        }
    }

    /* This would mean all digits are equal */
    return false;
}

bool bigint_lt(const bigint_t *num, const bigint_t *other) {

    /* If the signs are different, the positive one is bigger */
    if (num->is_negative && !other->is_negative) {
        return true;
    } else if (!num->is_negative && other->is_negative) {
        return false;
    }

    /* If they're positive, check which one has the lesser absolute value */
    if (!num->is_negative) {
        return bigint_abs_lt(num, other);
    } else {
        return bigint_abs_gt(num, other);
    }
}

bool bigint_gt(const bigint_t *num, const bigint_t *other) {

    /* If the signs are different, the positive one is bigger */
    if (num->is_negative && !other->is_negative) {
        return false;
    } else if (!num->is_negative && other->is_negative) {
        return true;
    }

    /* If they're positive, check which one has the greater absolute value */
    if (!num->is_negative) {
        return bigint_abs_gt(num, other);
    } else {
        return bigint_abs_lt(num, other);
    }
}


static inline void bigint_normalize(bigint_t *num) {

    if (num->array_info.count > 0) {
        uint32_t leading = num->items[num->array_info.count - 1];
        while (num->array_info.count > 0 && leading == 0) {
            if (--num->array_info.count == 0) {
                break;
            }
            leading = num->items[num->array_info.count - 1];
        }
    }
 
    if (num->array_info.count == 0) {
        num->is_negative = false;
    }
}
/* Performs the addition algorithm on the digits, regardless of the sign of the operators */
static inline void __bigint_add_helper(bigint_t *num, const bigint_t *other) {

    size_t max_count = num->array_info.count > other->array_info.count ? num->array_info.count : other->array_info.count;

    uint64_t carry = 0;
    for (size_t i = 0; i < max_count; ++i) {
        uint64_t sum = carry;

        if (i < other->array_info.count) {
            sum += other->items[i];
        }

        if (i < num->array_info.count) {
            sum += num->items[i];
            carry = sum / BASE;
            num->items[i] = sum % BASE;
        } else {
            carry = sum / BASE;
            num->items = da_append(num->items, &num->array_info, &sum);
        }
    }

    if (carry > 0) {
        num->items = da_append(num->items, &num->array_info, &carry);
    }
}

/* Performs the subtraction algorithm on the digits and flips the sign of the first if needed. */
static inline void __bigint_sub_helper(bigint_t *num, const bigint_t *other) {

    if (bigint_eq(num, other)) {
        num->array_info.count = 0;
        num->is_negative = false;
        return;
    }

    if (bigint_abs_gt(num, other)) {
        for (size_t i = 0; i < num->array_info.count; ++i) {
            uint64_t current_num = num->items[i];
            /* uint32_t current_other = i < other->count ? other->items[i] : 0; */
            uint32_t current_other = (i < other->array_info.count) * other->items[i];

            if (current_num < current_other) {
                size_t j = i + 1;
                while (j < num->array_info.count && num->items[j] == 0) {
                    num->items[j++] = BASE - 1;
                }
                num->items[j] -= 1;
                current_num += BASE;
            }

            num->items[i] = (current_num - current_other) % BASE;
        }

    } else {
        /*@todo: Check if we can avoid this allocation when doing reciprocal subtraction */
        bigint_t temp = bigint_with_capacity(other->array_info.count, num->array_info.allocator, num->array_info.alloc_ctx);
        bigint_copy(&temp, other);

        /* Swap the inputs of the algorithm and flip the sign of the result */
        for (size_t i = 0; i < temp.array_info.count; ++i) {
            uint64_t current_other = temp.items[i];
            /* uint32_t current_num = i < num->count ? num->items[i] : 0; */
            uint32_t current_num = (i < num->array_info.count) * num->items[i];


            if (current_num > current_other) {
                size_t j = i + 1;
                while (j < temp.array_info.count && temp.items[j] == 0) {
                    temp.items[j++] = BASE - 1;
                }
                temp.items[j] -= 1;
                current_other += BASE;
            }


            /* This can only happen when the subtrahend is bigger */
            uint64_t sub = (current_other - current_num) % BASE;
            if (num->array_info.count > i) {
                num->items[i] = sub;
            } else {
                num->items = da_append(num->items, &num->array_info, &sub);
            }
        }

        /* Flip the sign */
        num->is_negative = (!num->is_negative);

        /* Clean up temporary allocation */
        da_free(temp.items, &temp.array_info);
    }
}

void bigint_add_in(bigint_t *num, const bigint_t *other) {

    if (num->is_negative == other->is_negative) {
        __bigint_add_helper(num, other);
    } else {
        __bigint_sub_helper(num, other);
    }

    bigint_normalize(num);
}

void bigint_sub_in(bigint_t *num, const bigint_t *other) {

    if (num->is_negative != other->is_negative) {
        __bigint_add_helper(num, other);
    } else {
        __bigint_sub_helper(num, other);
    }

    bigint_normalize(num);
}

void bigint_mul_in(bigint_t *num, const bigint_t *other) {

    /* The general idea is to store the product in memory after */
    /* the current num, then replace num with the product: */
    /* [num] -> [num|prod] -> [prod] */
    size_t needed_capacity = 2 * num->array_info.count + other->array_info.count; /* num_count + (num_count + other_count) */

    /* Reserve the capacity for the product after the number itself (extend and reuse the pointer) */
    /* @sneaky_allocation */
    num->items = da_reserve(num->items, &num->array_info, needed_capacity);

    /* Store the original num_count */
    size_t num_count = num->array_info.count;

    /* Make sure everything after the count is zeroed */
    for (size_t i = num->array_info.count; i < needed_capacity; ++i) {
        num->items[i] = 0;
    }

    /* Start of the product */
    size_t prod_start_idx = num->array_info.count;
    size_t prod_count;


    for (size_t i = 0; i < num_count; ++i) {
        uint32_t carry = 0;
        for (size_t j = 0; j < other->array_info.count; ++j) {

            size_t offset = prod_start_idx + i + j;
            carry += num->items[offset];

            uint64_t prod = ((uint64_t)num->items[i] * (uint64_t)other->items[j]) + (uint64_t)carry;

            num->items[offset] = prod % BASE;
            carry = prod / BASE;

            /* branchless code */
            prod_count = i + j + 1;
        }
        if (carry > 0) {
            num->items[prod_start_idx + prod_count] = carry;
            ++prod_count;
        }
    }

    /* Copy the product back to the start of the number */
    for (size_t i = 0; i < prod_count; ++i) {
        num->items[i] = num->items[prod_start_idx + i];
    }

    num->array_info.count = prod_count;
    num->is_negative = (num->is_negative != other->is_negative);

    bigint_normalize(num);
}

void bigint_mul_in_u64(bigint_t *num, const uint64_t other) {

    /* Reserve the capacity for an extra item */
    num->items = da_reserve(num->items, &num->array_info, num->array_info.count + 1);
    num->items[num->array_info.count] = 0;

    /* Store the original num_count */
    size_t num_count = num->array_info.count;


    uint64_t carry = 0;
    for (size_t i = 0; i < num_count; ++i) {

        uint64_t prod = ((uint64_t)num->items[i] * (uint64_t)other) + carry;

        num->items[i] = prod % BASE;
        carry = prod / BASE;

    }
    if (carry > 0) {
        num->items[num->array_info.count] = carry;
        ++num->array_info.count;
    }

    bigint_normalize(num);
}

void bigint_div_in_u32(bigint_t *num, const uint32_t other, uint32_t *remainder) {

    uint32_t carry = 0;
    for (size_t j = num->array_info.count; j > 0; --j) {
        size_t i = j - 1;

        uint64_t factor = (uint64_t)num->items[i] + (uint64_t)carry * (uint64_t)BASE;
        num->items[i] = factor / other;
        carry = factor % other;
    }

    *remainder = carry;

    bigint_normalize(num);
}

/*
 * A very straightforward but probably inefficient implementation of long division.
 *
 * @todo: Look into Donald Knuth's division algorithm again, from the description found on:
 * https://skanthak.hier-im-netz.de/division.html
 */
divmod_t bigint_divmod(const bigint_t *dividend, const bigint_t *divisor, const allocator_t *allocator, void *alloc_ctx, error_t *err) {

    divmod_t result = {
        .quotient  = bigint_with_capacity(dividend->array_info.count - divisor->array_info.count + 1, allocator, alloc_ctx),
        .remainder = bigint_with_capacity(divisor->array_info.count, allocator, alloc_ctx),
    };

    if (divisor->array_info.count == 0 || divisor->items[divisor->array_info.count-1] == 0) {
        strcpy(err->error_msg, "Division by 0 is not allowed");
        err->is_error = true;
        return result;
    }

    /* Easy case */
    if (bigint_abs_lt(dividend, divisor)) {
        bigint_copy(&result.remainder, dividend);
        return result;
    }

    /* Create a non-negative version of the divisor to help calculate some operations */
    const bigint_t abs_divisor = {
        .is_negative = false,
        .array_info = divisor->array_info,
        .items = divisor->items
    };

    bigint_t temp = bigint_with_capacity(dividend->array_info.count, allocator, alloc_ctx);

    for (size_t j = dividend->array_info.count; j > 0; --j) {

        /* [ _ _ _ _ _ _ _] */
        /*                 ^j */
        /* Define the window for each division step */
        /* [ _ _ _ _ _ |_ _| ] */
        /*              ^start */
        /* [ _ _ _ _ _ |_ _|] */
        /*                ^end */
        size_t start;
        size_t end = j - 1;


        if (j == dividend->array_info.count) {
            /* On the first iteration, the window can take more than one digit */
            start = end >= divisor->array_info.count
                ? (end - divisor->array_info.count) + 1
                : 0;
            j = start+1;
        } else {
            /* Every subsequent iteration only takes one digit at a time */
            start = end;
        }


        bigint_mul_in_u64(&result.remainder, BASE);
        /* Multiply the remainder by the base and add it to the next slice */
        /* The trick here is to use the memory space reserved by the remainder */
        /* to store the slice and use it to calculate the next remainder */
        uint32_t remainder_carry = 0;
        for (size_t i = start; i <= end; ++i) { 
            uint64_t sum = remainder_carry;            

            sum += dividend->items[i];

            if (result.remainder.array_info.count > (i - start)) {
                result.remainder.items[i - start] += sum % BASE;
            } else {
                uint64_t item = sum % BASE;
                result.remainder.items = da_append(result.remainder.items, &result.remainder.array_info, &item);
            }
            remainder_carry = sum / BASE;
        }


        /* Find the greatest multiple of the divisor that is less then the dividend slice */
        uint64_t factor;
        /* Do a binary search to find the correct factor */
        uint64_t low  = 0;
        uint64_t high = BASE;
        while (low < high) {
            factor = (low + high) / 2;
            bigint_copy(&temp, &abs_divisor);
            bigint_mul_in_u64(&temp, factor);

            if (bigint_abs_eq(&temp, &result.remainder)) {
                break;
            } else if (bigint_abs_lt(&temp, &result.remainder)) {
                low = factor+1;
            } else {
                high = factor;
            }
        }
        /* This quotient will be at most off by one */
        if (bigint_abs_gt(&temp, &result.remainder)) {
            /* bigint_copy(&temp, divisor); */
            /* bigint_mul_in_u64(&temp, --factor); */
            bigint_sub_in(&temp, &abs_divisor);
            --factor;
        }

        /* Subtract the temporary multiplication from the slice to get the new remainder */
        /* Equivalent of bigint_sub_in(&result.remainder, &temp) but disconsidering the sign */
        for (size_t i = 0; i < result.remainder.array_info.count; ++i) {
            uint64_t current_num = result.remainder.items[i];
            uint64_t current_other = (i < temp.array_info.count) * temp.items[i];

            if (current_num < current_other) {
                size_t j = i + 1;
                while (j < result.remainder.array_info.count && result.remainder.items[j] == 0) {
                    result.remainder.items[j++] = BASE - 1;
                }
                result.remainder.items[j] -= 1;
                current_num += BASE;
            }

            result.remainder.items[i] = (current_num - current_other) % BASE;
        }

        /* Append the factor to the quotient (at this point the quotient array is in reverse order) */
        da_append(result.quotient.items, &result.quotient.array_info, &factor);
    }
    
    /* Reverse the quotient items to make it little-endian */
    da_reverse(result.quotient.items, &result.quotient.array_info);
    bigint_normalize(&result.remainder);

    /* Sign shenaningans */
    if (dividend->is_negative != divisor->is_negative) {
        result.quotient.is_negative = true;
        bigint_decrement(&result.quotient);
        bigint_sub_in(&result.remainder, &abs_divisor);
    }

    /* The remainder can only be negative if the divisor is negative */
    result.remainder.is_negative = divisor->is_negative && result.remainder.array_info.count > 0;

    bigint_normalize(&result.quotient);
    bigint_normalize(&result.remainder);

    return result;
};

/* Increment the absolute value ignoring the sign */
static void __bigint_decrement_helper(bigint_t *num) {
    size_t curr_idx = 0;
    while (num->items[curr_idx] == 0) {
        num->items[curr_idx++] = BASE - 1;
    }
    num->items[curr_idx] -= 1;
}

/* Decrement the absolute value ignoring the sign */
static void __bigint_increment_helper(bigint_t *num) {
    size_t curr_idx = 0;
    while (num->items[curr_idx] == (BASE - 1)) {
        if (curr_idx == num->array_info.count) {
            static uint32_t zero = 0;
            da_append(num, &num->array_info, &zero);
        }
        num->items[curr_idx++] = 0;
    }
    num->items[curr_idx] += 1;
}

void bigint_decrement(bigint_t *num) {

    if (num->array_info.count == 0) {
        num->is_negative = true;
        static uint64_t one = 1;
        num->items = da_append(num->items, &num->array_info, &one);
    }

    if (num->is_negative) {
        __bigint_increment_helper(num);
        return;
    } else {
        __bigint_decrement_helper(num);
        return;
    }
}

void bigint_increment(bigint_t *num) {

    if (num->array_info.count == 0) {
        num->is_negative = false;
        static uint64_t one = 1;
        num->items = da_append(num->items, &num->array_info, &one);
    }

    if (num->is_negative) {
        __bigint_decrement_helper(num);
        return;
    } else {
        __bigint_increment_helper(num);
        return;
    }
}

#endif /* #ifdef BIGINT_IMPL */

#endif /* #ifndef BIGINT_H */
