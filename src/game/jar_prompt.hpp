#ifndef VOYAGE_GAME_JAR_PROMPT_HPP
#define VOYAGE_GAME_JAR_PROMPT_HPP

#include <string>

#include "src/game/game_runtime.hpp"

namespace voyage {

struct JarPromptText {
    std::string invitation;
    std::string retry;
    std::string dismissal;
};

JarPromptText desktop_prompt_text();

Frame jar_prompt_frame(const std::string &error = {},
                       const JarPromptText &text = desktop_prompt_text());

}

#endif
