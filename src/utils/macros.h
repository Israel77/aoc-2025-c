#ifndef MACROS_H
#define MACROS_H

/* Compiler hints */
#if defined(__GNUC__) && (__GNUC__ >= 3)
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

/* Useful macros for protyping unfinished code. */
#define UNUSED(value) (void)(value)
#define TODO(message) do { fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); abort(); } while(0)
#define UNREACHABLE(message) do { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

/* Macros for testing */
#define TEST_OK(msg)    do { printf("\033[32m [  OK ] %s \033[0m\n", msg); ++tests_passed; } while (0)
#define TEST_FAIL(msg)  do { printf("\033[31m [ FAIL] %s \033[0m\n", msg); ++tests_failed; } while (0)

/* Align x up to a multiple of n where n is a power of 2 */
#define ALIGN_POW_2(x, n) (( (x) + ( (n) - 1 )) & ~( (n) - 1 ))

#define max(a, b) ((a) > (b)) ? (a) : (b)
#define min(a, b) ((a) < (b)) ? (a) : (b)

#define KB(x) (x) * 1024
#define MB(x) (x) * 1024 * 1024

#endif
