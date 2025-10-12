#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

typedef struct error_t error_t;

// This struct should be passed by pointer to functions that might
// result in an error.
struct error_t {
    // Whether an error was found
    bool is_error;
    // The error message
    char error_msg[1024];
};
#endif
