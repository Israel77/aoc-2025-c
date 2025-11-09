#include "prelude.h"
#include "part1.c"
#include "part2.c"
#include "tests.c"

#define FILE_CAP 100 * 8 * 1024
#define MAX_THREADS 32
static unsigned char file_buffer[FILE_CAP];
static arena_context_t *file_arena;
static arena_context_t solution_arena;

static string_t read_file();
static inline void setup();

static pthread_t threads[MAX_THREADS];

static struct part_context_common p1_common;
static struct part_context p1_contexts[P1_THREADS];

static struct part_context_common p2_common;
static struct part_context p2_contexts[P2_THREADS];

string_t input;

int main(void) {

    volatile uint64_t clock_start;
    volatile uint64_t clock_end;

    setup();

    printf("\n==== Day 4 ====\n");

#ifdef TEST_IMPL
    run_tests();
    arena_reset(&solution_arena);
#endif

#ifdef PART_1_IMPL
    printf("Solution to part 1:\n");
    clock_start = now_ns();

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

    clock_end = now_ns();
    string_println(&p1_contexts[0].common->output);
    printf("Took: %'ld ns\n", clock_end - clock_start);

    arena_reset(&solution_arena);
#endif /* ifdef PART1_IMPL */

#ifdef PART_2_IMPL
    printf("Solution to part 2:\n");
    clock_start = now_ns();

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

    clock_end = now_ns();
    string_println(&p2_contexts[0].common->output);
    printf("Took: %'ld ns\n", clock_end - clock_start);
#endif

    arena_destroy(&solution_arena);

    return 0;
}

static inline void setup() {

    setlocale(LC_NUMERIC, "pt_BR.UTF-8");

    /* Setup arenas */
    file_arena = arena_from_buf(file_buffer, FILE_CAP);
    solution_arena = arena_init(4096, ARENA_FAST_ALLOC | ARENA_GROWABLE | ARENA_MALLOC_BACKEND, NULL, NULL);

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

    string_builder_t file_sb = sb_read_file(file, &arena_allocator, file_arena);

    string_t file_str = sb_build(&file_sb);

    fclose(file);
    return file_str;
}
