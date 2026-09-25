#ifndef COMMON_RENDER_HPP
#define COMMON_RENDER_HPP

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/ui.hpp"

namespace render {

struct Surface {
    int32_t width = 0;
    int32_t height = 0;
    std::vector<uint32_t> pixels;

    Surface() {}
    Surface(int32_t w, int32_t h) : width(w), height(h), pixels((size_t)(w * h), 0xFF000000u) {}
    bool empty() const { return width <= 0 || height <= 0; }
    uint32_t *row(int32_t y) { return pixels.data() + (size_t)y * (size_t)width; }
    const uint32_t *row(int32_t y) const {
        return pixels.data() + (size_t)y * (size_t)width;
    }
};

enum Anchor {
    HCENTER = 1,
    VCENTER = 2,
    LEFT = 4,
    RIGHT = 8,
    TOP = 16,
    BOTTOM = 32,
    BASELINE = 64
};

int32_t glyphAdvance(Font *font);
int32_t lineHeight(Font *font);

struct S60Face;

class SoftGraphics : public Graphics {
    Surface *target_;
    int32_t cx_ = 0, cy_ = 0, cw_ = 0, ch_ = 0;
    void blendPixel(int32_t x, int32_t y, uint32_t argb);
    void fillSpan(int32_t x, int32_t y, int32_t w, uint32_t argb);
    int32_t drawGlyph(const S60Face *face, uint16_t ch, int32_t x, int32_t top, uint32_t argb);

public:
    explicit SoftGraphics(Surface *target);
    void bind(Surface *target);
    Surface *target() const { return target_; }
    void resetClip();

    void onClip() override;
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h) override;
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t aw, int32_t ah) override;
    void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2) override;
    void drawString(const std::string &text, int32_t x, int32_t y, int32_t anchor) override;
    void drawChar(uint16_t ch, int32_t x, int32_t y, int32_t anchor) override;
    void drawImage(Image *image, int32_t x, int32_t y, int32_t anchor, int32_t manipulation = 0) override;
};

Surface *surfaceFor(Image *image);

class SoftRenderer final : public PaintBackend {
public:
    explicit SoftRenderer(Surface *screen);
    SoftRenderer(const SoftRenderer &) = delete;
    SoftRenderer &operator=(const SoftRenderer &) = delete;

    Surface *surface() const { return screen_.target(); }
    std::mutex &frameMutex() { return mutex_; }
    void forgetImages();

    Graphics *screenGraphics() override { return &screen_; }
    Graphics *imageGraphics(Image *image) override;
    void beginPaint() override;
    void endPaint() override;

private:
    SoftGraphics screen_;
    std::mutex mutex_;
    std::unordered_map<Image *, std::unique_ptr<SoftGraphics>> image_graphics_;
};

bool writePng(const Surface &surface, const std::string &path);

}

#endif
