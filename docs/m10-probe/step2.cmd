@call C:\cxx1\m10\env.cmd
cd /d C:\cxx1\m10
cl /nologo /W3 dbgprobe.c >nul || echo dbgprobe build FAILED
echo ==== A: reference, cl /Zi
dbgprobe ref-cl.exe C:\cxx1\m10\ref.c 4
echo ==== B: hand-written CodeView, clang assembler, Microsoft link /DEBUG
clang -target x86_64-pc-windows-msvc -c m10probe.s -o m10probe.obj || echo ASSEMBLE FAILED
link /nologo /DEBUG m10probe.obj /OUT:m10probe.exe libcmt.lib legacy_stdio_definitions.lib || echo LINK FAILED
llvm-pdbutil dump -symbols -modi=0 m10probe.pdb
dbgprobe m10probe.exe C:\cxx1\m10\m10probe.c 4
echo ==== C: DWARF in COFF, clang -gdwarf, link /DEBUG
clang -target x86_64-pc-windows-msvc -gdwarf -O0 -c ref.c -o ref-dw.obj
link /nologo /DEBUG ref-dw.obj /OUT:ref-dw.exe libcmt.lib >nul
llvm-objdump -h ref-dw.obj | findstr debug
dbgprobe ref-dw.exe C:\cxx1\m10\ref.c 4
