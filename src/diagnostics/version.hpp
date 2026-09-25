#ifndef VOYAGE_DIAGNOSTICS_VERSION_HPP
#define VOYAGE_DIAGNOSTICS_VERSION_HPP

#include <string>

namespace voyage::version {

inline constexpr int kBuild = 1;

const char *commit();

inline std::string describe() {
    return "build " + std::to_string(kBuild) + " (" + commit() + ")";
}

}

#endif
