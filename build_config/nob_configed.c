#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#include "../nob.h"

#include "dirs.h"

#include "../build/config.h"

typedef const char* cstr;

typedef struct {
    cstr   *items;
    size_t count;
    size_t capacity;
} cstr_array_t;

typedef struct {
    _Bool auto_run;
    char *directory;
    char *file;
    char *output;
    Nob_Cmd *arguments;
} build_info_t;

typedef struct {
    build_info_t *items;
    size_t count;
    size_t capacity;
} compile_commands_t;

static cstr_array_t build_paths;
static compile_commands_t compile_commands;
static char base_path[MAX_FILE_PATH];

static void get_base_path();
static int create_output_dirs();
static void include_utils_tests(void);
static void include_solutions(void);
static int build_from_src(void);
static void include_info_only(void);
static int gen_compile_commands(void *compile_commands);
static int run_programs(void);

thrd_t compile_cmds_thread;

int main(void)
{
    int result = 0;

    get_base_path();

    if (create_output_dirs() != 0) return 1;

    if (BUILD_UTILS_TESTS) {
        include_utils_tests();
    }

    if (BUILD_SOLUTIONS) {
        include_solutions();
    }

    result = build_from_src();
    if (result > 0) return result;

    if (GEN_COMPILE_COMMANDS) {
        include_info_only();
        thrd_create(&compile_cmds_thread, gen_compile_commands, &compile_commands);
    }

    if (RUN_PROGS) {
        result = run_programs();
    }

    thrd_join(compile_cmds_thread, NULL);

    return result;
}

static int run_programs(void) {

    Nob_Cmd cmd = {0};

    for (size_t i = 0; i < compile_commands.count; ++i) {

        if (!compile_commands.items[i].auto_run) continue;

        if (ENABLE_PERF) {
            nob_cmd_append(&cmd, "perf", "stat", "-e", PERF_EVENTS);
        }

        nob_cmd_append(&cmd, compile_commands.items[i].output);

        if (!nob_cmd_run(&cmd)) return 1;
    }

    return 0;
}

static inline void get_base_path() {
    if (getcwd(base_path, sizeof (base_path)) != NULL) {
        nob_log(NOB_INFO, "Base path: %s", base_path);
        strcat(base_path, "/");
    } else {
        fprintf(stderr, "Could not get base path");
        abort();
    }
}

static int create_output_dirs() {

    Nob_Log_Level restore_log_level = nob_minimal_log_level;

    if (SILENT_BUILD_DIRS) {
        nob_minimal_log_level = NOB_WARNING;
    }

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

    if (SILENT_BUILD_DIRS) {
        nob_minimal_log_level = restore_log_level;
    }

    return 0;
}

/* Include build info for files that are not built by nob_configed (including itself) */
static void include_info_only(void) {

    /* nob.c */
    {
        Nob_Cmd *cmd = malloc(sizeof (Nob_Cmd));
        cmd->items = NULL;
        cmd->count = 0;
        cmd->capacity = 0;

        char *input_file = "nob.c";
        // nob_log(NOB_INFO, "Input file: %s", input_file);

        char *output_file = "nob";
        // nob_log(NOB_INFO, "Output file: %s", output_file);

        build_info_t command_data;
        command_data.file = input_file;
        command_data.directory = base_path;
        command_data.auto_run = false;

        nob_cc(cmd);
        nob_cc_output(cmd, output_file);
        nob_cc_inputs(cmd, input_file);
        
        command_data.arguments = cmd;
        command_data.output = output_file;
        command_data.auto_run = false;
        nob_da_append(&compile_commands, command_data);
    }

    /* nob_configed.c */
    {
        Nob_Cmd *cmd = malloc(sizeof (Nob_Cmd));
        cmd->items = NULL;
        cmd->count = 0;
        cmd->capacity = 0;

        char *input_file = SRC_BUILD_FOLDER"nob_configed.c";
        // nob_log(NOB_INFO, "Input file: %s", input_file);

        char *output_file = BUILD_FOLDER"nob_configed";
        // nob_log(NOB_INFO, "Output file: %s", output_file);

        build_info_t command_data;
        command_data.file = input_file;
        command_data.directory = base_path;

        nob_cc(cmd);
        nob_cc_flags(cmd);
        nob_cmd_append(cmd, "-I.", "-I"BUILD_FOLDER, "-I"SRC_BUILD_FOLDER); // -I is usually the same across all compilers
        nob_cc_output(cmd, output_file);
        nob_cc_inputs(cmd, input_file);
        
        command_data.arguments = cmd;
        command_data.output = output_file;
        nob_da_append(&compile_commands, command_data);
    }
}

static void include_utils_tests(void) {
    nob_da_append(&build_paths,"utils/tests/std_allocator_test");
    nob_da_append(&build_paths, "utils/tests/arena_allocator_test");
    // nob_da_append(&build_paths, "utils/tests/fixed_pool_allocator_test");
    nob_da_append(&build_paths, "utils/tests/string_utils_test");
    nob_da_append(&build_paths, "utils/tests/bigint_test");
    nob_da_append(&build_paths, "utils/tests/hashmap_tests");
    nob_da_append(&build_paths, "utils/tests/parsing_helpers_test");
}

