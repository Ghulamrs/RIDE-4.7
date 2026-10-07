# M10 - source-level debugging of c90 and cpp11 programs on x86_64-windows: analysis

2026-10-07. Analysis and a measured feasibility probe; no compiler source was changed.
Probe files and the raw transcript are in `docs/m10-probe/`. Everything below that says
"measured" was run on the Windows PC (`C:\cxx1\m10`), VS 2022 Community, its clang and link.exe.

## Verdict

**Feasible, and the shape is settled: CodeView written into the assembly text that the GNU
spelling already sends to clang's COFF assembler, linked by Microsoft's link.exe /DEBUG into a
PDB.** A hand-written probe in exactly the frame shape cxx1 emits (`push rbp; mov rbp,rsp; sub;
locals at -k(%rbp)`) with `.cv_*` line directives and hand-written `.debug$S` records - S_GPROC32,
S_REGREL32 off RBP, S_GDATA32, S_END, no `.debug$T` at all (built-in `T_INT4` only) - assembled
with clang, linked with link.exe /DEBUG, and **Microsoft's symbol engine stopped at source line 4,
gave the call stack with file and line, and printed every local with value, type and address**:

```
module m10probe: SymType 3 (3=PDB ...), lines 1, globals 1, types 1
line C:\cxx1\m10\m10probe.c:4 at 00007FF74B6E8AA7
stopped at 00007FF74B6E8AA7
call stack:
  #0 add                      C:\cxx1\m10\m10probe.c:4
  #1 main                     C:\cxx1\m10\m10probe.c:9
  #2 __scrt_common_main_seh   D:\a\_work\1\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288
locals and parameters:
  local  a    = 20     int32        @ 0000006BEF4FFECC  (flags 0x90 reg 334)
  local  b    = 1      int32        @ 0000006BEF4FFEC8  (flags 0x90 reg 334)
  local  s    = 21     int32        @ 0000006BEF4FFEC4  (flags 0x90 reg 334)
  global g    = 7      int32        @ 00007FF74B7A0000  (flags 0x2000000 reg 0)
exit 0
```

That is every column of RIDE's Locals grid (Name | Value | Type | Address) and its Call Stack
grid (Function | File | Line), for a program none of whose debug information came from a
Microsoft or LLVM compiler.

### The one thing the probe could not do: cdb is not installed on the box

