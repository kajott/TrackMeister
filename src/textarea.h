// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <string>

#include "renderer.h"

struct TextSpan {
    uint32_t color;
    std::string text;
    explicit TextSpan(uint32_t color_, const char* text_) : color(color_), text(text_) {}
};

struct TextLine {
    float size;
    float marginTop;
    float marginBottom;
    uint32_t defaultColor;
    std::vector<TextSpan> spans;

    float width() const;
    void draw(float x, float y);

    void addSpan(uint32_t color, const char* text);
    inline void addSpan(const char* text) { addSpan(defaultColor, text); }
    inline void addSpan(uint32_t color, const std::string& text) { addSpan(color, text.c_str()); }
    inline void addSpan(const std::string& text) { addSpan(defaultColor, text.c_str()); }

private:
    friend struct TextArea;
    TextArea* parent;
    explicit inline TextLine(TextArea& parent_, float size_, uint32_t defaultColor_, float marginTop_=0.f, float marginBottom_=0.f)
        : parent(&parent_), size(size_), defaultColor(defaultColor_), marginTop(marginTop_), marginBottom(marginBottom_) {}
};

struct TextArea {
    float defaultSize;
    uint32_t defaultColor;
    std::vector<TextLine*> lines;
    TextBoxRenderer& renderer;

    float width() const;
    float height() const;
    void draw(float x, float y);
    inline bool empty() const { return lines.empty(); }

    TextLine& addLine(float size, uint32_t lineDefaultColor, const char* initialText=nullptr);
    inline TextLine& addLine(float size) { return addLine(size, defaultColor, nullptr); }
    inline TextLine& addLine() { return addLine(defaultSize, defaultColor, nullptr); }
    inline TextLine& addLine(const char* text) { return addLine(defaultSize, defaultColor, text); }
    inline TextLine& addLine(uint32_t color, const char* text) { return addLine(defaultSize, color, text); }
    inline TextLine& addLine(const std::string& text) { return addLine(defaultSize, defaultColor, text.c_str()); }
    inline TextLine& addLine(uint32_t color, const std::string& text) { return addLine(defaultSize, color, text.c_str()); }
    inline TextLine& addLine(float size, uint32_t color, const std::string& text) { return addLine(size, color, text.c_str()); }

    void addSpan(uint32_t color, const char* text);
    inline void addSpan(const char* text) { addSpan(defaultColor, text); }
    inline void addSpan(uint32_t color, const std::string& text) { addSpan(color, text.c_str()); }
    inline void addSpan(const std::string& text) { addSpan(defaultColor, text.c_str()); }

    void addWrappedLine(float maxWidth, float size, uint32_t color, const char* text);
    inline void addWrappedLine(float maxWidth, const char* text) { addWrappedLine(maxWidth, defaultSize, defaultColor, text); }
    inline void addWrappedLine(float maxWidth, float size, uint32_t color, const std::string& text) { addWrappedLine(maxWidth, size, color, text.c_str()); }
    inline void addWrappedLine(float maxWidth, const std::string& text) { addWrappedLine(maxWidth, defaultSize, defaultColor, text.c_str()); }

    void ingest(TextArea& source);

    void clear();

    explicit inline TextArea(TextBoxRenderer& renderer_, float defaultSize_=16.0f, uint32_t defaultColor_=0xFFFFFFFFu)
        : renderer(renderer_), defaultSize(defaultSize_), defaultColor(defaultColor_) {}
    inline ~TextArea() { clear(); }
};
