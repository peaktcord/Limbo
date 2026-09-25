#ifndef COMMON_UI_HPP
#define COMMON_UI_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/common/font_metrics.hpp"
#include "src/common/runtime.hpp"
#include "src/common/session.hpp"
#include "src/storage/resource_store.hpp"
#include "src/text/text.hpp"

class Font {
    friend class SessionArena;
    int32_t face_, style_, size_;
    Font(int32_t f, int32_t st, int32_t sz) : face_(f), style_(st), size_(sz) {}
public:
    static Font *getFont(int32_t face, int32_t style, int32_t size) {
        auto &cache = currentSession().font_cache;
        for (void *entry : cache) {
            Font *f = static_cast<Font *>(entry);
            if (f->face_ == face && f->style_ == style && f->size_ == size) return f;
        }
        Font *f = session_new<Font>(face, style, size);
        cache.push_back(f);
        return f;
    }
    int32_t getSize() const { return size_; }
    int32_t getStyle() const { return style_; }
    int32_t getFace() const { return face_; }
    int32_t getHeight() const { return size_ == 8 ? 12 : (size_ == 16 ? 17 : 13); }
    const uint8_t *advances() const {
        if (size_ == 16) {
            return (style_ & 2) ? s60metrics::kLargeItalicAdvance
                                : s60metrics::kLargeBoldAdvance;
        }
        if (size_ == 0) {
            return (style_ & 1) ? s60metrics::kMediumBoldAdvance
                                : s60metrics::kMediumPlainAdvance;
        }
        return (style_ & 1) ? s60metrics::kSmallBoldAdvance
                            : s60metrics::kSmallPlainAdvance;
    }
    int32_t charWidth(uint16_t ch) const {
        uint32_t code = ch;
        if (code < s60metrics::kFirstCode || code > s60metrics::kLastCode) code = '?';
        return advances()[code - s60metrics::kFirstCode];
    }
    int32_t stringWidth(const std::string &s) const {
        return substringWidth(s, 0, voyage::text::length(s));
    }
    int32_t substringWidth(const std::string &s, int32_t off, int32_t len) const {
        int32_t width = 0;
        for (int32_t n = 0; n < len; ++n) {
            int32_t at = off + n;
            if (at < 0 || at >= voyage::text::length(s)) break;
            width += charWidth(voyage::text::byte_at(s, at));
        }
        return width;
    }
};

class Image {
    friend class SessionArena;
    int32_t w_ = 0, h_ = 0;
    std::vector<uint8_t> data_;
    Image() {}
    void parsePngSize() {
        if (data_.size() >= 24 && data_[1] == 'P' && data_[2] == 'N' && data_[3] == 'G') {
            w_ = (data_[16] << 24) | (data_[17] << 16) | (data_[18] << 8) | data_[19];
            h_ = (data_[20] << 24) | (data_[21] << 16) | (data_[22] << 8) | data_[23];
        }
    }
public:
    static Image *createImage(const std::string &resource);
    static Image *createImage(const SharedArray<int8_t> &data, int32_t off, int32_t len) {
        Image *img = session_new<Image>();
        img->data_.resize((size_t)len);
        for (int32_t i = 0; i < len; ++i) img->data_[(size_t)i] = (uint8_t)data[off + i];
        img->parsePngSize();
        return img;
    }
    static Image *createImage(int32_t w, int32_t h) {
        Image *img = session_new<Image>();
        img->w_ = w;
        img->h_ = h;
        return img;
    }
    int32_t getWidth() const { return w_; }
    int32_t getHeight() const { return h_; }
    class Graphics *getGraphics();
    const std::vector<uint8_t> &encodedBytes() const { return data_; }

    void *backend() const { return backend_; }
    void setBackend(void *p) { backend_ = p; }

private:
    void *backend_ = nullptr;
};

