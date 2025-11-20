#include <stdint.h>

/* Unsigned integers */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Signed integers */
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* Floats */
typedef float  f32;
typedef double f64;

/* Misc */
typedef uint8_t byte;
typedef char*   cstr;

/* Not quite typedefs but close enough to be in the same header */
#define internal   static
#define global_var static
#define lingering  static
