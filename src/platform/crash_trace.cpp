#include "src/platform/crash_trace.hpp"
#include "src/diagnostics/log.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>

namespace voyage::crash_trace {
namespace {

LPTOP_LEVEL_EXCEPTION_FILTER previous_filter = nullptr;
std::terminate_handler previous_terminate = nullptr;
bool symbols_ready = false;
bool installed = false;

void report(const char *format, ...) {
    char line[2048]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof line, format, args);
    va_end(args);
    if (std::FILE *file = logging::file_handle()) {
        std::fputs(line, file);
        std::fflush(file);
    }
}

const char *exception_name(DWORD code) {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "access violation";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
        case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
        case 0xC0000409: return "stack buffer overrun";
        default: return "Windows exception";
    }
}

void report_context() {
    SYSTEMTIME now{};
    GetSystemTime(&now);
    report("    %04u-%02u-%02uT%02u:%02u:%02uZ pid=%lu thread=%lu\n",
           now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
           GetCurrentProcessId(), GetCurrentThreadId());
    report("    last game context: %s\n", logging::last_context());
}

void print_stack(CONTEXT context, int skip = 0) {
    HANDLE process = GetCurrentProcess();
    STACKFRAME64 frame{};
    DWORD machine;
#if defined(_M_X64)
    machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_IX86)
    machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#else
#error "crash_trace: unsupported architecture"
#endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    alignas(SYMBOL_INFO) char symbol_buffer[sizeof(SYMBOL_INFO) + 1024]{};
    auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbol_buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 1024;
    for (int walked = 0; walked < 64 + skip; ++walked) {
        if (!StackWalk64(machine, process, GetCurrentThread(), &frame, &context, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0) break;
        if (walked < skip) continue;
        const DWORD64 pc = frame.AddrPC.Offset;
        DWORD64 displacement = 0;
        const char *name = "unknown symbol";
        if (symbols_ready && SymFromAddr(process, pc, &displacement, symbol)) name = symbol->Name;
        IMAGEHLP_MODULE64 module{};
        module.SizeOfStruct = sizeof module;
        const bool have_module = symbols_ready && SymGetModuleInfo64(process, pc, &module);
        report("    #%d 0x%llx %s + 0x%llx [%s + 0x%llx]\n", walked - skip,
               (unsigned long long)pc, name, (unsigned long long)displacement,
               have_module ? module.ModuleName : "unknown module",
               (unsigned long long)(have_module ? pc - module.BaseOfImage : pc));
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof line;
        DWORD line_displacement = 0;
        if (symbols_ready && SymGetLineFromAddr64(process, pc, &line_displacement, &line)) {
            report("        %s:%lu\n", line.FileName, line.LineNumber);
        }
    }
}

LONG WINAPI on_crash(EXCEPTION_POINTERS *info) {
    const auto *record = info->ExceptionRecord;
    report("\n[FATAL] Native crash: %s (0x%08lx) at %p\n", exception_name(record->ExceptionCode),
           record->ExceptionCode, record->ExceptionAddress);
    report_context();
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
        const ULONG_PTR kind = record->ExceptionInformation[0];
        report("    %s address 0x%llx\n", kind == 1 ? "writing" : kind == 8 ? "executing" : "reading",
               (unsigned long long)record->ExceptionInformation[1]);
    }
    print_stack(*info->ContextRecord);
    if (previous_filter != nullptr) return previous_filter(info);
    return EXCEPTION_CONTINUE_SEARCH;
}

[[noreturn]] void on_terminate() {
    report("\n[FATAL] C++ termination\n");
    if (std::exception_ptr error = std::current_exception()) {
        try { std::rethrow_exception(error); }
        catch (const std::exception &exception) { report("    uncaught exception: %s\n", exception.what()); }
        catch (...) { report("    uncaught exception of unknown type\n"); }
    } else {
        report("    terminate called without an active exception\n");
    }
    report_context();
    CONTEXT context{};
    RtlCaptureContext(&context);
    print_stack(context);
    std::_Exit(EXIT_FAILURE);
}

void on_throw(const char *message) {
    static thread_local bool tracing = false;
    if (tracing) return;
    tracing = true;
    report("\n[ERROR] Throw site: %s\n", message);
    report_context();
    CONTEXT context{};
    RtlCaptureContext(&context);
    print_stack(context, 2);
    tracing = false;
}

}

void install() {
    if (installed) return;
    ULONG stack_reserve = 64 * 1024;
    SetThreadStackGuarantee(&stack_reserve);
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
    symbols_ready = SymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE;
    if (!symbols_ready) logging::write(logging::Level::Warning, "Crash trace", "Symbols unavailable; addresses will still be recorded");
    previous_filter = SetUnhandledExceptionFilter(on_crash);
    previous_terminate = std::set_terminate(on_terminate);
    installed = true;
}

void uninstall() {
    if (!installed) return;
    logging::set_throw_trace_hook(nullptr);
    SetUnhandledExceptionFilter(previous_filter);
    std::set_terminate(previous_terminate);
    if (symbols_ready) SymCleanup(GetCurrentProcess());
    symbols_ready = false;
    installed = false;
}

void install_throw_trace() { logging::set_throw_trace_hook(on_throw); }

}
