#include "src/game/jar_prompt.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "src/common/render/render.hpp"

namespace voyage {
namespace {

std::vector<std::string> wrap_text(const std::string &text, Font *font, int32_t width) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = start;
        size_t space = std::string::npos;
        int32_t pixels = 0;
        while (end < text.size() && text[end] != '\n') {
            const int32_t advance = font->charWidth((unsigned char)text[end]);
            if (pixels + advance > width) break;
            if (text[end] == ' ') space = end;
            pixels += advance;
            ++end;
        }
        if (end < text.size() && text[end] != '\n' && space != std::string::npos) end = space;
        if (end == start && text[end] != '\n') ++end;
        lines.push_back(text.substr(start, end - start));
        start = end;
        while (start < text.size() && text[start] == ' ') ++start;
        if (start < text.size() && text[start] == '\n') ++start;
    }
    return lines;
}

}

JarPromptText desktop_prompt_text() {
    return JarPromptText{
        "Drop Oblivion's .jar\non this window.",
        "Drop Oblivion's .jar\nhere to try again.",
        "Esc to quit",
    };
}

Frame jar_prompt_frame(const std::string &error, const JarPromptText &text) {
    SessionContext session;
    SessionScope scope(session);
    render::Surface screen(kSurfaceWidth, kScreenHeight);
    render::SoftGraphics graphics(&screen);
    graphics.setColor(0xFFFFFF);
    graphics.setFont(Font::getFont(0, 1, 0));
    graphics.drawString(std::string("Limbo"), kPhoneWidth / 2, 8, render::HCENTER | render::TOP);

    Font *body = Font::getFont(0, 0, 8);
    graphics.setFont(body);
    const std::string message =
        error.empty() ? text.invitation : "Could not import that JAR.\n\n" + error;
    auto lines = wrap_text(message, body, kPhoneWidth - 16);
    constexpr int32_t top = 34;
    const int32_t bottom = error.empty() ? kScreenHeight - 24 : kScreenHeight - 52;
    const int32_t line_height = body->getHeight();
    const size_t room = (size_t)((bottom - top) / line_height);
    if (lines.size() > room) {
        lines.resize(room);
        std::string &last = lines.back();
        while (!last.empty() && body->stringWidth(last + "...") > kPhoneWidth - 16) last.pop_back();
        last += "...";
    }
    int32_t y = top + (bottom - top - (int32_t)lines.size() * line_height) / 2;
    for (const auto &line : lines) {
        graphics.drawString(line, kPhoneWidth / 2, y, render::HCENTER | render::TOP);
        y += line_height;
    }
    if (!error.empty()) {
        int32_t retry_y = kScreenHeight - 44;
        for (const auto &line : wrap_text(text.retry, body, kPhoneWidth - 16)) {
            graphics.drawString(line, kPhoneWidth / 2, retry_y, render::HCENTER | render::TOP);
            retry_y += line_height;
        }
    }
    graphics.drawString(text.dismissal, kPhoneWidth / 2, kScreenHeight - 14,
                        render::HCENTER | render::TOP);
    return Frame{kPhoneWidth, kScreenHeight, kSurfaceWidth, std::move(screen.pixels)};
}

}
