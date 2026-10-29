# Coding conventions

## Commits
- Start every commit message with `[CLAUDE]`, e.g. `[CLAUDE] Fix off-by-one in argchk`.
- Keep the `Co-Authored-By` trailer on commits made by Claude.

## Includes (C/C++)
- Every `#include` gets a trailing comment naming the functions, macros or types it is there for, with no space before the `//`:
  `#include <stdlib.h>//exit(), NULL`
- Exception: `<stdint.h>` and `<stdbool.h>` are left unannotated, since their types are obvious.
- Local headers are annotated like any other include, except `"bitops.h"`. Its `rol()`/`ror()` are treated as local primitives, equivalent to an operator that compiles down to one instruction, so they need no annotation.

## Style
- Indent with tabs. Opening braces go on the same line as the statement.
- Names are lowercase with underscores, short and abbreviated (`chk_dbl`, `rng_init`, `srv_flags`).
- Header include guards are prefixed with the project name, e.g. `NOTCIV_BITOPS_H`.
- Single-statement `if`s may go on one line without braces.
- Use `static inline` for small helpers and `static` for private file-scope state.
- Prefer `goto err` for error cleanup in C. Report failures with `perror()` and then `exit(EXIT_FAILURE)`.
- Plain `char` is for text only. Everything else uses the stdint types (`uint8_t`, `uint64_t`, etc.), including small values and counts.
- Plain `int` and `unsigned` appear only where the language or an API dictates them (`main`, signal handler prototypes, loop indexes). `sig_atomic_t`, `rlim_t` and similar types appear only where an API requires them.
- Integer literals use `UINT64_C()` and friends, not `UL`/`L` suffixes. Print stdint types with the `PRI*` macros from `<inttypes.h>`, not by casting to `unsigned long`. Compare syscall results with plain `-1`.
- Small flag sets are `uint8_t` bitfields. Document the bit layout in a comment above the variable and mark unused bits "rsvd".

## Comments
- Start each file with a one-line `//` summary, followed by the license notice.
- Comments are short and casual. Use them to explain design decisions, not to restate the code.
- Keep attribution and licensing notes for any code derived from elsewhere.

## Build
- Build with `make`. The Makefile passes no `-std`, so gcc uses its default (`gnu17`), which keeps POSIX interfaces like `sigaction()` visible. Don't add `-std=c17`.
- The Makefile's strict warning set (`-pedantic -Wall -Wextra -Wshadow -Wswitch-enum -Wswitch-default -Wfloat-equal -Wcast-align=strict`, etc.) applies to all code. It should build warning-free.
- The Makefile passes `-fno-math-errno`, so math functions like `sqrt()` don't set `errno` and can compile to a single instruction. Don't check `errno` after math calls. Don't add `-ffast-math`: it lets the compiler delete `isnan()` checks and reorder floating-point sums.
- The Makefile passes `-funsigned-char`, so `char` is unsigned on every target, as it already is on arm64 Linux. Never rely on its signedness.
