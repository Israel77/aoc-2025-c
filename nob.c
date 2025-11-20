#include <string.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_WARN_DEPRECATED
#include "nob.h"

#include "build_config/dirs.h"

static _Bool defconfig = false;

void parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "nob.h", "build_config/dirs.h");

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    parse_args(argc, argv);

    Cmd cmd = {0};
    Nob_String_Builder sb = {0};

    const char *conf_path = BUILD_FOLDER"config.h";
    int exists = file_exists(conf_path);
    if (exists < 0) return 1;
    if (exists == 0 || defconfig) {
        nob_log(INFO, "Generating initial %s", conf_path);
        sb_append_cstr(&sb, "#ifndef CONFIG_H_\n");
        sb_append_cstr(&sb, "#define CONFIG_H_\n");
        /* ----- Compilation options ----- */
        sb_append_cstr(&sb, "\n/* ----- Compilation options ----- */\n");
        sb_append_cstr(&sb, "/* Compile the solution for these days (0 will compile all) */\n");
        sb_append_cstr(&sb, "static const unsigned int COMPILE_SOLUTIONS[] = {0};\n");
        sb_append_cstr(&sb, "#define BUILD_SOLUTIONS   1    /* Compile the solutions for each day */\n");
        sb_append_cstr(&sb, "#define BUILD_UTILS_TESTS 0    /* Compile the tests for the utilities */\n");
        sb_append_cstr(&sb, "#define BUILD_ASYNC 1          /* Compile programs concurrently */\n");

        /* ----- Run options ----- */
        sb_append_cstr(&sb, "\n/* ----- Run options ----- */\n");
        sb_append_cstr(&sb, "#define RUN_PROGS 0            /* Run all the programs that were compiled */\n");
        sb_append_cstr(&sb, "#define ENABLE_PERF 0          /* Enable perf when running the programs (requires RUN_PROGS) */\n");
        sb_append_cstr(&sb, "/* Which events to monitor (if perf is enabled) */\n");
        sb_append_cstr(&sb, "#define PERF_EVENTS \"cycles,instructions,cache-references,cache-misses,branches,branch-misses\"\n");

        /* ----- Other configs ----- */
        sb_append_cstr(&sb, "\n/* ----- Other configs ----- */\n");
        sb_append_cstr(&sb, "#define SILENT_BUILD_DIRS 1    /* Do not log creation of existing directories */\n");
        sb_append_cstr(&sb, "#define GEN_COMPILE_COMMANDS 1 /* Generate compile_commands.json */\n");

        /* ----- General constants ----- */
        sb_append_cstr(&sb, "\n/* ----- General constants ----- */\n");
        sb_append_cstr(&sb, "#define MAX_FILE_PATH     4096 /* Maximum number of characters in a file path */\n");
        sb_append_cstr(&sb, "#endif // CONFIG_H_\n");
        if (!nob_write_entire_file(conf_path, sb.items, sb.count)) return 1;
        sb.count = 0;
        nob_log(INFO, "==================================");
        nob_log(INFO, "EDIT %s TO CONFIGURE YOUR BUILD!!!", conf_path);
        nob_log(INFO, "==================================");
    }

    const char *output_path = BUILD_FOLDER"nob_configed";
    const char *input_path = SRC_BUILD_FOLDER"nob_configed.c";
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-I.", "-I"BUILD_FOLDER, "-I"SRC_BUILD_FOLDER); // -I is usually the same across all compilers
    nob_cmd_append(&cmd, "-g"); // -I is usually the same across all compilers
    nob_cc_output(&cmd, output_path);
    nob_cc_inputs(&cmd, input_path);
    if (!cmd_run(&cmd)) return 1;

    cmd_append(&cmd, output_path);
    if (!cmd_run(&cmd)) return 1;

    return EXIT_SUCCESS;
}

void parse_args(int argc, char **argv) {

    while(argc > 0) {
        defconfig = strcmp(nob_shift(argv, argc), "defconfig") == 0;
    }

}
