/* ----------------------------------------
 * Standard headers
 * ---------------------------------------- */
#include <stdlib.h> // IWYU pragma: export
#include <stdio.h> // IWYU pragma: export
#include <stdint.h> // IWYU pragma: export
#include <stddef.h> // IWYU pragma: export
#include <assert.h> // IWYU pragma: export

/* ----------------------------------------
 * Dynamic arrays
 * ---------------------------------------- */
#include "../../utils/da.h" // IWYU pragma: export

/* ----------------------------------------
 * Allocators
 * ---------------------------------------- */
#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#define ALLOC_ARENA_IMPL
#define ALLOC_FIXED_POOl_IMPL
#include "../../utils/allocator.h" // IWYU pragma: export

/* ----------------------------------------
 * Strings
 * ---------------------------------------- */
#define STRING_UTILS_IMPL
#include "../../utils/string_utils.h" // IWYU pragma: export

