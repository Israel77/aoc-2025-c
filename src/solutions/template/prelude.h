// Custom utilities without functions (only macros and types)
#include "../../utils/da.h" // IWYU pragma: export
#include "../../utils/todo.h" // IWYU pragma: export
#include "../../utils/error.h" // IWYU pragma: export

// Utilites in order of dependency from each other
#ifndef ALLOC_STD_IMPL
#define ALLOC_STD_IMPL
#endif
#define ALLOC_ARENA_IMPL 
//#define ALLOC_FIXED_POOL_IMPL
#include "../../utils/allocator.h" // IWYU pragma: export

#define STRING_UTILS_IMPL
#include "../../utils/string_utils.h" // IWYU pragma: export

#define BIGINT_IMPL
#include "../../utils/bigint.h" // IWYU pragma: export
