#include <stddef.h>
#include <stdio.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "nob.h"

#include <string.h>

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} cstr_array_t;

static cstr_array_t program_paths;

void include_tests(void) {
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths, "utils/tests/arena_allocator_test");
    nob_da_append(&program_paths, "utils/tests/fixed_pool_allocator_test");
    nob_da_append(&program_paths, "utils/tests/string_utils_test");
    nob_da_append(&program_paths, "utils/tests/bigint_test");
}

void include_solutions(void) {
    nob_da_append(&program_paths,"solutions/template/template");
    nob_da_append(&program_paths,"solutions/day01/part1");
    nob_da_append(&program_paths,"solutions/day01/part2");
    // nob_da_append(&program_paths,"solutions/day02/part1");
    // nob_da_append(&program_paths,"solutions/day02/part2");
    // nob_da_append(&program_paths,"solutions/day03/part1");
    // nob_da_append(&program_paths,"solutions/day03/part2");
    // nob_da_append(&program_paths,"solutions/day04/part1");
    // nob_da_append(&program_paths,"solutions/day04/part2");
    // nob_da_append(&program_paths,"solutions/day05/part1");
    // nob_da_append(&program_paths,"solutions/day05/part2");
    // nob_da_append(&program_paths,"solutions/day06/part1");
    // nob_da_append(&program_paths,"solutions/day06/part2");
    // nob_da_append(&program_paths,"solutions/day07/part1");
    // nob_da_append(&program_paths,"solutions/day07/part2");
    // nob_da_append(&program_paths,"solutions/day08/part1");
    // nob_da_append(&program_paths,"solutions/day08/part2");
    // nob_da_append(&program_paths,"solutions/day09/part1");
    // nob_da_append(&program_paths,"solutions/day09/part2");
    // nob_da_append(&program_paths,"solutions/day10/part1");
    // nob_da_append(&program_paths,"solutions/day10/part2");
    // nob_da_append(&program_paths,"solutions/day11/part1");
    // nob_da_append(&program_paths,"solutions/day11/part2");
    // nob_da_append(&program_paths,"solutions/day12/part1");
    // nob_da_append(&program_paths,"solutions/day12/part2");
    // nob_da_append(&program_paths,"solutions/day13/part1");
    // nob_da_append(&program_paths,"solutions/day13/part2");
    // nob_da_append(&program_paths,"solutions/day14/part1");
    // nob_da_append(&program_paths,"solutions/day14/part2");
    // nob_da_append(&program_paths,"solutions/day15/part1");
    // nob_da_append(&program_paths,"solutions/day15/part2");
    // nob_da_append(&program_paths,"solutions/day16/part1");
    // nob_da_append(&program_paths,"solutions/day16/part2");
    // nob_da_append(&program_paths,"solutions/day17/part1");
    // nob_da_append(&program_paths,"solutions/day17/part2");
    // nob_da_append(&program_paths,"solutions/day18/part1");
    // nob_da_append(&program_paths,"solutions/day18/part2");
    // nob_da_append(&program_paths,"solutions/day19/part1");
    // nob_da_append(&program_paths,"solutions/day19/part2");
    // nob_da_append(&program_paths,"solutions/day20/part1");
    // nob_da_append(&program_paths,"solutions/day20/part2");
    // nob_da_append(&program_paths,"solutions/day21/part1");
    // nob_da_append(&program_paths,"solutions/day21/part2");
    // nob_da_append(&program_paths,"solutions/day22/part1");
    // nob_da_append(&program_paths,"solutions/day22/part2");
    // nob_da_append(&program_paths,"solutions/day23/part1");
    // nob_da_append(&program_paths,"solutions/day23/part2");
    // nob_da_append(&program_paths,"solutions/day24/part1");
    // nob_da_append(&program_paths,"solutions/day24/part2");
    // nob_da_append(&program_paths,"solutions/day25/part1");
    // nob_da_append(&program_paths,"solutions/day25/part2");
}

int build_programs_async(void) {

    Cmd cmd = {0};
    Procs procs = {0};

    // Spawn one async process per target collecting them to procs dynamic array
    // For each program name we will look for SRC_FOLDER/{program_path}.c and build equivalent optimized and debug versions
    for (size_t i = 0; i < program_paths.count; ++i) {

        const char *program_path = program_paths.items[i];

        char input_file[4096];
        strcpy(input_file, SRC_FOLDER);
        strcat(input_file, program_path);
        strcat(input_file, ".c");

        char output_file[4096];
        strcpy(output_file, BUILD_FOLDER);
        strcat(output_file, program_path);

        char output_file_dbg[4096];
        strcpy(output_file_dbg, BUILD_FOLDER);
        strcat(output_file_dbg, program_path);
        strcat(output_file_dbg, "_dbg");

        // Optimized version
        nob_cc(&cmd);
        nob_cc_flags(&cmd);
        nob_cmd_append(&cmd, "-O3", "-g", "-std=c11", "-march=znver4", "-DALLOC_STD_IMPL");
        nob_cc_output(&cmd, output_file);
        nob_cc_inputs(&cmd, input_file);
        if (!cmd_run(&cmd, .async = &procs)) return 1;

        // Debug version
        nob_cc(&cmd);
        nob_cc_flags(&cmd);
        nob_cmd_append(&cmd, "-g", "-std=c11", "-DDEBUG_MODE", "-DALLOC_STD_IMPL");
        nob_cc_output(&cmd, output_file_dbg);
        nob_cc_inputs(&cmd, input_file);
        if (!cmd_run(&cmd, .async = &procs)) return 1;
    }


    // Wait on all the async processes to finish and reset procs dynamic array to 0
    if (!procs_flush(&procs)) return 1;

    return 0;
}

int create_build_dirs() {

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    if (!mkdir_if_not_exists(BUILD_FOLDER"solutions")) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER"solutions/template")) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER"utils")) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER"utils/tests")) return 1;

    // Create a directory for each day
    char buffer[1024];
    for (int d = 1; d <= 25; ++d) {
        if (d < 10) {
            sprintf(buffer, "%ssolutions/day0%d", BUILD_FOLDER, d);
        } else {
            sprintf(buffer, "%ssolutions/day%d", BUILD_FOLDER, d);
        }
        if (!mkdir_if_not_exists(buffer)) return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (create_build_dirs() != 0) return 1;

    include_tests();
    // include_solutions();

    return build_programs_async();
}