class Graphics {
protected:
    int32_t color_ = 0;
    Font *font_ = Font::getFont(0, 0, 0);
    int32_t clipX_ = 0, clipY_ = 0, clipW_ = 0x7FFFFFFF, clipH_ = 0x7FFFFFFF;
public:
    virtual ~Graphics() = default;
    void setColor(int32_t c) { color_ = c; }
    int32_t getColor() const { return color_; }
    void setFont(Font *f) { font_ = f; }
    Font *getFont() const { return font_; }
    void setClip(int32_t x, int32_t y, int32_t w, int32_t h) {
        clipX_ = x; clipY_ = y; clipW_ = w; clipH_ = h;
        onClip();
    }
    void clipRect(int32_t x, int32_t y, int32_t w, int32_t h) {
        int32_t x0 = clipX_ > x ? clipX_ : x;
        int32_t y0 = clipY_ > y ? clipY_ : y;
        int32_t x1 = clipX_ + clipW_ < x + w ? clipX_ + clipW_ : x + w;
        int32_t y1 = clipY_ + clipH_ < y + h ? clipY_ + clipH_ : y + h;
        clipX_ = x0;
        clipY_ = y0;
        clipW_ = x1 > x0 ? x1 - x0 : 0;
        clipH_ = y1 > y0 ? y1 - y0 : 0;
        onClip();
    }
    virtual void onClip() {}
    virtual void fillRect(int32_t, int32_t, int32_t, int32_t) {}
    virtual void drawRect(int32_t, int32_t, int32_t, int32_t) {}
    virtual void fillRoundRect(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t) {}
    virtual void drawLine(int32_t, int32_t, int32_t, int32_t) {}
    virtual void drawString(const std::string &, int32_t, int32_t, int32_t) {}
    virtual void drawChar(uint16_t, int32_t, int32_t, int32_t) {}
    enum Manipulation : int32_t {
        TRANS_NONE = 0,
        TRANS_MIRROR = 2,
        IMAGE_FLIP_HORIZONTAL = 2
    };

    virtual void drawImage(Image *, int32_t, int32_t, int32_t, int32_t = 0) {}
};

class PaintBackend {
public:
    virtual ~PaintBackend() = default;
    virtual Graphics *screenGraphics() = 0;
    virtual Graphics *imageGraphics(Image *image) = 0;
    virtual void beginPaint() {}
    virtual void endPaint() {}
};

inline Graphics *Image::getGraphics() {
    if (PaintBackend *paint = currentSession().paint) return paint->imageGraphics(this);
    return session_new<Graphics>();
}

inline Image *Image::createImage(const std::string &resource) {
    auto bytes = voyage::storage::read_resource(currentSession().resources, resource);
    if (!bytes) {
        const std::string message = "Image resource not found: " + resource;
        voyage::logging::trace_throw(message.c_str());
        throw std::runtime_error(message);
    }
    Image *img = session_new<Image>();
    img->data_ = std::move(*bytes);
    img->parsePngSize();
    return img;
}

class Displayable {
public:
    virtual ~Displayable() = default;
};

class Canvas : public Displayable {
    bool dirty_ = false;
public:
    virtual void paint(Graphics *g) = 0;
    virtual void keyPressed(int32_t) {}
    virtual void keyReleased(int32_t) {}
    virtual void showNotify() {}
    virtual void hideNotify() {}
    void setFullScreenMode(bool) {}
    int32_t getWidth() const { return currentSession().canvas_width; }
    int32_t getHeight() const { return currentSession().canvas_height; }
    int32_t getGameAction(int32_t keyCode) const {
        switch (keyCode) {
            case -1: return 1;
            case -2: return 6;
            case -3: return 2;
            case -4: return 5;
            case -5: return 8;
            case -11: return 11;
            case -12: return 12;
            case -13: return 13;
            case -14: return 14;
            case 50: return 1;
            case 56: return 6;
            case 52: return 2;
            case 54: return 5;
            case 53: return 8;
        }
        return 0;
    }
    void repaint() { dirty_ = true; }
    void serviceRepaints();
};

class Display {
public:
    static Display *getDisplay() {
        static Display d;
        return &d;
    }
    Displayable *getCurrent() const { return currentSession().current_display; }
    void setCurrent(Displayable *d) {
        Displayable *previous = currentSession().current_display;
        bool changed = previous != d;
        if (!changed) return;
        if (Canvas *c = dynamic_cast<Canvas *>(previous)) c->hideNotify();
        currentSession().current_display = d;
        if (Canvas *c = dynamic_cast<Canvas *>(d)) c->showNotify();
    }
};

inline void Canvas::serviceRepaints() {
    if (!dirty_) return;
    if (Display::getDisplay()->getCurrent() != this) return;
    dirty_ = false;
    if (PaintBackend *paint = currentSession().paint) {
        paint->beginPaint();
        try {
            this->paint(paint->screenGraphics());
        } catch (...) {
            paint->endPaint();
            throw;
        }
        paint->endPaint();
    } else {
        static Graphics headless;
        this->paint(&headless);
    }
}

#endif
