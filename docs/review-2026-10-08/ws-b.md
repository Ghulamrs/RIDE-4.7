# WS-B - RIDE documentation and the architecture page

Branch `review/b-docs` off RIDE main `644d6d7`. Workstream B of
`C++Optimize/docs/REVIEW-PLAN-2026-10-08.md`, answering the review
`CppOptimize-critical-review_1.md` sections 7.1-7.3 and the DOCUMENT rows of
sections 4, 5 and 6 that name B. Docs only: nothing under `src/` changed, and
`python3 tools/seal check` reads `seal ride 5.1: 80 files, 0 differ`.

## 1. Section 7.3, as each line read on 2026-10-08 before this branch

Quoted from `644d6d7`; the line numbers are the review's, re-read, with the
line as the file holds it today where the review's number has drifted.

- `help/07-building.md:49`: "`Ctrl-T` moves to the next target; the Target menu names all three."
- `help/07-building.md:53-56`: a three-row table - "| `x86_64-windows` | MASM, assembled by `ml64` |", "| `x86_64-linux` | GNU assembly |", "| `arm64-darwin` | this Mac's own |" - under "## Targets".
- `help/07-building.md:57-58`: "**Only the host's own target reaches a program.** c90, cpp11 and shalimar generate for all three".
- `help/README.md:33-34`: "| [C](c.md) | c90, three targets, DWARF on two of them |" and "| [C++](cpp.md) | cpp11, three targets, and the host's compiler by name |".
- `help/c.md:11`: "| targets | `x86_64-windows`, `x86_64-linux`, `arm64-darwin` |"; `help/c.md:22`: "## The three targets, and what reaches a program"; `help/c.md:28`: "On Windows c90 writes **MASM**, assembled by `ml64` and linked by `link`."
- `help/01-what-it-is.md:92`: "machine alike, with the same three targets and DWARF on the same two of them."
- `help/08-debugging.md:66`: "same behaviour on all four targets - including `x86_64-windows`" (the one page that had four).
- `help/cpp.md:14`: "| targets | `x86_64-windows`, `x86_64-linux`, `arm64-darwin` - the same three as c90 |"; `help/cpp.md:69-72`: "**cpp11 writes DWARF for `x86_64-linux` and `arm64-darwin`** ... On `x86_64-windows` it writes MASM and no line table, exactly as c90 does, and the Debug tab says so."; `help/cpp.md:82-83`: "So on Windows, C++ under cpp11 is where C under c90 is - no line table - and C++ under cl carries everything."
- `help/08-debugging.md:43`: "| c90 or cpp11 on `x86_64-windows` | **no** - MASM carries no line table |".
- `help/01-what-it-is.md:116-117` (the review's 110-111): "**It has no optimiser of its own.** What optimisation you get is whatever the compiler you chose does."; `help/07-building.md:79-82` meanwhile: "cpp11 optimises every target: on tms6747 `-O1` keeps locals in registers, schedules execute packets and fills delay slots, and `-O2` adds software pipelining, loop-invariant hoisting and inlining."; and `help/01-what-it-is.md` names none of tms6747, vm6747, sim6747, asm6x, lnk6x or RTS6x.
- `docs/ride-5.0-architecture.html:6` "<title>RIDE 5.0 Architecture</title>"; `:97` "RIDE 5.0 - compilers, tools and targets"; `:115-116` "RIDE 5.0 ... seal ride-5.0.dat"; `:281-292` the seals table reading `ride-5.0.dat 35BD9D0E`, `c90-1.1.dat B1317483`, `cxx1-1.5.dat D6B8B482`, `lnk6x-1.0.dat 1EABC53A`, `sim6747-1.1.dat DDB5751B`, `rts6x-1.0.dat 5AFCD02F`; `:297` "master seal 1732D0A4 is stale until the next release". `MASTER.SEAL` the same day: `ride-5.1.dat 0A46C214`, `c90-1.2.dat 8D82BAA5`, `cxx1-1.6.dat 2891841F`, `lnk6x-1.1.dat 1D45D6CE`, `sim6747-1.2.dat 2D1A3C81`, `rts6x-1.2.dat 040CE785`, master `DC1EF907`. `src/about.cpp:28` returns "5.1". No `ride-5.0.dat` exists in the tree (`ride-4.5`, `4.51`, `4.7`, `5.1`).
- `help/06-the-project.md:125-127`: "installed in `bin/lib/rts6x-tms6747` in two builds - `rts6x.lib` at -O2 for a Release build and `rts6xd.lib` at -O0 with `_DEBUG` for a Debug one, with `printf6x.lib` and `printf6xd.lib`" (T6: the page is kept; S2 is G's and makes it true).
- T7 and T8 are cpp11's documents and WS-A's.

Also found stale while re-reading, outside the review's list: `help/manual/`
(the fifteen-part Complete Manual) says "no line table in the MASM spelling",
"ml64" as the IDE's assembler and "vm6747 assembles and runs the .s" in Parts
I, II, V, VI, VII, XI and XV; `README.md:221-225, 274-275, 335-350, 436-441,
704-706, 742-744` carry the pre-M10 Windows debugging story and "three
targets"; `src/help.cpp:11,21,22` (the Help > Contents one-liners) say "three
targets" and is `src/`, which this workstream does not touch.
