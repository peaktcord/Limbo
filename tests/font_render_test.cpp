#include "src/common/render/render.hpp"

#include <cassert>
#include <cstdint>

namespace {

uint64_t hash_surface(const render::Surface &surface) {
    uint64_t hash = 1469598103934665603ULL;
    for (uint32_t pixel : surface.pixels) {
        hash ^= pixel;
        hash *= 1099511628211ULL;
    }
    return hash;
}

}

int main() {
    SessionContext session;
    SessionScope scope(session);

    Font *small_plain = Font::getFont(0, 0, 8);
    Font *medium_plain = Font::getFont(0, 0, 0);
    Font *small_bold = Font::getFont(0, 1, 8);
    Font *medium_bold = Font::getFont(0, 1, 0);
    Font *large_bold = Font::getFont(0, 1, 16);
    Font *large_italic = Font::getFont(0, 2, 16);

    assert(small_plain->getHeight() == 12);
    assert(medium_plain->getHeight() == 13);
    assert(small_bold->getHeight() == 12);
    assert(medium_bold->getHeight() == 13);
    assert(large_bold->getHeight() == 17);
    assert(large_italic->getHeight() == 17);
    assert(small_bold->charWidth('i') == 3);
    assert(small_bold->charWidth('m') == 11);
    assert(small_bold->stringWidth(std::string("iii")) == 9);
    assert(small_bold->stringWidth(std::string("mmm")) == 33);
    assert(small_bold->substringWidth(std::string("aim"), 1, 2) == 14);
    assert(small_plain->charWidth((uint16_t)0x100) == small_plain->charWidth('?'));
    assert(medium_bold->charWidth('m') == 12);
    assert(large_bold->charWidth('m') == 14);
    assert(large_italic->charWidth('m') == 11);

    const uint64_t expected_face_hashes[] = {
        8744483760284289267ULL,
        16966031333497483303ULL,
        16884497473336252700ULL,
        7431572804960067206ULL,
        13856220605058401216ULL,
        4030885033484826709ULL,
    };
    Font *faces[] = {small_plain, medium_plain, small_bold, medium_bold,
                     large_bold, large_italic};
    for (size_t index = 0; index < 6; ++index) {
        Font *font = faces[index];
        render::Surface face(64, 24);
        render::SoftGraphics face_graphics(&face);
        face_graphics.setColor(0xFFFFFF);
        face_graphics.setFont(font);
        face_graphics.drawString(std::string("Ami-?"), 1, 1,
                                 render::LEFT | render::TOP);
        assert(hash_surface(face) == expected_face_hashes[index]);
    }

    render::Surface surface(64, 24);
    render::SoftGraphics graphics(&surface);
    graphics.setColor(0xFFFFFF);
    graphics.setFont(small_bold);
    graphics.drawString(std::string("Nokia 3650"), 2, 1, render::LEFT | render::TOP);
    assert(hash_surface(surface) == 11893345547572349724ULL);

    render::Surface fallback(16, 16);
    render::Surface question(16, 16);
    render::SoftGraphics fallback_graphics(&fallback);
    render::SoftGraphics question_graphics(&question);
    fallback_graphics.setColor(0xFFFFFF);
    question_graphics.setColor(0xFFFFFF);
    fallback_graphics.setFont(small_plain);
    question_graphics.setFont(small_plain);
    fallback_graphics.drawChar((uint16_t)0x100, 0, 0, render::LEFT | render::TOP);
    question_graphics.drawChar('?', 0, 0, render::LEFT | render::TOP);
    assert(fallback.pixels == question.pixels);

    for (Font *font : faces) {
        for (int code = 0; code < 256; ++code) {
            const std::string text(1, static_cast<char>(code));
            assert(font->stringWidth(text) == font->charWidth(static_cast<uint16_t>(code)));
            assert(font->substringWidth("A" + text + "z", 1, 1) == font->charWidth(static_cast<uint16_t>(code)));
            render::Surface by_text(32, 24), by_code(32, 24);
            render::SoftGraphics text_graphics(&by_text), code_graphics(&by_code);
            text_graphics.setFont(font);
            code_graphics.setFont(font);
            text_graphics.setColor(0xFFFFFF);
            code_graphics.setColor(0xFFFFFF);
            text_graphics.drawString(text, 1, 1, render::LEFT | render::TOP);
            code_graphics.drawChar(static_cast<uint16_t>(code), 1, 1, render::LEFT | render::TOP);
            assert(by_text.pixels == by_code.pixels);
        }
    }
    return 0;
}
