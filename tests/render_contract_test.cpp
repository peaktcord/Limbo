#include "src/common/render/render.hpp"

#include <cassert>
#include <cstdint>

namespace {

constexpr uint32_t kBlack = 0xFF000000u;
constexpr uint32_t kBlue = 0xFF0000FFu;
constexpr uint32_t kRed = 0xFFFF0000u;

void assert_rect(const render::Surface &surface, int32_t x0, int32_t y0,
                 int32_t x1, int32_t y1, uint32_t inside,
                 uint32_t outside) {
    for (int32_t y = 0; y < surface.height; ++y) {
        for (int32_t x = 0; x < surface.width; ++x) {
            bool in_rect = x >= x0 && x < x1 && y >= y0 && y < y1;
            assert(surface.row(y)[x] == (in_rect ? inside : outside));
        }
    }
}

}

int main() {
    SessionContext session;
    SessionScope scope(session);

    render::Surface clipped(5, 4);
    render::SoftGraphics clip_graphics(&clipped);
    clip_graphics.setClip(1, 1, 3, 2);
    clip_graphics.setColor(0xFF0000);
    clip_graphics.fillRect(-2, -2, 10, 10);
    assert_rect(clipped, 1, 1, 4, 3, kRed, kBlack);

    render::Surface line(5, 5);
    render::SoftGraphics line_graphics(&line);
    line_graphics.setClip(1, 1, 3, 3);
    line_graphics.setColor(0xFF0000);
    line_graphics.drawLine(0, 0, 4, 4);
    for (int32_t y = 0; y < 5; ++y) {
        for (int32_t x = 0; x < 5; ++x) {
            bool drawn = x == y && x >= 1 && x <= 3;
            assert(line.row(y)[x] == (drawn ? kRed : kBlack));
        }
    }

    render::Surface rectangles(5, 5);
    render::SoftGraphics rectangle_graphics(&rectangles);
    rectangle_graphics.setColor(0xFF0000);
    rectangle_graphics.drawRect(1, 1, 2, 2);
    for (int32_t y = 0; y < 5; ++y) {
        for (int32_t x = 0; x < 5; ++x) {
            bool edge = x >= 1 && x <= 3 && y >= 1 && y <= 3 &&
                        (x == 1 || x == 3 || y == 1 || y == 3);
            assert(rectangles.row(y)[x] == (edge ? kRed : kBlack));
        }
    }
    rectangle_graphics.drawRect(4, 4, 0, 0);
    assert(rectangles.row(4)[4] == kRed);

    render::Surface rounded(7, 7);
    render::SoftGraphics rounded_graphics(&rounded);
    rounded_graphics.setColor(0xFF0000);
    rounded_graphics.fillRoundRect(0, 0, 7, 7, 4, 4);
    assert(rounded.row(0)[0] == kBlack);
    assert(rounded.row(0)[1] == kBlack);
    assert(rounded.row(0)[2] == kRed);
    assert(rounded.row(0)[4] == kRed);
    assert(rounded.row(0)[5] == kBlack);
    assert(rounded.row(0)[6] == kBlack);
    for (int32_t x = 0; x < 7; ++x) assert(rounded.row(1)[x] == kRed);
    assert(rounded.row(6)[0] == kBlack);
    assert(rounded.row(6)[2] == kRed);

    Image *image = Image::createImage(3, 1);
    render::Surface *source = render::surfaceFor(image);
    source->pixels[0] = kRed;
    source->pixels[1] = 0x0000FF00u;
    source->pixels[2] = 0x80FFFFFFu;

    render::Surface blit(5, 2);
    render::SoftGraphics blit_graphics(&blit);
    blit_graphics.setColor(0x0000FF);
    blit_graphics.fillRect(0, 0, 5, 2);
    blit_graphics.drawImage(image, 4, 1, render::RIGHT | render::BOTTOM);
    assert(blit.row(0)[0] == kBlue);
    assert(blit.row(0)[1] == kRed);
    assert(blit.row(0)[2] == kBlue);
    assert(blit.row(0)[3] == 0xFF8080FFu);
    assert(blit.row(0)[4] == kBlue);
    for (int32_t x = 0; x < 5; ++x) assert(blit.row(1)[x] == kBlue);

    render::Surface mirrored(3, 1);
    render::SoftGraphics mirrored_graphics(&mirrored);
    mirrored_graphics.setColor(0x0000FF);
    mirrored_graphics.fillRect(0, 0, 3, 1);
    mirrored_graphics.drawImage(image, 0, 0, 0, Graphics::TRANS_MIRROR);
    assert(mirrored.row(0)[0] == 0xFF8080FFu);
    assert(mirrored.row(0)[1] == kBlue);
    assert(mirrored.row(0)[2] == kRed);

    Image *grid_image = Image::createImage(3, 2);
    render::Surface *grid = render::surfaceFor(grid_image);
    grid->pixels = {0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu,
                    0xFF00FFFFu, 0xFFFF00FFu, 0xFFFFFFFFu};
    render::Surface image_clipped(4, 3);
    render::SoftGraphics image_clip_graphics(&image_clipped);
    image_clip_graphics.setClip(1, 1, 2, 1);
    image_clip_graphics.drawImage(grid_image, 0, 0, render::LEFT | render::TOP);
    for (int32_t y = 0; y < image_clipped.height; ++y) {
        for (int32_t x = 0; x < image_clipped.width; ++x) {
            uint32_t expected = kBlack;
            if (x == 1 && y == 1) expected = 0xFFFF00FFu;
            if (x == 2 && y == 1) expected = 0xFFFFFFFFu;
            assert(image_clipped.row(y)[x] == expected);
        }
    }

    Font *small_bold = Font::getFont(0, 1, 8);
    int32_t text_width = small_bold->stringWidth(std::string("Hi"));
    int32_t text_height = small_bold->getHeight();
    render::Surface top_left(text_width, text_height);
    render::Surface bottom_right(text_width, text_height);
    render::Surface centered(text_width, text_height);
    render::Surface baseline(text_width, text_height);
    render::SoftGraphics top_left_graphics(&top_left);
    render::SoftGraphics bottom_right_graphics(&bottom_right);
    render::SoftGraphics centered_graphics(&centered);
    render::SoftGraphics baseline_graphics(&baseline);
    for (render::SoftGraphics *graphics : {&top_left_graphics, &bottom_right_graphics,
                                          &centered_graphics, &baseline_graphics}) {
        graphics->setColor(0xFFFFFF);
        graphics->setFont(small_bold);
    }
    top_left_graphics.drawString(std::string("Hi"), 0, 0, render::LEFT | render::TOP);
    bottom_right_graphics.drawString(std::string("Hi"), text_width, text_height,
                                     render::RIGHT | render::BOTTOM);
    centered_graphics.drawString(std::string("Hi"), text_width / 2, text_height / 2,
                                 render::HCENTER | render::VCENTER);
    baseline_graphics.drawString(std::string("Hi"), 0, 10,
                                 render::LEFT | render::BASELINE);
    assert(top_left.pixels == bottom_right.pixels);
    assert(top_left.pixels == centered.pixels);
    assert(top_left.pixels == baseline.pixels);

    return 0;
}
