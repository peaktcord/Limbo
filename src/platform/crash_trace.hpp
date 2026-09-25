#ifndef VOYAGE_PLATFORM_CRASH_TRACE_HPP
#define VOYAGE_PLATFORM_CRASH_TRACE_HPP

namespace voyage::crash_trace {

void install();
void uninstall();
void install_throw_trace();

}

#endif
