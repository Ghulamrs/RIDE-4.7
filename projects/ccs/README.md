# Sample CCS 7.4 projects

Five Code Composer Studio 7.4 projects for the TMS320C6747, made by CCS
itself and shipped as CCS left them: `.project`, `.ccsproject`, `.cproject`,
the linker command file and the sources. RIDE opens a CCS project folder as
it is and writes nothing into it; they are here to try that on.

| folder     | what it is                                                        |
|------------|-------------------------------------------------------------------|
| `K6747c`   | C: `main.c`, `inc/config.h`, and `util.c` linked from `../src`     |
| `K6747cpp` | C++: `main.cpp` (a class and `printf`), built with exceptions     |
| `P7misc`   | C: `lib/util.c` in a subfolder with its own options, and `extra.c` excluded from the build |
| `Sample`   | C++: `Math.cpp` over class templates for a vector, a matrix and a quaternion (`Vector.h`, `Matrix.h`, `Quat.h`), printed with `std::cout` |
| `SampleExt`| C++: `Sample` extended - two 3x3 matrices multiplied, a vector's cross, dot and component products, a matrix times a vector, and a rotation about Z; the same three headers grown with the operators it uses |

## Opening one

The CCS switch in `settings.json` ships on, `"ccs": { "enabled": true }`, and
these five are copied into `Documents/RIDE/projects/ccs` - where
`Project > Open...` starts. On Windows choose a project's `.project`; on macOS
its folder. `"root"` may name a CCS install (`C:/ti/ccsv7`); on a machine
without CCS it is left empty. On the console, give the folder:

    ride K6747c --run
    ride K6747cpp --config release --run

RIDE takes the project's device, sources, include paths, defines and
configurations (Debug and Release) and builds with its own cpp11 or c90,
asm6x and lnk6x for tms6747, running the program on vm6747. Every option of
the project it does not honour is named in the console on the first line of
the build, with RIDE's default used instead.

## What each prints

| project    | Debug                               | Release                             |
|------------|-------------------------------------|-------------------------------------|
| `K6747c`   | `K6747c: level 0, twice(21) = 42`   | `K6747c: level 2, twice(21) = 42`   |
| `K6747cpp` | `K6747cpp: level 0, counter 42`     | `K6747cpp: level 3, counter 42`     |
| `P7misc`   | `K6747c: level 0, twice(21) = 42`   | `K6747c: level 0, twice(21) = 42`   |
| `Sample`   | `Hello Math!`, then a unit quaternion `1 0 0 0`, a zero vector and a zero 3x3 matrix | the same |
| `SampleExt`| `Sample`'s lines, then `A`, `B`, `C = A * B`, `a`, `b`, `a * b`, `a . b = 32`, `a ^ b`, `A * a`, `Rz(0.5)` and `Rz * a` | the same |

LEVEL comes from each configuration's defines in `.cproject`. P7misc says
`K6747c` because it was made from that project's sources; its `extra.c`
would not link if it were built, so a correct run is itself the check that
the exclusion was read. Release builds also report the TI objects made: a
`.out` for the real board needs TI's linker, named under Tools.

Sample prints through `std::cout`, and RIDE's C++ library writes it the way
`printf` does. Built by CCS 7.4 itself and run on TI's C6747 simulator, the
same program prints nothing: TI's own `<iostream>` emits no console output
there, though its `printf` does. Its values are CCS 7.4's all the same,
checked through `printf`.

SampleExt is a copy of Sample's project files with its own sources, and
its `.project` names it `SampleExt` rather than `Sample`, so the two can sit
in one CCS workspace.

The one change from the projects CCS wrote: K6747c's linked `util.c` was
recorded with an absolute path on the machine that made it, and is written
here as `PARENT-1-PROJECT_LOC/src/util.c`, the relative form CCS itself uses.
The unchanged originals are in `docs/ccs-reference`.
