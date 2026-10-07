# M10 - source-level debugging on x86_64-windows: organisation

2026-10-07. The design that follows from M10-ANALYSIS.md. Nothing here is built yet.

## The pipeline for a Debug build of a c90/cpp11 program on Windows

```
c90/cpp11 -g -masm=gnu -arch x86_64-windows   ->  .s with .cv_* lines + .debug$S/.debug$T data
clang -target x86_64-pc-windows-msvc -c       ->  COFF .obj (the driver already finds VS's clang)
Microsoft link.exe /DEBUG                     ->  .exe + .pdb
cdb (RIDE's DebuggerCdb, as for ToolMsvc)     ->  Locals / Watch / Call Stack grids
```

## Files, per repository

**C++Optimize (cpp11)** and **VM6747/Compiler-Ci (c90)** - the same design written twice, as each
compiler's backend is its own (the way Dwarf.cpp already is):

- `src/backend/CodeView.h/.cpp` (new), beside `Dwarf.cpp`. One entry point,
  `writeCodeView(std::string &out, const Target &, file, compDir, fns, globals)`, taking the
  **same** `DwarfFunction` / `DwarfGlobal` vectors the DWARF writer takes - so `X86_64Linux.cpp`
  collects once and calls whichever writer the target asks for. Rename the two structs
  `DebugFunction`/`DebugGlobal` when the second reader arrives (a move, golden 0 changed).
- `X86_64Linux.cpp` (the shared x86 code generator): where it calls `writeDwarf(... kElfDwarf ...)`
  call `writeCodeView` for x86_64-windows instead; give each function a `.cv_func_id n` after its
  label and turn `markLine`'s `.loc file line col` into `.cv_loc n file line col`.
- `Spelling.h/.cpp`: `CoffSpelling::fileEntry` writes `.cv_file`, `location` writes `.cv_loc`
  with the function id - the GNU spelling for ELF/Mach-O is unchanged.
- `X86_64Windows.cpp`: `emitsLineTable(Gnu)` stays true; `writesDwarf()` false is already right.
- Driver: nothing new - `-g` with x86_64-windows already chooses the GNU spelling when clang is
  there and refuses MASM by name.

**RIDE-4.7**:

