#include "src/utils/todo.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <threads.h>

#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#include "nob.h"

#define ALLOC_STD_IMPL
#define ALLOC_ARENA_IMPL
#include "src/utils/allocator.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

static const int MAX_FILE_PATH = 4096;

typedef const char* cstr;

typedef struct {
    cstr   *items;
    size_t count;
    size_t capacity;
} cstr_array_t;


typedef struct {
    cstr directory;
    cstr file;
    cstr output;
    Nob_Cmd *arguments;
} command_t;

typedef struct {
    command_t *items;
    size_t count;
    size_t capacity;
} compile_commands_t;

static int async = 1;
static cstr_array_t program_paths;
static compile_commands_t compile_commands;

static int create_build_dirs();
static void include_tests(void);
static void include_solutions(void);
static int build_programs_async(void);
static int gen_compile_commands(void *arg);

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (create_build_dirs() != 0) return 1;

    include_tests();
    // include_solutions();

    return build_programs_async();
}

static int create_build_dirs() {

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER"solutions")) return 1;
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER"solutions/template")) return 1;
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER"utils")) return 1;
    if (!nob_mkdir_if_not_exists(BUILD_FOLDER"utils/tests")) return 1;

    // Create a directory for each day
    char buffer[1024];
    for (int d = 1; d <= 25; ++d) {
        if (d < 10) {
            sprintf(buffer, "%ssolutions/day0%d", BUILD_FOLDER, d);
        } else {
            sprintf(buffer, "%ssolutions/day%d", BUILD_FOLDER, d);
        }
        if (!nob_mkdir_if_not_exists(buffer)) return 1;
    }

    return 0;
}

static void include_tests(void) {
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths,"utils/tests/std_allocator_test");
    nob_da_append(&program_paths, "utils/tests/arena_allocator_test");
    nob_da_append(&program_paths, "utils/tests/fixed_pool_allocator_test");
    nob_da_append(&program_paths, "utils/tests/string_utils_test");
    nob_da_append(&program_paths, "utils/tests/bigint_test");
    nob_da_append(&program_paths, "utils/tests/hashmap_tests");
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

static int build_programs_async(void) {

    multiarena_context_t build_ctx = {
        .inner_alloc = &global_std_allocator,
        .inner_ctx = NULL
    };

    Nob_Procs procs = {0};

    char base_path[MAX_FILE_PATH];

    if (getcwd(base_path, sizeof (base_path)) != NULL) {
        nob_log(NOB_INFO, "Base path: %s", base_path);
    } else {
        fprintf(stderr, "Could not get base path");
        abort();
    }

    strcat(base_path, "/");

    // Spawn one async process per target collecting them to procs dynamic array
    // For each program name we will look for SRC_FOLDER/{program_path}.c and build equivalent optimized and debug versions
    for (size_t i = 0; i < program_paths.count; ++i) {

        Nob_Cmd *cmd = multiarena_alloc(&build_ctx, sizeof (Nob_Cmd));
        Nob_Cmd *cmd_dbg = multiarena_alloc(&build_ctx, sizeof (Nob_Cmd));

        command_t command_data;

        const char *program_path = program_paths.items[i];

        char *input_file = multiarena_alloc(&build_ctx, MAX_FILE_PATH);
        strcpy(input_file, base_path);
        strcat(input_file, SRC_FOLDER);
        strcat(input_file, program_path);
        strcat(input_file, ".c");
        // nob_log(NOB_INFO, "Input file: %s", input_file);

        char *output_file = multiarena_alloc(&build_ctx, MAX_FILE_PATH);
        strcpy(output_file, base_path);
        strcat(output_file, BUILD_FOLDER);
        strcat(output_file, program_path);
        // nob_log(NOB_INFO, "Output file: %s", output_file);

        char *output_file_dbg = multiarena_alloc(&build_ctx, MAX_FILE_PATH);
        strcpy(output_file_dbg, base_path);
        strcat(output_file_dbg, BUILD_FOLDER);
        strcat(output_file_dbg, program_path);
        strcat(output_file_dbg, "_dbg");
        // nob_log(NOB_INFO, "Output file (debug): %s", output_file_dbg);

        command_data.file = input_file;
        command_data.directory = base_path;

        // Optimized version
        nob_cc(cmd);
        nob_cc_flags(cmd);
        nob_cmd_append(cmd, "-O3", "-g", "-std=c11", "-march=znver4", "-DALLOC_STD_IMPL");
        nob_cc_output(cmd, output_file);
        nob_cc_inputs(cmd, input_file);
        
        command_data.arguments = cmd;
        command_data.output = output_file;
        nob_da_append(&compile_commands, command_data);

        if (async && !nob_cmd_run(cmd, .async = &procs) ) {
            return 1;
        } else if (!nob_cmd_run(cmd)) {
            return 1;
        }

        // Debug version
        nob_cc(cmd_dbg);
        nob_cc_flags(cmd_dbg);
        nob_cmd_append(cmd_dbg, "-Og", "-g", "-std=c11", "-DDEBUG_MODE", "-DALLOC_STD_IMPL");
        nob_cc_output(cmd_dbg, output_file_dbg);
        nob_cc_inputs(cmd_dbg, input_file);
        
        command_data.arguments = cmd_dbg;
        command_data.output = output_file_dbg;
        nob_da_append(&compile_commands, command_data);

        if (async && !nob_cmd_run(cmd_dbg, .async = &procs) ) {
            return 1;
        } else if (!nob_cmd_run(cmd_dbg)) {
            return 1;
        }
    }

    // Write compile_commands to a file
    thrd_t gen_thread;
    thrd_create(&gen_thread, gen_compile_commands, &compile_commands);

    // Wait on all the async processes to finish and reset procs dynamic array to 0
    if (!nob_procs_flush(&procs)) return 1;

    thrd_join(gen_thread, NULL);

    multiarena_free_all(&build_ctx);

    return 0;
}

static int gen_compile_commands(void *arg) {

    compile_commands_t *commands = arg;

    FILE *output = fopen("compile_commands.json", "w");
    if (!output) {
        nob_log(NOB_WARNING, "Could not write to file compile_commands.json");
        return 1;
    }
    
    nob_log(NOB_INFO, "GEN: compile_commands.json");

    fputs("[\n", output);
    
    for (size_t i = 0; i < commands->count; ++i) {
        command_t cmd = commands->items[i];

        fputs("    {\n", output);

        fprintf(output, "        \"directory\": %s,\n", cmd.directory);
        fprintf(output, "        \"file\": %s,\n", cmd.file);
        fprintf(output, "        \"output\": %s,\n", cmd.output);

        fprintf(output, "        \"arguments\": [\n", cmd.output);
        for (size_t j = 0; j < cmd.arguments->count; ++j) {
            const char* arg = cmd.arguments->items[j];
            fprintf(output, "        \"%s\"", arg);

            if (j == cmd.arguments->count - 1) {
                fputs("\n", output);
            } else {
                fputs(",\n", output);
            }
        }
        fprintf(output, "        ]\n", cmd.output);

        fputs("    }", output);
        if (i == commands->count - 1) {
            fputs("\n", output);
        } else {
            fputs(",\n", output);
        }
    }

    fputs("]\n", output);

    nob_log(NOB_INFO, "compile_commands.json generated successfully");
    return 0;
}

