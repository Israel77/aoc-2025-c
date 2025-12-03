# Advent of Code 2025

My solutions to AoC 2025. All solutions are written in standard C11, no external libraries were used, but there is no portability guarantees as they were only tested on Linux x86-64. OS-specific headers and architecture intrinsics might be used.

My main goal for this year is to run all solutions under 10ms on a Ryzen 9 7950X. The setup for each solution is designed to enable multithreading and is based on the ideas from this article by Ryan Fleury: [Multi-Core By Default](https://www.rfleury.com/p/multi-core-by-default), which was presented to me by (LucasGdosR)[https://github.com/LucasGdosR/]. However, not all solutions will actually run multithreaded by default.

From some quick benchmarks that I did on my computer, there's an overhead of ~20-30 μs for each additional thread created and joined. There are diminishing returns when running short-lived tasks such as the AoC solutions with more threads and, in particular, going above 16 threads usually makes the solution worse on the test machine (as it has 16 physical cores).

For simple problems, multithreading might also make the solution slower, because it prevents the compiler from inlining the function and doing better optimizations, which tipically makes it at least 3x slower (sometimes orders of magnitude slower!). Therefore, in my setup, if the number of threads is set to 1, it will call the function directly rather than spawning and joining a single thread.

## Compiling and running

This project uses [`nob`](https://github.com/tsoding/nob.h) as it "build system". You will only need a C compiler (both gcc and clang should work).
1 - Compile nob.c
`cc -o nob.c nob`

2 - Run nob
`./nob`

This should compile the entire project. The programs for each day are stored in build/solutions/day_XX/main. By default all programs will run after compilation, you can change this behaviour by tweaking build/config.h (generated at the first time you execute nob).

## Current status

| Day | Part 1 | Part 2 | Multithreaded |
|-----|--------|--------|---------------|
| 01  | ✅     | ✅     | No            |
| 02  | ✅     | ✅     | Yes           |
| 03  | ✅     | ✅     | No*           |
| 04  | ❌     | ❌     |               |
| 05  | ❌     | ❌     |               |
| 06  | ❌     | ❌     |               |
| 07  | ❌     | ❌     |               |
| 08  | ❌     | ❌     |               |
| 09  | ❌     | ❌     |               |
| 10  | ❌     | ❌     |               |
| 11  | ❌     | ❌     |               |
| 12  | ❌     | ❌     |               |

* Able to run in parallel, but no performance gain observed.
