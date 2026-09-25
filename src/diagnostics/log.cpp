#include "src/diagnostics/log.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <mutex>
#include <stdexcept>

namespace voyage::logging {
namespace {

std::FILE *log_file = nullptr;
std::mutex log_mutex;
PersistHook persist_hook = nullptr;
std::atomic<ThrowTraceHook> throw_hook{nullptr};
thread_local char context_buffer[2048] = "startup";

const char *level_name(Level level) {
    switch (level) {
        case Level::Info: return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
    }
    return "ERROR";
}

void write_text(std::string_view text) {
    for (char c : text) {
        if (c == '\n') std::fputs("\\n", log_file);
        else if (c == '\r') std::fputs("\\r", log_file);
        else if ((unsigned char)c < 32 && c != '\t') std::fputc('?', log_file);
        else std::fputc((unsigned char)c, log_file);
    }
}

}

void open_file(const std::filesystem::path &path) {
    namespace fs = std::filesystem;
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file != nullptr) throw std::runtime_error("a session log is already open");
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    fs::path previous = path;
    previous += ".prev";
    const bool rotate = fs::exists(path);
    if (rotate) {
        if (!fs::is_regular_file(path)) throw std::runtime_error("log path is not a regular file: " + path.string());
        fs::rename(path, previous);
    }
#ifdef _WIN32
    log_file = _wfopen(path.c_str(), L"wb");
#else
    log_file = std::fopen(path.c_str(), "wb");
#endif
    if (log_file == nullptr) {
        if (rotate) {
            std::error_code ec;
            fs::rename(previous, path, ec);
        }
        throw std::runtime_error("cannot open session log: " + path.string());
    }
}

void close_file() noexcept {
    std::lock_guard<std::mutex> lock(log_mutex);
    persist_hook = nullptr;
    if (log_file != nullptr) std::fclose(log_file);
    log_file = nullptr;
}

void write(Level level, const char *component, std::string_view message) noexcept {
    PersistHook notify = nullptr;
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file == nullptr) return;
        const std::time_t now = std::time(nullptr);
        std::tm utc{};
#ifdef _WIN32
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        char timestamp[32]{};
        std::strftime(timestamp, sizeof timestamp, "%Y-%m-%dT%H:%M:%SZ", &utc);
        std::fprintf(log_file, "%s [%s] ", timestamp, level_name(level));
        write_text(component);
        std::fputs(": ", log_file);
        write_text(message);
        std::fputc('\n', log_file);
        std::fflush(log_file);
        notify = persist_hook;
    }
    if (notify != nullptr) notify();
}

void exception(const char *component, const char *operation, const std::exception &error) noexcept {
    char message[2048]{};
    std::snprintf(message, sizeof message, "%s: %s", operation, error.what());
    write(Level::Error, component, message);
}

void set_persist_hook(PersistHook hook) noexcept {
    std::lock_guard<std::mutex> lock(log_mutex);
    persist_hook = hook;
}

void set_context(std::string_view context) noexcept {
    const size_t size = std::min(context.size(), sizeof context_buffer - 1);
    if (size != 0) std::memcpy(context_buffer, context.data(), size);
    context_buffer[size] = '\0';
}

const char *last_context() noexcept { return context_buffer; }
std::FILE *file_handle() noexcept { return log_file; }
void set_throw_trace_hook(ThrowTraceHook hook) noexcept { throw_hook.store(hook); }
void trace_throw(const char *message) noexcept {
    if (ThrowTraceHook hook = throw_hook.load()) hook(message);
}

}
