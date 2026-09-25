#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "src/platform/crash_trace.hpp"
#include "src/common/session.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
std::string read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}

int child(const std::string &mode, const char *path) {
    namespace log = voyage::logging;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    log::open_file(path);
    voyage::crash_trace::install();
    log::set_context("phase=script interpreter level=/test.scr opcode_pc=123 actor_slot=4");
    if (mode == "native") {
        const ULONG_PTR parameters[] = {0, 0x1234};
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 2, parameters);
    } else if (mode == "terminate") {
        throw std::runtime_error("controlled uncaught exception");
    } else {
        voyage::crash_trace::install_throw_trace();
        try {
            SharedArray<int32_t> values(1);
            values[17] = 1;
            assert(false);
        } catch (const std::out_of_range &error) {
            assert(std::string(error.what()) == "Array index 17 out of bounds for length 1");
            log::exception("Test", "Caught bounds exception", error);
        }
        voyage::crash_trace::uninstall();
        log::close_file();
        return 0;
    }
    return 2;
}
}

int main(int argc, char **argv) {
    if (argc == 4 && std::string(argv[1]) == "--child") return child(argv[2], argv[3]);
    const char *temp = std::getenv("TEST_TMPDIR");
    assert(temp != nullptr);
    const std::filesystem::path root = std::filesystem::path(temp) / "crash-trace";
    std::filesystem::create_directories(root);
    wchar_t executable[32768]{};
    assert(GetModuleFileNameW(nullptr, executable, 32768) != 0);
    for (const std::string mode : {"native", "terminate", "throw"}) {
        const auto log = root / (mode + ".log");
        const auto console = root / (mode + ".console");
        SECURITY_ATTRIBUTES security{sizeof security, nullptr, TRUE};
        HANDLE output = CreateFileW(console.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                    &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(output != INVALID_HANDLE_VALUE);
        HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &security, OPEN_EXISTING, 0, nullptr);
        assert(input != INVALID_HANDLE_VALUE);
        STARTUPINFOW startup{};
        startup.cb = sizeof startup;
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdOutput = startup.hStdError = output;
        startup.hStdInput = input;
        PROCESS_INFORMATION process{};
        std::wstring command = L"\"" + std::wstring(executable) + L"\" --child " +
            std::wstring(mode.begin(), mode.end()) + L" \"" + log.wstring() + L"\"";
        assert(CreateProcessW(executable, command.data(), nullptr, nullptr, TRUE,
                              CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process));
        const DWORD wait = WaitForSingleObject(process.hProcess, 20000);
        if (wait != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 2);
        assert(wait == WAIT_OBJECT_0);
        DWORD status = 0;
        assert(GetExitCodeProcess(process.hProcess, &status));
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        CloseHandle(input);
        CloseHandle(output);
        assert((mode == "throw") == (status == 0));
        const std::string report = read(log);
        assert(report.find("level=/test.scr opcode_pc=123 actor_slot=4") != std::string::npos);
        assert(report.find("#0 0x") != std::string::npos);
        if (mode == "native") {
            assert(report.find("Native crash: access violation (0xc0000005)") != std::string::npos);
            assert(report.find("reading address 0x1234") != std::string::npos);
        } else if (mode == "terminate") {
            assert(report.find("uncaught exception: controlled uncaught exception") != std::string::npos);
        } else {
            assert(report.find("Throw site:") != std::string::npos);
            assert(report.find("Caught bounds exception") != std::string::npos);
            assert(report.find("Array index 17 out of bounds for length 1") != std::string::npos);
            assert(report.find("java.lang.") == std::string::npos);
        }
        assert(read(console).empty());
    }
}
