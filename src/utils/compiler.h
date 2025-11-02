/* include/linux/compiler.h */

#if defined(__GNUC__) && (__GNUC__ >= 3)
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

