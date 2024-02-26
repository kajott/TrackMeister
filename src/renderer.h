// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <algorithm>

#include "font_data.h"

//! text alignment constants
namespace Align {
    constexpr uint8_t Left     = 0x00;  //!< horizontally left-aligned
    constexpr uint8_t Center   = 0x01;  //!< horizontally centered
    constexpr uint8_t Right    = 0x02;  //!< horizontally right-aligned
    constexpr uint8_t Top      = 0x00;  //!< vertically top-aligned (text is *below* indicated point)
    constexpr uint8_t Middle   = 0x10;  //!< vertically centered
    constexpr uint8_t Bottom   = 0x20;  //!< vertically bottom-aligned (text is *above* indicated point)
    constexpr uint8_t Baseline = 0x30;  //!< vertically baseline-aligned (indicated point is at text baseline)
    constexpr uint8_t HMask    = 0x0F;  //!< \private horizontal alignment mask
    constexpr uint8_t VMask    = 0xF0;  //!< \private vertical alignment mask
};

//! a renderer that can draw two things: MSDF text, or rounded boxes
class TextBoxRenderer {
    const char* m_error = nullptr;
    int m_vpWidth, m_vpHeight;
    float m_vpScaleX, m_vpScaleY;
    unsigned m_vao;
    unsigned m_sampler;
    unsigned m_vbo;
    unsigned m_ibo;
    unsigned m_prog;
    unsigned m_tex;
    unsigned m_fontTex;
    const FontData::Font *m_currentFont;
    int m_quadCount;

    struct Vertex {
        float pos[2];    // screen position (already transformed into NDC)
        float tc[2];     // texture coordinate | half-size coordinate (goes from -x/2 to x/2, with x=width or x=height)
        float size[3];   // not used | xy = half size, z = border radius
        float br[2];     // blend range: x = distance to outline (in pixels) that corresponds to middle gray, y = reciprocal of range
        uint32_t color;  // color to draw in
        uint32_t mode;   // 0 = box, 1 = text
    };

    Vertex* m_vertices;

    Vertex* newVertices();
    Vertex* newVertices(uint8_t mode, float x0, float y0, float x1, float y1);
    Vertex* newVertices(uint8_t mode, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);

    const FontData::Glyph* getGlyph(uint32_t codepoint) const;
    void alignText(float &x, float &y, float size, const char* text, uint8_t align);

    inline void useTexture(unsigned texID)
        { if (texID && (texID != m_tex)) { flush(); m_tex = texID; } }

public:
    bool init();
    void shutdown();
    void viewportChanged();
    void flush();

    inline const char* error()  const { return m_error; }
    inline int viewportWidth()  const { return m_vpWidth; }
    inline int viewportHeight() const { return m_vpHeight; }

    struct TextureDimensions { int width, height; };
    static unsigned loadTexture(const void* pngData, size_t pngSize, int channels, bool mipmap, TextureDimensions* dims=nullptr);
    static unsigned loadTexture(const char* filename, int channels, bool mipmap, TextureDimensions* dims=nullptr);
    static void freeTexture(unsigned &texID);

    void box(int x0, int y0, int x1, int y1,
             uint32_t colorUpperLeft, uint32_t colorLowerRight,
             bool horizontalGradient=false, int borderRadius=0,
             float blur=1.0f, float offset=0.0f);
    inline void box(int x0, int y0, int x1, int y1, uint32_t color)
        { box(x0, y0, x1, y1, color, color); }

    void logo(int x0, int y0, int x1, int y1, uint32_t color, unsigned texID);

    void outlineBox(int x0, int y0, int x1, int y1,
                    uint32_t colorUpper, uint32_t colorLower,  // all colors are forced to fully opaque!
                    uint32_t colorOutline=0xFFFFFFFF,
                    int outlineWidth=0,  // positive: outline *outside* the box coords, negative: outline *inside* the box coords
                    int borderRadius=0,
                    int shadowOffset=0, float shadowBlur=0.0f, float shadowAlpha=1.0f, int shadowGrow=0);

    inline void circle(int x, int y, int r, uint32_t color, float blur=1.0f, float offset=0.0f)
        { box(x - r, y - r, x + r, y + r, color, color, false, r, blur, offset); }

    const char* setFont(const char* name);

    int textSizeGranularity() const;
    float textBaseline() const;
    float textNumberHeight() const;
    float textWidth(const char* text) const;
    float text(float x, float y, float size, const char* text,
              uint8_t align,
              uint32_t colorUpper, uint32_t colorLower,
              float blur=1.0f, float offset=0.0f);
    inline float text(float x, float y, float size, const char* text,
              uint8_t align = Align::Left + Align::Top,
              uint32_t color=0xFFFFFFFF)
              { return this->text(x, y, size, text, align, color, color); }

    float outlineText(float x, float y, float size, const char* text,
                     uint8_t align = Align::Left + Align::Top,
                     uint32_t colorUpper=0xFFFFFFFF, uint32_t colorLower=0xFFFFFFFF,
                     uint32_t colorOutline=0xFF000000, float outlineWidth=0.0f,
                     int shadowOffset=0, float shadowBlur=0.0f, float shadowAlpha=1.0f, float shadowGrow=0.0f);
    inline float shadowText(float x, float y, float size, const char* text,
                           uint8_t align = Align::Left + Align::Top,
                           uint32_t colorUpper=0xFFFFFFFF, uint32_t colorLower=0xFFFFFFFF,
                           int shadowOffset=0, float shadowBlur=0.0f, float shadowAlpha=1.0f, float shadowGrow=0.0f)
        { return outlineText(x, y, size, text, align, colorUpper, colorLower, 0, 0.0f, shadowOffset, shadowBlur, shadowAlpha, shadowGrow); }

    int control(int x, int y, int size, uint8_t vAlign, bool keyboard,
                const char* control, const char* label=nullptr,
                uint32_t textColor=0xFFFFFFFF, uint32_t backgroundColor=0xFF000000);

    // helper functions
    static uint32_t nextCodepoint(const char* &utf8string);
    static inline uint32_t makeAlpha(float alpha)
        { return uint32_t(std::min(1.f, std::max(0.f, alpha)) * 255.f + .5f) << 24; }
    static inline uint32_t extraAlpha(uint32_t color, float alpha)
        { return (color & 0x00FFFFFFu) | (uint32_t(std::min(1.f, std::max(0.f, alpha)) * float(color >> 24) + .5f) << 24); }
};