void include_solutions(void) {

    // if (nob_file_exists("solutions/template/main.c") == 0)
    //     nob_da_append(&build_paths,"solutions/template/main");


    if (COMPILE_SOLUTIONS[0] == 0) {
        for (int day = 1; day <= 25; ++day) {
            char file_with_extension[MAX_FILE_PATH];
            char *file_no_extension   = malloc(MAX_FILE_PATH - 2);

            sprintf(file_with_extension, SRC_FOLDER"solutions/day%02d/main.c", day);
            sprintf(file_no_extension, "solutions/day%02d/main", day);

            if (nob_file_exists(file_with_extension) > 0) 
                nob_da_append(&build_paths,file_no_extension);
            
        }
    } else {
        size_t number_of_days = sizeof (COMPILE_SOLUTIONS) / sizeof (COMPILE_SOLUTIONS[0]);

        for (size_t i = 0; i < number_of_days; ++i) {

            int day = COMPILE_SOLUTIONS[i];

            char file_with_extension[MAX_FILE_PATH];
            char *file_no_extension   = malloc(MAX_FILE_PATH - 2);

            sprintf(file_with_extension, SRC_FOLDER"solutions/day%02d/main.c", day);
            sprintf(file_no_extension, "solutions/day%02d/main", day);

            if (nob_file_exists(file_with_extension) > 0) 
                nob_da_append(&build_paths,file_no_extension);
        }

    }

}

static int build_from_src(void) {

    Nob_Procs procs = {0};

    // Spawn one async process per target collecting them to procs dynamic array
    // For each program name we will look for SRC_FOLDER/{program_path}.c and build equivalent optimized and debug versions
    for (size_t i = 0; i < build_paths.count; ++i) {

        /* Who cares about memory leaks in 2025? */
        Nob_Cmd *cmd = malloc(sizeof (Nob_Cmd));
        Nob_Cmd *cmd_dbg = malloc(sizeof (Nob_Cmd));

        const char *program_path = build_paths.items[i];

        char *input_file = malloc(MAX_FILE_PATH);
        strcpy(input_file, base_path);
        strcat(input_file, SRC_FOLDER);
        strcat(input_file, program_path);
        strcat(input_file, ".c");
        // nob_log(NOB_INFO, "Input file: %s", input_file);

        char *output_file = malloc(MAX_FILE_PATH);
        strcpy(output_file, base_path);
        strcat(output_file, BUILD_FOLDER);
        strcat(output_file, program_path);
        // nob_log(NOB_INFO, "Output file: %s", output_file);

        char *output_file_dbg = malloc(MAX_FILE_PATH);
        strcpy(output_file_dbg, base_path);
        strcat(output_file_dbg, BUILD_FOLDER);
        strcat(output_file_dbg, program_path);
        strcat(output_file_dbg, "_dbg");
        // nob_log(NOB_INFO, "Output file (debug): %s", output_file_dbg);

        build_info_t command_data;
        command_data.file = input_file;
        command_data.directory = base_path;

        // Optimized version
        nob_cc(cmd);
        nob_cc_flags(cmd);
        nob_cmd_append(cmd, "-O3", "-g", "-Wno-unused-function", "-march=znver4","-std=c11", "-DALLOC_STD_IMPL", "-D_DEFAULT_SOURCE");
        nob_cc_output(cmd, output_file);
        nob_cc_inputs(cmd, input_file);
        
        command_data.arguments = cmd;
        command_data.output = output_file;
        command_data.auto_run = true;
        nob_da_append(&compile_commands, command_data);

        if (BUILD_ASYNC) {
            if (!nob_cmd_run(cmd, .async = &procs, .no_reset = true)) return 1;
        } else {
            if (!nob_cmd_run(cmd, .no_reset = true)) return 1;
        }

        // Debug version
        nob_cc(cmd_dbg);
        nob_cc_flags(cmd_dbg);
        nob_cmd_append(cmd_dbg, "-O0",  "-g", "-Wno-unused-function", "-march=znver4", "-std=c11", "-DDEBUG_MODE", "-DALLOC_STD_IMPL","-D_DEFAULT_SOURCE", "-finstrument-functions");
        nob_cc_output(cmd_dbg, output_file_dbg);
        nob_cc_inputs(cmd_dbg, input_file);
        
        command_data.arguments = cmd_dbg;
        command_data.output = output_file_dbg;
        command_data.auto_run = false;
        nob_da_append(&compile_commands, command_data);

        if (BUILD_ASYNC) {
            if (!nob_cmd_run(cmd_dbg, .async = &procs, .no_reset = true)) return 1;
        } else {
            if (!nob_cmd_run(cmd_dbg, .no_reset = true)) return 1;
        }
    }

    // Wait on all the async processes to finish and reset procs dynamic array to 0
    if (!nob_procs_flush(&procs)) return 1;

    return 0;
}

static int gen_compile_commands(void *cmds) {

    compile_commands_t *commands = cmds;

    FILE *output = fopen("compile_commands.json", "w");
    if (!output) {
        nob_log(NOB_WARNING, "Could not write to file compile_commands.json");
        return 1;
    }
    
    nob_log(NOB_INFO, "GEN: compile_commands.json");

    fputs("[\n", output);
    
    for (size_t i = 0; i < commands->count; ++i) {
        build_info_t cmd = commands->items[i];

        fputs("    {\n", output);

        fprintf(output, "        \"directory\": \"%s\",\n", cmd.directory);
        fprintf(output, "        \"file\": \"%s\",\n", cmd.file);
        fprintf(output, "        \"output\": \"%s\",\n", cmd.output);

        fprintf(output, "        \"arguments\": [\n");
        for (size_t j = 0; j < cmd.arguments->count; ++j) {
            const char* arg = cmd.arguments->items[j];
            fprintf(output, "        \"%s\"", arg);

            if (j == cmd.arguments->count - 1) {
                fputs("\n", output);
            } else {
                fputs(",\n", output);
            }
        }
        fprintf(output, "        ]\n");

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

