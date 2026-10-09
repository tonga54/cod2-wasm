/* glibc exposes the ucontext register indexes under GNU feature definitions. */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#    define _GNU_SOURCE 1
#endif
#include "crash_handler.h"

#ifndef COD2_GIT_HASH
#    define COD2_GIT_HASH "unknown"
#endif
#define CR_BUILD_STAMP __DATE__ " " __TIME__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#    include <io.h>
#    include <fcntl.h>
#    include <signal.h>
#else
#    include <unistd.h>
#endif

static char cr_appName[64] = "CoD2";
static char cr_version[64] = "?";
static char cr_buildDate[64] = "?";
static char cr_cmdline[2048] = "";
static char cr_exePath[1024] = "";

#define CR_MAX_FIELDS 24
static struct {
    char name[48];
    char value[256];
} cr_fields[CR_MAX_FIELDS];
static int cr_fieldCount = 0;
static volatile int cr_inHandler = 0;

void Sys_CrashSetField(const char *name, const char *value)
{
    int i;
    if (!name || !name[0])
        return;
    if (!value)
        value = "";
    for (i = 0; i < cr_fieldCount; i++) {
        if (strncmp(cr_fields[i].name, name, sizeof(cr_fields[i].name)) == 0) {
            snprintf(cr_fields[i].value, sizeof(cr_fields[i].value), "%s", value);
            return;
        }
    }
    if (cr_fieldCount >= CR_MAX_FIELDS)
        return;
    snprintf(cr_fields[cr_fieldCount].name, sizeof(cr_fields[cr_fieldCount].name), "%s", name);
    snprintf(cr_fields[cr_fieldCount].value, sizeof(cr_fields[cr_fieldCount].value), "%s", value);
    cr_fieldCount++;
}

static void cr_emit(int fd, const char *fmt, ...)
{
    char buf[2048];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n > (int)sizeof(buf))
        n = (int)sizeof(buf);
    if (fd >= 0) {
        int _w = (int)write(fd, buf, n);
        (void)_w;
    }
    {
        int _w = (int)write(2 , buf, n);
        (void)_w;
    }
}

static void cr_emit_common(int fd, const char *crashDesc)
{
    time_t now = time(NULL);
    char when[64];
    struct tm *tmv = localtime(&now);
    int i;
    when[0] = '\0';
    if (tmv)
        strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", tmv);

    cr_emit(fd, "================ %s CRASH REPORT ================\n\n", cr_appName);
    cr_emit(fd, "crash      : %s\n", crashDesc);
    cr_emit(fd, "time       : %s\n", when);
    cr_emit(fd, "version    : %s (game data %s)\n", cr_version, cr_buildDate);
    cr_emit(fd, "build      : %s  (git %s)\n", CR_BUILD_STAMP, COD2_GIT_HASH);
    if (cr_exePath[0])
        cr_emit(fd, "executable : %s\n", cr_exePath);
    cr_emit(fd, "command    : %s\n", cr_cmdline[0] ? cr_cmdline : "(none)");
    for (i = 0; i < cr_fieldCount; i++)
        cr_emit(fd, "%-10s : %s\n", cr_fields[i].name, cr_fields[i].value);
    cr_emit(fd, "\n");
}

#ifdef _WIN32

#    include <windows.h>
#    include <dbghelp.h>

static HWND cr_win_edit = NULL;

