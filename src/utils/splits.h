#include <stddef.h>
#include <stdbool.h>
#include "typedefs.h"

#define split_by_thread(ctx, count, start, end) split_count_evenly((ctx)->thread_idx, (ctx)->common->thread_count, count, start, end)

internal void split_count_evenly(size_t idx, size_t part_count, size_t count, size_t *start, size_t *end) {

    const size_t tasks_per_thread = count / part_count;
    const size_t remainders = count % part_count;

    const bool take_remainder = idx < remainders;
    const size_t prev_remainders = take_remainder ? idx : remainders;

    *start =  idx   * tasks_per_thread + prev_remainders;
    *end   = *start + tasks_per_thread + (take_remainder ? 1 : 0);
}
