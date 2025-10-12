#include "prelude.h"

typedef struct {
    array_info_t array_info;
    uint32_t *items;
} u32_array_t;

u32_array_t parse_input() {

    FILE *file = fopen("inputs/day_01.txt", "r");

    assert(file && "File not found");

    error_t err = {0};
    arena_context_t ctx = {
        .inner_alloc = &global_std_allocator,
        .inner_ctx = NULL
    };

    u32_array_t result = {
        .array_info = {
            .allocator = &global_std_allocator,
            .alloc_ctx = NULL
        }
    };

    string_builder_t file_sb = sb_read_file(file, &arena_allocator, &ctx);

    string_t file_str = sb_build(&file_sb);

    string_array_t numbers_str = string_split_by_char(&file_str, '\n', &arena_allocator, &ctx);

    da_reserve(&result.array_info, result.items, numbers_str.array_info.count);
    for (size_t i = 0; i < numbers_str.array_info.count; ++i) {

        string_t num_str = numbers_str.items[i];
        if (num_str.count == 0) continue;
        
        uint64_t num = string_parse_u64_safe(&num_str, &err);
        if (err.is_error) {
            goto defer;
        }

        da_append(&result.array_info, result.items, &num);
    }

defer:
    fclose(file);
    arena_free_all(&ctx);

    if (err.is_error) {
        fprintf(stderr, "%s\n", err.error_msg);
        abort();
    }

    return result;
}

int main(void) {

    u32_array_t nums = parse_input();

    uint32_t count_incr = 0;
    for (size_t i = 1; i < nums.array_info.count; ++i) {
        count_incr += (nums.items[i] > nums.items[i-1]);
    }

    printf("%d\n", count_incr);

    nums.array_info.allocator->free(nums.items, nums.array_info.alloc_ctx, nums.array_info.capacity * sizeof(uint32_t));

    return 0;
}