static LRESULT CALLBACK cr_win_proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_SIZE && cr_win_edit) {
        MoveWindow(cr_win_edit, 0, 0, LOWORD(l), HIWORD(l), TRUE);
        return 0;
    }
    if (m == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

static void cr_win_popup(const char *path, const char *crashDesc)
{
    static char text[256 * 1024];
    static char title[300];
    FILE *f;
    size_t rd = 0, j = 0;
    WNDCLASSA wc;
    HWND win;
    HFONT font;
    MSG msg;
    static char raw[200 * 1024];
    size_t i;

    f = fopen(path, "rb");
    if (f) {
        rd = fread(raw, 1, sizeof(raw) - 1, f);
        fclose(f);
    }
    raw[rd] = '\0';
    for (i = 0; i < rd && j < sizeof(text) - 2; i++) {
        if (raw[i] == '\n' && (i == 0 || raw[i - 1] != '\r'))
            text[j++] = '\r';
        text[j++] = raw[i];
    }
    text[j] = '\0';
    if (rd == 0)
        snprintf(text, sizeof(text), "%s crashed:\r\n%s\r\n(report file %s)", cr_appName, crashDesc, path);
    snprintf(title, sizeof(title), "%s crashed - %s", cr_appName, crashDesc);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = cr_win_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "CoD2CrashWnd";
    RegisterClassA(&wc);

    win = CreateWindowExA(WS_EX_TOPMOST, "CoD2CrashWnd", title,
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
                          NULL, NULL, wc.hInstance, NULL);
    if (!win) {
        MessageBoxA(NULL, text, title, MB_OK | MB_ICONERROR);
        return;
    }

    cr_win_edit = CreateWindowExA(0, "EDIT", NULL,
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                                      ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                  0, 0, 1000, 700, win, NULL, wc.hInstance, NULL);
    font = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       FIXED_PITCH | FF_MODERN, "Consolas");
    if (font)
        SendMessageA(cr_win_edit, WM_SETFONT, (WPARAM)font, TRUE);
    SetWindowTextA(cr_win_edit, text);

    ShowWindow(win, SW_SHOW);
    SetForegroundWindow(win);
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void cr_win_regs(int fd, CONTEXT *c)
{
    if (!c)
        return;
#    if defined(_WIN64)
    cr_emit(fd, "registers:\n");
    cr_emit(fd, "  rip=%016llx rsp=%016llx rbp=%016llx rflags=%08lx\n",
            (unsigned long long)c->Rip, (unsigned long long)c->Rsp, (unsigned long long)c->Rbp, (unsigned long)c->EFlags);
    cr_emit(fd, "  rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n",
            (unsigned long long)c->Rax, (unsigned long long)c->Rbx, (unsigned long long)c->Rcx, (unsigned long long)c->Rdx);
    cr_emit(fd, "  rsi=%016llx rdi=%016llx r8 =%016llx r9 =%016llx\n",
            (unsigned long long)c->Rsi, (unsigned long long)c->Rdi, (unsigned long long)c->R8, (unsigned long long)c->R9);
    {   /* exe base + rip-relative offset, so the crash site is symbolizable from
         * the .map even when StackWalk yields no frames. */
        HMODULE hm = GetModuleHandleA(NULL);
        cr_emit(fd, "  exe_base=%016llx  rip_rel=0x%llx\n",
                (unsigned long long)(uintptr_t)hm,
                (unsigned long long)((uintptr_t)c->Rip - (uintptr_t)hm));
    }
    {   /* Identify the module holding rip (usually a system DLL when StackWalk
         * fails) and the faulting thread, to tell main-thread init from a worker. */
        HMODULE hmod = NULL;
        char modName[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)(uintptr_t)c->Rip, &hmod) && hmod) {
            GetModuleFileNameA(hmod, modName, sizeof(modName));
            cr_emit(fd, "  rip module=%s +0x%llx\n", modName,
                    (unsigned long long)((uintptr_t)c->Rip - (uintptr_t)hmod));
        } else {
            cr_emit(fd, "  rip module=<unknown>\n");
        }
        cr_emit(fd, "  thread=%lu\n", (unsigned long)GetCurrentThreadId());
    }
    {   /* Raw stack words near rsp: lets the call chain be read by eye when the
         * unwinder fails (e.g. fault in a DLL with no frame info). */
        uintptr_t sp = (uintptr_t)c->Rsp;
        int i;
        cr_emit(fd, "  stack@rsp:");
        for (i = 0; i < 24; i++) {
            uintptr_t a = sp + (uintptr_t)i * sizeof(uintptr_t);
            if (IsBadReadPtr((void *)a, sizeof(uintptr_t))) {
                cr_emit(fd, " <unmapped@+0x%x>", (unsigned)(i * (int)sizeof(uintptr_t)));
                break;
            }
            if ((i & 3) == 0)
                cr_emit(fd, "\n   +0x%03x:", (unsigned)(i * (int)sizeof(uintptr_t)));
            cr_emit(fd, " %016llx", (unsigned long long)*(uintptr_t *)a);
        }
        cr_emit(fd, "\n");
    }