- `src/toolchain.cpp` `emitsDebugInfo`: add `arch == "x86_64-windows"` for ToolCc1/ToolCxx1.
- `assemblerFlag`/`prepareFor`: in the Debug configuration on x86_64-windows pass `-masm=gnu`
  (never `-masm=masm`, even when RIDE's masm.exe is named) and link with Microsoft's link.exe
  `/DEBUG` (never ours, which writes no PDB) - say so in the build log when that overrides a
  setting the user made.
- `src/debugger.cpp` `dbg_for`: ToolCc1/ToolCxx1 on x86_64-windows -> DebuggerCdb, the same
  command set as ToolMsvc (`bp`, `dv /t /V`, `k`); the message when cdb or clang is missing.
- `debugNote` text for the new case.

**MASM, LINK**: nothing in M10.

## The CodeView subset

`.debug$S` (signature 4), one DEBUG_S_SYMBOLS subsection per function plus the line table
(`.cv_linetable`), `.cv_filechecksums`, `.cv_stringtable`. `.debug$T` (signature 4) for types,
indices from 0x1000, built-ins below.

| need | records |
| --- | --- |
| compile unit | S_OBJNAME, S_COMPILE3 (language C 0x00 / C++ 0x01, machine 0xD0 x64) |
| function | S_GPROC32 / S_LPROC32 (static) with LF_PROCEDURE + LF_ARGLIST type; S_FRAMEPROC; S_END |
| params, locals | S_REGREL32, register RBP (334), offset = the slot's -k; parameters first, in order (dbghelp counts them off LF_ARGLIST - measured: without it they show as locals) |
| blocks | S_BLOCK32 / S_END from `DwarfBlock` |
| globals | S_GDATA32 / S_LDATA32 (`.secrel32`, `.secidx`) |
| static locals | S_LDATA32 inside the proc |
| base types | built-in indices: T_CHAR 0x10, T_UCHAR 0x20, T_SHORT 0x11, T_INT4 0x74, T_UINT4 0x75, T_QUAD 0x13, T_UQUAD 0x23, T_REAL32 0x40, T_REAL64 0x41, T_BOOL08 0x30, T_WCHAR 0x71, T_VOID 3; pointer to built-in = 0x600 + base (T_64P...) |
| derived | LF_POINTER (other pointees), LF_MODIFIER (const), LF_ARRAY, LF_FIELDLIST + LF_MEMBER + LF_STRUCTURE/LF_CLASS/LF_UNION (forward ref + definition), LF_ENUM + LF_ENUMERATE, LF_BITFIELD |
| C++ | LF_MFUNCTION + LF_METHOD/LF_ONEMETHOD (member functions, `this` as the first S_REGREL32 with a `T *const` type), LF_BCLASS for bases, LF_VTSHAPE if cheap; namespaces by qualified names in the records (cl's own way) |

Frame: every cxx1/c90 local at -O0 has one slot `-k(%rbp)` for the whole function, which is why
S_REGREL32 is enough and S_LOCAL + S_DEFRANGE is not needed for M10. Release (-O1/-O2) builds
emit no debug information on Windows in M10.

## Milestones - each shippable and testable in the RIDE window

| | milestone | test | estimate |
| --- | --- | --- | --- |
| W1 | **cpp11: line table + procs.** `.cv_file/.cv_func_id/.cv_loc/.cv_linetable`, S_COMPILE3, S_GPROC32 with T_NOTYPE, S_END; RIDE: emitsDebugInfo, -masm=gnu, MS link /DEBUG, cdb for ToolCxx1 | a case per shape linked and run under cdb by a script on the box (`bp file:line`, `k`, `p` stepping, compare with an `.expected`); RIDE window: breakpoint, F10/F11, Call Stack grid Function/File/Line | 2 agent rounds (~4 h); first needs cdb installed |
| W2 | **cpp11: params/locals/globals with base types and pointers.** LF_PROCEDURE/LF_ARGLIST, S_REGREL32, S_GDATA32/S_LDATA32, S_BLOCK32, built-in types, LF_POINTER/LF_MODIFIER | `dv /t /V` output per case against cl's for the same source (the reference oracle: cl /Zi on the identical .cpp); Locals and Watch grids fill Name/Value/Type/Address | 2 rounds (~4 h) |
| W3 | **c90: W1 + W2** - port CodeView.cpp into Compiler-Ci (its Dwarf.cpp differs from cpp11's by 3 lines today), RIDE for ToolCc1 | the same box script over c90's suite; RIDE window on a C project | 1-2 rounds (~3 h) |
| W4 | **aggregates and C++ classes**: LF_ARRAY, LF_STRUCTURE/UNION/CLASS + field lists, LF_ENUM, bit-fields; member functions, `this`, bases | `dv /t`, `dt` of a struct/array/class local against cl's; expanding a struct row in RIDE's grid | 3 rounds (~6 h); C++ members are most of it |
| W5 | **RIDE wiring finished + window test**: messages when clang/cdb/link.exe are absent, debugNote, the window test (`C:\cxx1\gui` scripts driving installed RIDE.exe) on c90 and cpp11 projects, Mac parity text | the installed RIDE: a c90 and a cpp11 project each stop at a breakpoint and fill all three grids | 1-2 rounds (~3 h) |

Total about 9-11 agent rounds. Order W1 -> W2 -> W3 -> W4 -> W5; W3 may run beside W4.

## Oracles and gates

- **cl /Zi on the same source** is the oracle for what cdb shows (`dv /t /V`, `k`) - the names,
  types and values must agree; addresses differ by layout and are only checked to be the slot.
- `llvm-pdbutil dump -symbols -types` (in VS's Llvm\bin) on the linked PDB, to read what was
  written when cdb disagrees.
- The emit golden: every non-Windows target and the MASM spelling must read 0 changed; the
  x86_64-windows GNU goldens change only under `-g` (the golden is recorded without it - add a
  `-g` golden for Windows in W1).
- `m10-probe/dbgprobe.c` stays as a cdb-free check of a PDB (dbghelp), usable in the suite.
