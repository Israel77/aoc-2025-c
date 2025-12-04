#include "prelude.h"
#include "part1.c"
#include "part2.c"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FILE_CAP 100 * 8 * 1024
#define MAX_THREADS 32
#define BENCHMARK_RUNS 8

static unsigned char file_buffer[FILE_CAP];
static arena_context_t *file_arena_ctx;
static arena_context_t solution_arena_ctx;

static allocator_t file_arena;
static allocator_t solution_arena;

static string_t read_file();
static inline void setup();

static pthread_t threads[MAX_THREADS];

static struct part_context_common p1_common;
static struct part_context p1_contexts[P1_THREADS];

static struct part_context_common p2_common;
static struct part_context p2_contexts[P2_THREADS];

string_t input;

static void run_part_1();
static void run_part_2();
static void run_benchmarks();

int main(void) {

    setup();

    printf("\n==== Day 04 ====\n");

#ifdef TEST_IMPL
    run_tests();
#endif

#ifdef PART_1_IMPL
    printf("Solution to part 1:\n");
    run_part_1();
    string_println(&p1_contexts[0].common->output);

    arena_reset(solution_arena.alloc_ctx);
#endif /* ifdef PART1_IMPL */

#ifdef PART_2_IMPL
    printf("Solution to part 2:\n");
    run_part_2();
    string_println(&p2_contexts[0].common->output);
#endif

    run_benchmarks();

    return 0;
}

static void run_benchmarks() {
    volatile uint64_t clock_start;
    volatile uint64_t clock_end;

#ifdef PART_1_IMPL
    u64 p1_times[BENCHMARK_RUNS];
    u16 p1_count = 0;
    for (size_t i = 0; i < BENCHMARK_RUNS; ++i) {
        clock_start = now_ns();
        run_part_1();
        clock_end = now_ns();
        p1_times[p1_count++] = clock_end - clock_start;
        arena_reset(solution_arena.alloc_ctx);
    }
    u64 p1_avg = 0;
    u64 p1_min = UINT64_MAX;
    u64 p1_max = 0;
    for (size_t i = 0; i < BENCHMARK_RUNS; ++i) {
        p1_avg += p1_times[i] / BENCHMARK_RUNS;

        if (p1_times[i] < p1_min) p1_min = p1_times[i];
        if (p1_times[i] > p1_max) p1_max = p1_times[i];
    }
    printf("Part 1 stats (min/max/avg): %'lu, %'lu, %'lu\n", p1_min, p1_max, p1_avg);

#endif /* ifdef PART1_IMPL */


#ifdef PART_2_IMPL
    u64 p2_times[BENCHMARK_RUNS];
    u16 p2_count = 0;
    for (size_t i = 0; i < BENCHMARK_RUNS; ++i) {
        clock_start = now_ns();
        run_part_2();
        clock_end = now_ns();
        p2_times[p2_count++] = clock_end - clock_start;
        arena_reset(solution_arena.alloc_ctx);
    }
    u64 p2_avg = 0;
    u64 p2_min = UINT64_MAX;
    u64 p2_max = 0;
    for (size_t i = 0; i < BENCHMARK_RUNS; ++i) {
        p2_avg += p2_times[i] / BENCHMARK_RUNS;

        if (p2_times[i] < p2_min) p2_min = p2_times[i];
        if (p2_times[i] > p2_max) p2_max = p2_times[i];
    }
    printf("Part 2 stats (min/max/avg): %'lu, %'lu, %'lu\n", p2_min, p2_max, p2_avg);
#endif
}

static void run_part_1() {

    memset(&p1, 0, sizeof (p1));

    if (min(MAX_THREADS, P1_THREADS) > 1) {
        for (size_t i = 0; i < p1_contexts[0].common->thread_count; ++i) {
            pthread_create(&threads[i], NULL, p1_solve, &p1_contexts[i]);
        }

        for (size_t i = 0; i < p1_contexts[0].common->thread_count; ++i) {
            pthread_join(threads[i], NULL);
        }
    } else {
        /* When running single-threaded, call the function directly to avoid overhead
         * and enable optimizations */
        p1_solve(&p1_contexts[0]);
    }
}
static void run_part_2() {
    memset(&p2, 0, sizeof (p2));

    if (min(MAX_THREADS, P2_THREADS) > 1) {
        for (size_t i = 0; i < p2_contexts[0].common->thread_count; ++i) {
            pthread_create(&threads[i], NULL, p2_solve, &p2_contexts[i]);
        }

        for (size_t i = 0; i < p2_contexts[0].common->thread_count; ++i) {
            pthread_join(threads[i], NULL);
        }
    } else {
        /* When running single-threaded, call the function directly to avoid overhead
         * and enable optimizations */
        p2_solve(&p2_contexts[0]);
    }
}

static inline void setup() {

    setlocale(LC_NUMERIC, "pt_BR.UTF-8");

    /* Setup arenas */
    file_arena_ctx = arena_from_buf(file_buffer, FILE_CAP);
    file_arena.alloc_ctx = file_arena_ctx;
    file_arena.interface = &arena_interface;

    solution_arena_ctx = arena_init(4096, ARENA_FAST_ALLOC | ARENA_GROWABLE | ARENA_VIRTUAL_BACKEND, NULL, NULL);
    solution_arena.alloc_ctx = &solution_arena_ctx;
    solution_arena.interface = &arena_interface;

    /* Load the file into memory */
    input = read_file();

    /* Setup contexts for each part */
    p1_common.input = &input;
    p1_common.arena = &solution_arena;
    p1_common.thread_count = min(MAX_THREADS, P1_THREADS);

    pthread_barrier_init(&p1_common.barrier, NULL, p1_common.thread_count);

    for (size_t i = 0; i < p1_common.thread_count; ++i) {
        p1_contexts[i].thread_idx = i;
        p1_contexts[i].common     = &p1_common;
    }

    p2_common.input = &input;
    p2_common.arena = &solution_arena;
    p2_common.thread_count = min(MAX_THREADS, P2_THREADS);

    pthread_barrier_init(&p2_common.barrier, NULL, p2_common.thread_count);

    for (size_t i = 0; i < p2_common.thread_count; ++i) {
        p2_contexts[i].thread_idx = i;
        p2_contexts[i].common     = &p2_common;
    }
}

static string_t read_file() {

    FILE *file = fopen("inputs/day_04.txt", "r");
    assert(file && "File not found");

    string_builder_t file_sb = sb_read_file(file, &file_arena);

    string_t file_str = sb_build(&file_sb);

    fclose(file);
    return file_str;
}