#    else
    cr_emit(fd, "registers:\n");
    cr_emit(fd, "  eip=%08lx esp=%08lx ebp=%08lx eflags=%08lx\n",
            (unsigned long)c->Eip, (unsigned long)c->Esp, (unsigned long)c->Ebp, (unsigned long)c->EFlags);
    cr_emit(fd, "  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n",
            (unsigned long)c->Eax, (unsigned long)c->Ebx, (unsigned long)c->Ecx, (unsigned long)c->Edx);
    cr_emit(fd, "  esi=%08lx edi=%08lx\n", (unsigned long)c->Esi, (unsigned long)c->Edi);
#    endif
    cr_emit(fd, "\n");
}

static void cr_win_backtrace(int fd, CONTEXT *ctxIn)
{
    HANDLE proc = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    CONTEXT ctx;
    STACKFRAME64 frame;
    DWORD machine;
    int depth = 0;
    char symbuf[sizeof(SYMBOL_INFO) + 512];
    SYMBOL_INFO *sym = (SYMBOL_INFO *)symbuf;

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    SymInitialize(proc, NULL, TRUE);

    if (ctxIn)
        ctx = *ctxIn;
    else
        RtlCaptureContext(&ctx);

    memset(&frame, 0, sizeof(frame));
#    if defined(_WIN64)
    machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrStack.Offset = ctx.Rsp;
#    else
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrStack.Offset = ctx.Esp;
#    endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    /* call-through-NULL (or into an unmapped page): rip/eip is bogus and the
     * frame pointer is gone, so a normal StackWalk yields nothing. The failed
     * CALL pushed the return address, so [rsp]/[esp] is the caller -- recover it
     * so the backtrace at least shows the call site. */
    if ((frame.AddrPC.Offset == 0 || SymGetModuleBase64(proc, frame.AddrPC.Offset) == 0) && frame.AddrStack.Offset) {
#    if defined(_WIN64)
        if (!IsBadReadPtr((void *)(uintptr_t)frame.AddrStack.Offset, 8)) {
            DWORD64 ret = *(DWORD64 *)(uintptr_t)frame.AddrStack.Offset;
            cr_emit(fd, "  (bad-call rip=0x%llx) return address [rsp] = 0x%llx\n",
                    (unsigned long long)frame.AddrPC.Offset, (unsigned long long)ret);
            if (ret) {
                frame.AddrPC.Offset = ret;
                frame.AddrStack.Offset += 8;
                ctx.Rip = ret;
                ctx.Rsp = frame.AddrStack.Offset;
            }
        }
#    else
        DWORD ret = 0;
        if (!IsBadReadPtr((void *)(uintptr_t)frame.AddrStack.Offset, 4)) {
            ret = *(DWORD *)(uintptr_t)frame.AddrStack.Offset;
            cr_emit(fd, "  (bad-call eip=0x%08lx) return address [esp] = 0x%08lx\n",
                    (unsigned long)frame.AddrPC.Offset, (unsigned long)ret);
        }
        if (ret) {
            frame.AddrPC.Offset = ret;
            frame.AddrStack.Offset += 4;
            ctx.Eip = ret;
            ctx.Esp = frame.AddrStack.Offset;
        }
#    endif
    }

    cr_emit(fd, "backtrace (most recent first):\n");
    {
        /* Always symbolize the faulting instruction directly: on x64 StackWalk64 can produce
           zero frames when unwind info is unavailable (rbp=0), leaving no source line at all. */
#    if defined(_WIN64)
        DWORD64 fpc = (DWORD64)ctx.Rip;
#    else
        DWORD64 fpc = (DWORD64)ctx.Eip;
#    endif
        DWORD64 fdisp = 0, fmod = SymGetModuleBase64(proc, fpc);
        char fmodName[MAX_PATH] = "?";
        IMAGEHLP_LINE64 fline;
        DWORD flineDisp = 0;
        const char *fname = "??";
        if (fmod)
            GetModuleFileNameA((HMODULE)(uintptr_t)fmod, fmodName, sizeof(fmodName));
        memset(sym, 0, sizeof(symbuf));
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 500;
        if (SymFromAddr(proc, fpc, &fdisp, sym))
            fname = sym->Name;
        memset(&fline, 0, sizeof(fline));
        fline.SizeOfStruct = sizeof(fline);
        if (SymGetLineFromAddr64(proc, fpc, &flineDisp, &fline))
            cr_emit(fd, "  [faulting rip] %s+0x%llx  (%s:%lu)\n", fname,
                    (unsigned long long)fdisp, fline.FileName, (unsigned long)fline.LineNumber);
        else
            cr_emit(fd, "  [faulting rip] %s+0x%llx  [%s+0x%llx]\n", fname,
                    (unsigned long long)fdisp, fmodName,
                    (unsigned long long)(fmod ? fpc - fmod : 0));
    }
    while (depth < 64 && StackWalk64(machine, proc, thread, &frame, &ctx, NULL,
                                     SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        DWORD64 pc = frame.AddrPC.Offset;
        DWORD64 disp = 0;
        DWORD64 modBase;
        char modName[MAX_PATH] = "?";
        const char *name = "??";
        IMAGEHLP_LINE64 line;
        DWORD lineDisp = 0;
        if (pc == 0)
            break;

        modBase = SymGetModuleBase64(proc, pc);
        if (modBase) {
            HMODULE hm = (HMODULE)(uintptr_t)modBase;
            GetModuleFileNameA(hm, modName, sizeof(modName));
        }
        memset(sym, 0, sizeof(symbuf));
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 500;
        if (SymFromAddr(proc, pc, &disp, sym))
            name = sym->Name;

        memset(&line, 0, sizeof(line));
        line.SizeOfStruct = sizeof(line);
        if (SymGetLineFromAddr64(proc, pc, &lineDisp, &line))
            cr_emit(fd, "  #%-2d 0x%016llx  %s+0x%llx  (%s:%lu)  [%s+0x%llx]\n",
                    depth, (unsigned long long)pc, name, (unsigned long long)disp,
                    line.FileName, (unsigned long)line.LineNumber,
                    modName, (unsigned long long)(pc - modBase));
        else
            cr_emit(fd, "  #%-2d 0x%016llx  %s+0x%llx  [%s+0x%llx]\n",
                    depth, (unsigned long long)pc, name, (unsigned long long)disp,
                    modName, (unsigned long long)(modBase ? pc - modBase : 0));
        depth++;
    }
    cr_emit(fd, "\n");

    /* Stack-scan fallback: when the fault is in a system DLL (StackWalk yields no
     * usable frames), scan the stack for values that land in our exe -- likely
     * return addresses -- and print exe-relative offsets so cod2_win32.map can
     * localize the caller. */
    {
        uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
        uintptr_t lo = base, hi = base + 0x900000; /* generous exe span */
#if defined(_WIN64)
        uintptr_t sp = (uintptr_t)ctx.Rsp;
#else
        uintptr_t sp = (uintptr_t)ctx.Esp;
#endif
        uintptr_t cur;
        int found = 0;
        cr_emit(fd, "stack scan (exe-range return-address candidates):\n");
        for (cur = sp; cur < sp + 0x6000 && found < 48; cur += sizeof(uintptr_t)) {
            uintptr_t v;
            if (IsBadReadPtr((void *)cur, sizeof(v)))
                break;
            v = *(uintptr_t *)cur;
            if (v >= lo && v < hi) {
                cr_emit(fd, "  [rsp+0x%04llx] 0x%016llx  rel=0x%llx\n",
                        (unsigned long long)(cur - sp), (unsigned long long)v,
                        (unsigned long long)(v - base));
                found++;
            }
        }
        if (!found)
            cr_emit(fd, "  (none found)\n");
        cr_emit(fd, "\n");
    }
    SymCleanup(proc);
}

static const char *cr_win_exname(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "ACCESS_VIOLATION (segfault)";
    case EXCEPTION_STACK_OVERFLOW:
        return "STACK_OVERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_PRIV_INSTRUCTION:
        return "PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:
        return "IN_PAGE_ERROR";
    case 0xC0000409:
        return "STACK_BUFFER_OVERRUN (stack smashing)";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "DATATYPE_MISALIGNMENT";
    default:
        return "exception";
    }
}

