#ifndef VOYAGE_DIAGNOSTICS_LOG_HPP
#define VOYAGE_DIAGNOSTICS_LOG_HPP

#include <cstdio>
#include <exception>
#include <filesystem>
#include <string_view>

namespace voyage::logging {

enum class Level { Info, Warning, Error, Fatal };

void open_file(const std::filesystem::path &path);
void close_file() noexcept;
void write(Level level, const char *component, std::string_view message) noexcept;
void exception(const char *component, const char *operation, const std::exception &error) noexcept;

using PersistHook = void (*)();
void set_persist_hook(PersistHook hook) noexcept;

void set_context(std::string_view context) noexcept;
const char *last_context() noexcept;

std::FILE *file_handle() noexcept;

using ThrowTraceHook = void (*)(const char *);
void set_throw_trace_hook(ThrowTraceHook hook) noexcept;
void trace_throw(const char *message) noexcept;

}

#endif
