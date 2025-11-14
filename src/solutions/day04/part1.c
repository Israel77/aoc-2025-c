#include "prelude.h"
#define PART_1_IMPL

#define P1_THREADS 1

/* Shared data between threads */
struct p1_data {
    bingo_t bingo;
    string_array_t lines;
    pthread_mutex_t arena_mtx;

    pthread_mutex_t winner_mtx;
    size_t winner_idx;
    atomic_uint_fast8_t winner_round;
};

static p1_data p1;

/* Functions for part 1 */
static inline void p1_split_lines(struct part_context *ctx);
static inline void check_own_boards(struct part_context *ctx);
static inline uint64_t build_answer();

void *p1_solve(void *arg) {

    struct part_context *ctx = arg;

    /* IO and synchronization */
    size_t thread_count = ctx->common->thread_count;
    size_t thread_idx   = ctx->thread_idx;

    if (thread_idx == 0) {

        p1_split_lines(ctx);

        pthread_mutex_init(&p1.arena_mtx, NULL);
        p1.bingo.boards.array_info.allocator = &arena_allocator;
        p1.bingo.boards.array_info.alloc_ctx = ctx->common->arena;
        p1.bingo.boards.array_info.item_size = sizeof (*p1.bingo.boards.items);

        pthread_mutex_init(&p1.winner_mtx, NULL);
        p1.winner_round = UINT8_MAX;
        p1.winner_idx = SIZE_MAX;
    }
    sync(ctx);

    p1_setup(ctx);

    sync(ctx);

    /* Check the boards for this thread */
    check_own_boards(ctx);

    sync(ctx);

    if (thread_idx == thread_count - 1) {
        uint64_t answer     = build_answer();

        string_builder_t sb = sb_from_u64(answer, &arena_allocator, ctx->common->arena);
        ctx->common->output = sb_build(&sb);
    }

    pthread_mutex_destroy(&p1.arena_mtx);
    pthread_mutex_destroy(&p1.winner_mtx);

    return NULL;
}

static inline uint64_t build_answer() {

    uint64_t unmarked_total = 0;

    uint8_t winner_round = atomic_load(&p1.winner_round);
    size_t winner_idx = p1.winner_idx;


    for (size_t row = 0; row < BINGO_BOARD_ROWS; ++row) {
        for (size_t col = 0; col < BINGO_BOARD_COLS; ++col) {

            board_t *board = &p1.bingo.boards.items[winner_idx];

            if (!(*board)[row][col].marked) {
                unmarked_total += (*board)[row][col].value;
            }
        
        }
    }
    
    return p1.bingo.results[winner_round] * unmarked_total;
}

static inline void p1_split_lines(struct part_context *ctx) {

    pthread_mutex_lock(&p1.arena_mtx);
    p1.lines = string_split_by_char(ctx->common->input, '\n', &arena_allocator, ctx->common->arena);
    pthread_mutex_unlock(&p1.arena_mtx);
}

static inline void p1_parse_results() {
    uint8_t temp_buf[128];
    arena_context_t *temp = arena_from_buf(temp_buf, 128);

    string_builder_t sb = sb_with_capacity(3, &arena_allocator, temp_buf);

    for (size_t ch_idx = 0; ch_idx < p1.lines.items[0].count; ++ch_idx) {

        char ch = p1.lines.items[0].chars[ch_idx];
    
        if (ch != ',') {
            sb_append_char(&sb, ch);

        } else {
            string_t s = sb_build(&sb);
            string_t _;
            p1.bingo.results[p1.bingo.result_count] = parse_u8(&s, &_);
            p1.bingo.result_count++;

            arena_reset(temp);
            sb = sb_with_capacity(3, &arena_allocator, temp_buf);
        }
    }
}

static inline void check_own_boards(struct part_context *ctx) {

    size_t thread_idx   = ctx->thread_idx;
    size_t thread_count = ctx->common->thread_count;

    /* Subtract one from the total because the first line contains the results and add
     * one to the rows because every board is preceded by a new line separator */
    size_t num_boards = (p1.lines.array_info.count - 1) / (BINGO_BOARD_ROWS + 1);
    size_t boards_per_thread = num_boards / thread_count;
    size_t remainders        = num_boards % thread_count;

    const bool take_remainder    = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * boards_per_thread + prev_remainders;
    const size_t end   = start + boards_per_thread + (take_remainder ? 1 : 0);

    bool found_winner = false;

    for (uint8_t r = 0; r < p1.bingo.result_count; ++r) {

        uint8_t current_result = p1.bingo.results[r];

        for (size_t i = start; i < end; ++i) {

            board_t *board = &p1.bingo.boards.items[i];

            mark_result(*board, current_result);
            found_winner = is_winner(*board);

            if (found_winner) {
                /* Critical section */
                pthread_mutex_lock(&p1.winner_mtx);

                bool should_write = atomic_load_explicit(&p1.winner_round, memory_order_acquire) > r;

                if (should_write) {
                    p1.winner_idx = i;
                    atomic_store_explicit(&p1.winner_round, r, memory_order_release);
                }

                pthread_mutex_unlock(&p1.winner_mtx);
                return;
            }
        }

        /* Checks if another thread has found a winner in an earlier round */
        if (atomic_load_explicit(&p1.winner_round, memory_order_acquire) <= r) {
            break;
        }
    }
}

static inline void p1_setup(struct part_context *ctx) {

    size_t thread_idx   = ctx->thread_idx;
    size_t thread_count = ctx->common->thread_count;

    size_t num_boards = (p1.lines.array_info.count - 1) / (BINGO_BOARD_ROWS + 1);
    size_t boards_per_thread = num_boards / thread_count;
    size_t remainders        = num_boards % thread_count;

    const bool take_remainder    = thread_idx < remainders;
    const size_t prev_remainders = take_remainder ? thread_idx : remainders;

    const size_t start = thread_idx * boards_per_thread + prev_remainders;
    const size_t end   = start + boards_per_thread + (take_remainder ? 1 : 0);

    /* Parse the sequence of numbers */
    if (thread_idx == thread_count - 1) {
        p1_parse_results();
        p1.bingo.boards.items = da_reserve(p1.bingo.boards.items, &p1.bingo.boards.array_info, num_boards);
    }

    sync(ctx);

    for (size_t board_idx = start; board_idx < end; board_idx++) {

        uint8_t temp_buf[1024];
        arena_context_t *temp = arena_from_buf(temp_buf, 1024);

        /* +1 to account for the empty line separator */
        size_t line_idx = board_idx * (BINGO_BOARD_ROWS + 1) + 1; 
        string_t line   = p1.lines.items[line_idx];

        /* Skip the empty line */
        while (line.count == 0) {
            line = p1.lines.items[++line_idx];
        }

        board_t board = {0};

        for (size_t row = 0; row < BINGO_BOARD_ROWS; ++row) {

            line = p1.lines.items[line_idx + row];
            string_array_t nums_str = string_split_by_char(&line, ' ', &arena_allocator, temp);

            size_t col = 0;
            for (size_t i = 0; i < nums_str.array_info.count; ++i) {
                if (nums_str.items[i].count != 0) {
                    string_t _;
                    uint64_t value  = parse_u64(&nums_str.items[i], &_);
                    board[row][col].value = value;
                    ++col;
                }
            }

        }

        pthread_mutex_lock(&p1.arena_mtx);
        p1.bingo.boards.items = da_append(p1.bingo.boards.items, &p1.bingo.boards.array_info, &board);
        pthread_mutex_unlock(&p1.arena_mtx);

        arena_reset(temp);
    }
}