static void cr_win_do(EXCEPTION_POINTERS *ep, const char *descIn)
{
    char path[1024];
    char desc[256];
    int fd;
    DWORD pid = GetCurrentProcessId();

    if (cr_inHandler)
        return;
    cr_inHandler = 1;

    if (ep && ep->ExceptionRecord) {
        DWORD code = ep->ExceptionRecord->ExceptionCode;
        if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2)
            snprintf(desc, sizeof(desc), "%s at address 0x%p (%s)", cr_win_exname(code),
                     (void *)ep->ExceptionRecord->ExceptionInformation[1],
                     ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read");
        else
            snprintf(desc, sizeof(desc), "%s (code 0x%08lx)", cr_win_exname(code), (unsigned long)code);
    } else {
        snprintf(desc, sizeof(desc), "%s", descIn ? descIn : "fatal error");
    }

    snprintf(path, sizeof(path), "cod2_crash_%lu.txt", (unsigned long)pid);
    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);

    cr_emit_common(fd, desc);
    cr_win_regs(fd, ep ? ep->ContextRecord : NULL);
    cr_win_backtrace(fd, ep ? ep->ContextRecord : NULL);
    cr_emit(fd, "================ END CRASH REPORT ================\n");
    if (fd >= 0)
        close(fd);

    {
        int fd2 = open("Z:\\tmp\\cod2_crash_latest.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd2 >= 0) {
            cr_emit_common(fd2, desc);
            cr_win_regs(fd2, ep ? ep->ContextRecord : NULL);
            cr_win_backtrace(fd2, ep ? ep->ContextRecord : NULL);
            cr_emit(fd2, "================ END CRASH REPORT ================\n");
            close(fd2);
        }
    }
    if (getenv("COD2_CRASH_NOPOPUP"))
        return;

    cr_win_popup(path, desc);
}