`C:\Program Files (x86)\Windows Kits\10\Debuggers\x64` holds only `dbgcore.dll`, `dbghelp.dll`,
`srcsrv.dll`, `symsrv.dll` - no `cdb.exe`, no `dbgeng.dll`; no WinDbg app either. VS's own
`lldb.exe` (VC\Tools\Llvm\x64\bin) exits 0 printing nothing, even for `--version` (a missing
Python DLL, not investigated). So the stand-in is `m10-probe/dbgprobe.c`: 84 lines on the Win32
debug API (CreateProcess DEBUG_ONLY_THIS_PROCESS, an int3 at the line's address) and
**dbghelp.dll** - SymGetLineFromName64, StackWalk64, SymSetContext + SymEnumSymbols,
SymGetTypeInfo. dbghelp is the symbol engine dbgeng (cdb's engine) uses to read a PDB, so what it
reads cdb reads; but **"cdb printed the local" is not yet measured, only "Microsoft's PDB reader
did"**. Installing the Windows SDK's "Debugging Tools for Windows" feature on the box closes
that, and it is also what RIDE's existing ToolMsvc debugging needs there - see the decisions.

## What exists today (verified in the sources)

| | x86_64-windows today |
| --- | --- |
| RIDE `emitsDebugInfo` (src/toolchain.cpp:385) | ToolCc1/ToolCxx1: true only on x86_64-linux and arm64-darwin, so a Debug build on Windows passes no `-g` and `dbg_for` answers DebuggerNone |
| RIDE `dbg_program` (src/debugger.cpp:317) | finds `%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\cdb.exe`, else `cdb` on PATH |
| cpp11 `-g` | DWARF via `Dwarf.cpp` (kElfDwarf, kMachODwarf) on the two Itanium targets. On x86_64-windows `emitsLineTable(Syntax::Gnu)` is true (X86_64Windows.cpp:66): the GNU spelling writes `.file`/`.loc` and nothing else - no `.debug_info` (`writesDwarf()` gates the DWARF body); the MASM spelling refuses `-g` by name (Driver.cpp:996). The driver already prefers clang over a masm.exe beside it when `-g` is asked (Driver.cpp:977) |
| c90 (Compiler-Ci) `-g` | the same design and the same refusal (Driver.cpp:782); `-masm=gnu` exists (Driver.cpp:680) and `GnuSpelling` is in its Spelling.h, so c90 already has a GNU spelling on Windows - it does not need one built first |
| our MASM (`~/Developer/Claude/MASM`) | accepts and ignores `/Zi` (src/main.cpp:66) |
| our LINK (`~/Developer/Claude/LINK`) | `/debug is accepted and does nothing yet - no .pdb is written` (src/main.cpp:147) |
| RIDE's install | ships our masm.exe/link.exe, no clang; uses VS's link.exe unless ours is named |

So "`-g` works, that spelling being the only one with a line table" in C++Optimize's CLAUDE.md
means: the GNU spelling carries a **DWARF** line table (`.loc`) into the COFF object. Measurement
C below shows that no Microsoft tool reads it.

## The four questions, measured

**1. What cdb needs.** CodeView: `.debug$S` (symbols, line tables, file checksums, string table)
and `.debug$T` (types) in each object, merged by link.exe /DEBUG into a PDB. Reference built with
`cl /Zi /Od ref.c /link /DEBUG` and read by the same probe (transcript, section A): stop at
`ref.c:4`, stack `add ref.c:4 / main ref.c:8`, locals `a=20 b=1 s=21 int32` with addresses. cl
describes them as S_REGREL32 off RSP (reg 335) and marks a and b as parameters (flags 0xd0 =
0x90 | SYMFLAG_PARAMETER) - which it knows from the function's LF_PROCEDURE/LF_ARGLIST type. The
hand probe used T_NOTYPE for the function, so a and b came back as locals (0x90): **parameters
need the function type record**, the one piece of `.debug$T` the first milestone wants.

**2(a). CodeView from assembly text through clang's assembler - works (measured, above).** The
assembler does the hard parts: `.cv_file`/`.cv_func_id`/`.cv_loc`/`.cv_linetable` build the line
table, `.cv_filechecksums`/`.cv_stringtable` the file tables, and `.secrel32`/`.secidx` the
relocations a symbol record needs. Everything else is `.short`/`.long`/`.asciz` - the same way
`Dwarf.cpp` already writes DWARF as data directives. `llvm-pdbutil dump -symbols` on the linked
PDB shows the records exactly as written (S_GPROC32 `add` code size 34; S_REGREL32 a/b/s at RBP
-4/-8/-12). Two facts learnt: link.exe moves S_GDATA32 to the PDB's global stream (it is found
there by name); and clang also offers `.cv_def_range ... frame_ptr_rel` with S_LOCAL, the modern
form clang itself emits (`ref-clang.s`), but S_REGREL32 is simpler and enough while every local
has one fixed slot - which is true at -O0, i.e. the Debug configuration.

**2(b). ml64 /Zi** - not measured (no time); it describes the *.asm* file's own lines, so it can
only ever point the debugger at the generated assembly, not the C/C++ source. Not useful.

**2(c). Our MASM learning CodeView** - possible (it would need the `.cv_*` equivalents and
`SECREL`/`SECTION` relocations), but it is a second assembler feature for a path that already
works with clang; and our LINK writes no PDB at all, so it would need a PDB writer too
(MSF container, TPI/IPI/DBI streams, the GSI hash tables) - the largest single item anywhere in
this analysis. Not recommended for M10.

**2(d). DWARF in COFF - not read (measured).** `clang -gdwarf` object, `.debug_info`/`.debug_line`
present (`llvm-objdump -h`), linked /DEBUG: dbghelp loads the PDB (SymType 3) and
`SymGetLineFromName64` fails with 1168 (ERROR_NOT_FOUND) - link.exe drops DWARF and Microsoft's
engine sees no lines. (lldb on Windows can read DWARF, but VS's lldb does not run on this box,
and RIDE drives cdb.)

**3. What each part needs** - in M10-ORGANISATION.md. In one line: a `CodeView.cpp` beside each
compiler's `Dwarf.cpp`, fed by the same `DwarfFunction`/`DwarfGlobal`/`Local`/`Type` information,
selected for the x86_64-windows GNU spelling; RIDE turning `emitsDebugInfo` on for that target,
forcing the GNU spelling (clang) and Microsoft link.exe /DEBUG for a Debug build, and starting cdb.

## Options compared

| option | cost | verdict |
| --- | --- | --- |
| CodeView text + clang's assembler + MS link.exe /DEBUG | one emitter per compiler (~500 lines each), RIDE wiring; needs VS's "C++ Clang tools" and link.exe on the user's machine - the same machine that has cdb | **chosen**, measured |
| DWARF + a DWARF-reading Windows debugger (lldb) | none in the compilers; RIDE gains an lldb-on-Windows path; VS's lldb did not run here; RIDE's Debug tab is built round cdb for Windows | rejected |
| our MASM + our LINK writing CodeView and a PDB | assembler directives + a full PDB writer | rejected for M10; a possible later milestone if a clang-free install matters |
| ml64 /Zi | free | useless: maps to the .asm, not the source |

## Decisions only the user can make

1. **Install cdb on the Windows PC** - Windows SDK "Debugging Tools for Windows" (a download, so
   it needs the user's go-ahead). Until then cdb itself cannot be measured and RIDE's existing
   ToolMsvc debugging cannot run on that box either.
2. **The Debug build on Windows requires VS's clang and Microsoft link.exe.** Acceptable (the
   machine that has cdb has VS), or should RIDE ship clang's assembler (llvm-mc, ~30 MB), or
   should our MASM/LINK eventually learn CodeView/PDB (large)?
