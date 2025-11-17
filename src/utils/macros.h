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
#define TEST_ASSERT(condition, msg) do {     \
        if ((condition)) TEST_OK(msg);       \
        else TEST_FAIL(msg);                 \
    } while (0)

/* Align x up to a multiple of n where n is a power of 2 */
#define ALIGN_POW_2(x, n) (( (x) + ( (n) - 1 )) & ~( (n) - 1 ))

#define PROF_START(x) { const char *section = (x); uint64_t __clock_start = now_ns();
#define PROF_END  uint64_t __clock_end = now_ns(); printf("%s took: %'ld ns\n", section, __clock_end - __clock_start);}

#define max(a, b) ((a) > (b)) ? (a) : (b)
#define min(a, b) ((a) < (b)) ? (a) : (b)
#define sign_of(x) ((x) == 0) ? (0) : (((x) > 0) ? (1) : (-1))

#define KB(x) (x) * 1024
#define MB(x) (x) * 1024 * 1024

#endif
