/* M10 stand-in for cdb (not installed on the box): a 150-line debugger on the
   Win32 debug API and dbghelp.dll - Microsoft's symbol engine, the one cdb's
   dbgeng loads. Usage: dbgprobe prog.exe source.c line
   Stops at source:line, prints the call stack and every local/param with
   value, type and address, as RIDE's Locals and Call Stack grids want. */
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#pragma comment(lib, "dbghelp.lib")

static HANDLE proc; static CONTEXT ctx;

static void typeName(ULONG64 base, ULONG ti, char *out) {
    DWORD tag = 0, bt = 0; ULONG64 len = 0; WCHAR *nm = 0;
    SymGetTypeInfo(proc, base, ti, TI_GET_SYMTAG, &tag);
    SymGetTypeInfo(proc, base, ti, TI_GET_LENGTH, &len);
    if (SymGetTypeInfo(proc, base, ti, TI_GET_SYMNAME, &nm) && nm) {
        sprintf(out, "%ls", nm); LocalFree(nm); return; }
    if (tag == 16 /* SymTagBaseType */) { SymGetTypeInfo(proc, base, ti, TI_GET_BASETYPE, &bt);
        sprintf(out, "%s%u", bt == 6 || bt == 13 ? "int" : bt == 7 || bt == 14 ? "unsigned" : bt == 8 ? "float" : "base", (unsigned)(len * 8)); return; }
    if (tag == 14 /* SymTagPointerType */) { DWORD inner; SymGetTypeInfo(proc, base, ti, TI_GET_TYPEID, &inner);
        typeName(base, inner, out); strcat(out, " *"); return; }
    sprintf(out, "tag%u/%u", (unsigned)tag, (unsigned)len);
}

static BOOL CALLBACK onSym(PSYMBOL_INFO s, ULONG size, PVOID user) {
    ULONG64 addr = s->Address; char ty[256]; int v = 0; const char *kind = (const char *)user;
    if (s->Flags & SYMFLAG_REGREL) addr = (s->Register == 334 ? ctx.Rbp : ctx.Rsp) + (LONG64)(LONG)s->Address;
    if (s->Flags & SYMFLAG_FRAMEREL) addr = ctx.Rbp + (LONG64)(LONG)s->Address;
    ReadProcessMemory(proc, (void *)addr, &v, 4, 0);
    typeName(s->ModBase, s->TypeIndex, ty);
    printf("  %-6s %-4s = %-6d %-12s @ %p  (flags 0x%x reg %u)\n", kind, s->Name, v, ty, (void *)addr, (unsigned)s->Flags, (unsigned)s->Register);
    (void)size; return TRUE;
}

int main(int argc, char **argv) {
    STARTUPINFOA si = {sizeof si}; PROCESS_INFORMATION pi; DEBUG_EVENT ev;
    ULONG64 bp = 0; BYTE saved = 0, cc = 0xCC; int done = 0;
    if (argc < 4) return 2;
    if (!CreateProcessA(argv[1], 0, 0, 0, FALSE, DEBUG_ONLY_THIS_PROCESS, 0, 0, &si, &pi)) { printf("CreateProcess %lu\n", GetLastError()); return 1; }
    proc = pi.hProcess;
    while (!done && WaitForDebugEvent(&ev, INFINITE)) {
        DWORD cont = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            IMAGEHLP_LINE64 ln = {sizeof ln}; LONG disp; ULONG64 base;
            SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEBUG);
            SymInitialize(proc, 0, FALSE);
            base = SymLoadModuleEx(proc, ev.u.CreateProcessInfo.hFile, argv[1], 0, (ULONG64)ev.u.CreateProcessInfo.lpBaseOfImage, 0, 0, 0);
            { IMAGEHLP_MODULE64 m = {sizeof m}; SymGetModuleInfo64(proc, base, &m);
              printf("module %s: SymType %d (3=PDB 5=deferred 0=none), lines %d, globals %d, types %d\n", m.ModuleName, m.SymType, m.LineNumbers, m.GlobalSymbols, m.TypeInfo); }
            if (SymGetLineFromName64(proc, 0, argv[2], atoi(argv[3]), &disp, &ln)) {
                bp = ln.Address; printf("line %s:%s at %p\n", argv[2], argv[3], (void *)bp);
                ReadProcessMemory(proc, (void *)bp, &saved, 1, 0); WriteProcessMemory(proc, (void *)bp, &cc, 1, 0);
            } else printf("no address for %s:%s (error %lu)\n", argv[2], argv[3], GetLastError());
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            EXCEPTION_RECORD *er = &ev.u.Exception.ExceptionRecord;
            if (er->ExceptionCode == EXCEPTION_BREAKPOINT && (ULONG64)er->ExceptionAddress == bp) {
                HANDLE th = OpenThread(THREAD_ALL_ACCESS, FALSE, ev.dwThreadId);
                IMAGEHLP_STACK_FRAME sf = {0}; STACKFRAME64 fr = {0}; CONTEXT wc; int depth;
                ctx.ContextFlags = CONTEXT_FULL; GetThreadContext(th, &ctx); ctx.Rip = bp;
                WriteProcessMemory(proc, (void *)bp, &saved, 1, 0); SetThreadContext(th, &ctx);
                printf("stopped at %p\ncall stack:\n", (void *)ctx.Rip);
                wc = ctx; fr.AddrPC.Offset = wc.Rip; fr.AddrFrame.Offset = wc.Rbp; fr.AddrStack.Offset = wc.Rsp;
                fr.AddrPC.Mode = fr.AddrFrame.Mode = fr.AddrStack.Mode = AddrModeFlat;
                for (depth = 0; depth < 6 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, th, &fr, &wc, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0); ++depth) {
                    char b[sizeof(SYMBOL_INFO) + 256]; PSYMBOL_INFO s = (PSYMBOL_INFO)b; DWORD64 d = 0; DWORD dl = 0; IMAGEHLP_LINE64 l = {sizeof l};
                    s->SizeOfStruct = sizeof(SYMBOL_INFO); s->MaxNameLen = 255;
                    if (!SymFromAddr(proc, fr.AddrPC.Offset, &d, s)) strcpy(s->Name, "?");
                    if (!SymGetLineFromAddr64(proc, fr.AddrPC.Offset, &dl, &l)) { l.FileName = "?"; l.LineNumber = 0; }
                    printf("  #%d %-24s %s:%lu\n", depth, s->Name, l.FileName, l.LineNumber);
                }
                sf.InstructionOffset = ctx.Rip; sf.FrameOffset = ctx.Rbp; sf.StackOffset = ctx.Rsp;
                SymSetContext(proc, &sf, 0);
                printf("locals and parameters:\n");
                SymEnumSymbols(proc, 0, 0, onSym, "local");
                { char b[sizeof(SYMBOL_INFO) + 256]; PSYMBOL_INFO s = (PSYMBOL_INFO)b; s->SizeOfStruct = sizeof(SYMBOL_INFO); s->MaxNameLen = 255;
                  if (SymFromName(proc, "g", s)) onSym(s, 0, "global"); else printf("  no global g\n"); }
                CloseHandle(th);
            } else if (!ev.u.Exception.dwFirstChance || er->ExceptionCode != EXCEPTION_BREAKPOINT) cont = DBG_EXCEPTION_NOT_HANDLED;
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) { printf("exit %lu\n", ev.u.ExitProcess.dwExitCode); done = 1; }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, cont);
    }
    return 0;
}