static LONG WINAPI cr_seh_filter(EXCEPTION_POINTERS *ep)
{
    cr_win_do(ep, NULL);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void cr_abort_handler(int sig)
{
    (void)sig;
    cr_win_do(NULL, "abort() / stack smashing detected");
    _exit(3);
}

void Sys_InstallCrashHandler(const char *appName, const char *version,
                             const char *buildDate, const char *cmdline)
{
    if (appName && appName[0])
        snprintf(cr_appName, sizeof(cr_appName), "%s", appName);
    if (version && version[0])
        snprintf(cr_version, sizeof(cr_version), "%s", version);
    if (buildDate && buildDate[0])
        snprintf(cr_buildDate, sizeof(cr_buildDate), "%s", buildDate);
    if (cmdline)
        snprintf(cr_cmdline, sizeof(cr_cmdline), "%s", cmdline);
    GetModuleFileNameA(NULL, cr_exePath, sizeof(cr_exePath));

    /* Reserve guard-stack space so the SEH filter can still run (and write a report) on a
       stack overflow -- otherwise the process dies silently with no crash file. */
    {
        ULONG guarantee = 65536;
        SetThreadStackGuarantee(&guarantee);
    }

    SetUnhandledExceptionFilter(cr_seh_filter);
    signal(SIGABRT, cr_abort_handler);
}

#elif defined(__EMSCRIPTEN__)

void Sys_InstallCrashHandler(const char *appName, const char *version,
                             const char *buildDate, const char *cmdline)
{
    (void)appName;
    (void)version;
    (void)buildDate;
    (void)cmdline;
}

#else

#    include <unistd.h>
#    include <signal.h>
#    include <fcntl.h>
#    include <sys/utsname.h>
#    include <sys/wait.h>

#    ifndef __APPLE__
#        include <execinfo.h>
#    endif
#    define __USE_GNU
#    include <ucontext.h>
#    include <dlfcn.h>

static const char *cr_sig_name(int sig)
{
    switch (sig) {
    case SIGSEGV:
        return "SIGSEGV (segmentation fault)";
    case SIGABRT:
        return "SIGABRT (abort / stack smashing detected)";
    case SIGFPE:
        return "SIGFPE (arithmetic exception)";
    case SIGILL:
        return "SIGILL (illegal instruction)";
    case SIGBUS:
        return "SIGBUS (bus error)";
    case SIGTRAP:
        return "SIGTRAP (trap)";
    default:
        return "fatal signal";
    }
}

static void cr_posix_regs(int fd, void *ucontext)
{
    ucontext_t *uc = (ucontext_t *)ucontext;
    if (!uc)
        return;
#    if defined(__x86_64__) || defined(_M_X64)
    {
        greg_t *r = uc->uc_mcontext.gregs;
        cr_emit(fd, "registers:\n");
        cr_emit(fd, "  rip=%016llx rsp=%016llx rbp=%016llx efl=%08llx\n",
                (unsigned long long)r[REG_RIP], (unsigned long long)r[REG_RSP],
                (unsigned long long)r[REG_RBP], (unsigned long long)r[REG_EFL]);
        cr_emit(fd, "  rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n",
                (unsigned long long)r[REG_RAX], (unsigned long long)r[REG_RBX],
                (unsigned long long)r[REG_RCX], (unsigned long long)r[REG_RDX]);
        cr_emit(fd, "  rsi=%016llx rdi=%016llx r8 =%016llx r9 =%016llx\n",
                (unsigned long long)r[REG_RSI], (unsigned long long)r[REG_RDI],
                (unsigned long long)r[REG_R8], (unsigned long long)r[REG_R9]);
    }
#    elif defined(__i386__)
    {
        greg_t *r = uc->uc_mcontext.gregs;
        cr_emit(fd, "registers:\n");
        cr_emit(fd, "  eip=%08x esp=%08x ebp=%08x efl=%08x\n",
                (unsigned)r[REG_EIP], (unsigned)r[REG_ESP], (unsigned)r[REG_EBP], (unsigned)r[REG_EFL]);
        cr_emit(fd, "  eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
                (unsigned)r[REG_EAX], (unsigned)r[REG_EBX], (unsigned)r[REG_ECX], (unsigned)r[REG_EDX]);
        cr_emit(fd, "  esi=%08x edi=%08x\n", (unsigned)r[REG_ESI], (unsigned)r[REG_EDI]);
    }
#    else
    cr_emit(fd, "registers: (unsupported arch)\n");
#    endif
    cr_emit(fd, "\n");
}

static void cr_posix_backtrace(int fd)
{
#    ifndef __APPLE__
    void *frames[64];
    int n = backtrace(frames, 64);
    Dl_info dli;
    int i;

    cr_emit(fd, "backtrace (most recent first, %d frames):\n", n);
    for (i = 0; i < n; i++) {
        unsigned long addr = (unsigned long)frames[i];
        if (dladdr(frames[i], &dli) && dli.dli_fname) {
            unsigned long base = (unsigned long)dli.dli_fbase;
            const char *modname = strrchr(dli.dli_fname, '/');
            modname = modname ? modname + 1 : dli.dli_fname;
            if (dli.dli_sname)
                cr_emit(fd, "  #%-2d 0x%012lx  %s+0x%lx  [%s+0x%lx]\n", i, addr,
                        dli.dli_sname, addr - (unsigned long)dli.dli_saddr,
                        modname, addr - base);
            else
                cr_emit(fd, "  #%-2d 0x%012lx  [%s+0x%lx]\n", i, addr, modname, addr - base);
        } else {
            cr_emit(fd, "  #%-2d 0x%012lx\n", i, addr);
        }
    }
    cr_emit(fd, "\n");

    if (cr_exePath[0] && n > 0) {
        char *argv[64 + 5];
        char addrbuf[64][20];
        int argc = 0, k;
        pid_t pid;
        argv[argc++] = (char *)"addr2line";
        argv[argc++] = (char *)"-a";
        argv[argc++] = (char *)"-f";
        argv[argc++] = (char *)"-C";
        argv[argc++] = (char *)"-p";
        argv[argc++] = (char *)"-e";
        argv[argc++] = cr_exePath;
        for (k = 0; k < n && k < 64; k++) {
            snprintf(addrbuf[k], sizeof(addrbuf[k]), "0x%lx", (unsigned long)frames[k]);
            argv[argc++] = addrbuf[k];
        }
        argv[argc] = NULL;
        cr_emit(fd, "resolved backtrace:\n");
        pid = fork();
        if (pid == 0) {
            if (fd >= 0)
                dup2(fd, 1);
            else {
                int dn = open("/dev/null", O_WRONLY);
                if (dn >= 0)
                    dup2(dn, 1);
            }
            dup2(1, 2);
            execvp("addr2line", argv);
            _exit(127);
        } else if (pid > 0) {
            int st;
            waitpid(pid, &st, 0);
        }
        cr_emit(fd, "\n");
    }

    if (cr_exePath[0]) {
        cr_emit(fd, "to resolve manually: addr2line -f -C -e %s", cr_exePath);
        for (i = 0; i < n; i++)
            cr_emit(fd, " 0x%lx", (unsigned long)frames[i]);
        cr_emit(fd, "\n\n");
    }
    cr_emit(fd, "raw frames:\n");
    if (fd >= 0)
        backtrace_symbols_fd(frames, n, fd);
    cr_emit(fd, "\n");
#    else
    cr_emit(fd, "backtrace: (unavailable on this platform)\n\n");
#    endif
}

static void cr_posix_osinfo(int fd)
{
    struct utsname u;
    if (uname(&u) == 0)
        cr_emit(fd, "os         : %s %s %s (%s)\n", u.sysname, u.release, u.version, u.machine);
}

#    if defined(__linux__)
#        include <X11/Xlib.h>
#        include <X11/keysym.h>

static void cr_x11_fallback(const char *path)
{
    static char buf[200 * 1024];
    static char *lines[8192];
    int nlines = 0;
    FILE *f;
    size_t rd = 0, i;
    Display *d;
    Window w;
    Atom wmDelete;
    XFontStruct *fnt;
    GC gc;
    int s, fh, asc, top = 0, winH = 700, running = 1;

    f = fopen(path, "rb");
    if (f) {
        rd = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
    }
    buf[rd] = '\0';
    lines[nlines++] = buf;
    for (i = 0; i < rd && nlines < 8192; i++)
        if (buf[i] == '\n') {
            buf[i] = '\0';
            if (i + 1 < rd)
                lines[nlines++] = &buf[i + 1];
        }

    d = XOpenDisplay(NULL);
    if (!d)
        return;
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 0, 0, 1000, 700, 0,
                            BlackPixel(d, s), WhitePixel(d, s));
    XStoreName(d, w, "CoD2 crashed - scroll: arrows/wheel/PgUp/PgDn, Esc to close");
    wmDelete = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, w, &wmDelete, 1);
    XSelectInput(d, w, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
    fnt = XLoadQueryFont(d, "9x15");
    if (!fnt)
        fnt = XLoadQueryFont(d, "fixed");
    gc = XCreateGC(d, w, 0, NULL);
    if (fnt)
        XSetFont(d, gc, fnt->fid);
    fh = fnt ? fnt->ascent + fnt->descent : 15;
    asc = fnt ? fnt->ascent : 12;
    XMapWindow(d, w);

    while (running) {
        XEvent e;
        XNextEvent(d, &e);
        if (e.type == Expose || e.type == ConfigureNotify) {
            int rows, k;
            if (e.type == ConfigureNotify)
                winH = e.xconfigure.height;
            XClearWindow(d, w);
            rows = winH / fh;
            for (k = 0; k < rows && top + k < nlines; k++)
                XDrawString(d, w, gc, 4, asc + k * fh, lines[top + k], (int)strlen(lines[top + k]));
        } else if (e.type == KeyPress) {
            KeySym key = XLookupKeysym(&e.xkey, 0);
            int rows = winH / fh;
            if (key == XK_Escape || key == XK_q)
                running = 0;
            else if (key == XK_Down)
                top += 1;
            else if (key == XK_Up)
                top -= 1;
            else if (key == XK_Next)
                top += rows - 2;
            else if (key == XK_Prior)
                top -= rows - 2;
            else if (key == XK_Home)
                top = 0;
            else if (key == XK_End)
                top = nlines - rows;
            if (top < 0)
                top = 0;
            if (top > nlines - 1)
                top = nlines - 1;
            XClearArea(d, w, 0, 0, 0, 0, True);
        } else if (e.type == ButtonPress) {
            if (e.xbutton.button == 5)
                top += 3;
            else if (e.xbutton.button == 4)
                top -= 3;
            if (top < 0)
                top = 0;
            if (top > nlines - 1)
                top = nlines - 1;
            XClearArea(d, w, 0, 0, 0, 0, True);
        } else if (e.type == ClientMessage) {
            running = 0;
        }
    }
    XCloseDisplay(d);
}
#    endif

static void cr_posix_popup(const char *path, const char *desc)
{
    char title[300], fnarg[1100], titlearg[320];
    pid_t pid;
    if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY"))
        return;
    snprintf(title, sizeof(title), "%s crashed - %s", cr_appName, desc);
    snprintf(titlearg, sizeof(titlearg), "--title=%s", title);
    snprintf(fnarg, sizeof(fnarg), "--filename=%s", path);
    pid = fork();
    if (pid == 0) {

        execlp("zenity", "zenity", "--text-info", fnarg, titlearg,
               "--width=1000", "--height=680", "--font=monospace 9", (char *)NULL);
        execlp("kdialog", "kdialog", "--title", title, "--textbox", path, "1000", "680", (char *)NULL);
        execlp("xmessage", "xmessage", "-center", "-file", path, (char *)NULL);
#    if defined(__linux__)
        cr_x11_fallback(path);
#    endif
        _exit(127);
    }

}

static struct sigaction cr_old[8];
static const int cr_sigs[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS };

static void cr_posix_handler(int sig, siginfo_t *info, void *ucontext)
{
    char path[1024];
    char desc[256];
    int fd;

    if (cr_inHandler) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }
    cr_inHandler = 1;

    if ((sig == SIGSEGV || sig == SIGBUS) && info)
        snprintf(desc, sizeof(desc), "%s, fault address 0x%lx",
                 cr_sig_name(sig), (unsigned long)(uintptr_t)info->si_addr);
    else
        snprintf(desc, sizeof(desc), "%s", cr_sig_name(sig));

    snprintf(path, sizeof(path), "cod2_crash_%d.txt", (int)getpid());
    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);

    cr_emit_common(fd, desc);
    cr_posix_osinfo(fd);
    cr_emit(fd, "\n");
    cr_posix_regs(fd, ucontext);
    cr_posix_backtrace(fd);
    cr_emit(fd, "================ END CRASH REPORT ================\n");
    if (fd >= 0)
        close(fd);

    cr_posix_popup(path, desc);

    signal(sig, SIG_DFL);
    raise(sig);
}

void Sys_InstallCrashHandler(const char *appName, const char *version,
                             const char *buildDate, const char *cmdline)
{
    static char altstack[65536];
    stack_t ss;
    struct sigaction sa;
    unsigned i;

    if (appName && appName[0])
        snprintf(cr_appName, sizeof(cr_appName), "%s", appName);
    if (version && version[0])
        snprintf(cr_version, sizeof(cr_version), "%s", version);
    if (buildDate && buildDate[0])
        snprintf(cr_buildDate, sizeof(cr_buildDate), "%s", buildDate);
    if (cmdline)
        snprintf(cr_cmdline, sizeof(cr_cmdline), "%s", cmdline);
    {
        ssize_t n = readlink("/proc/self/exe", cr_exePath, sizeof(cr_exePath) - 1);
        if (n > 0)
            cr_exePath[n] = '\0';
    }

    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = cr_posix_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    for (i = 0; i < sizeof(cr_sigs) / sizeof(cr_sigs[0]); i++)
        sigaction(cr_sigs[i], &sa, &cr_old[i]);
}

#endif
