// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <string>
#include <algorithm>

#include "renderer.h"

#include "textarea.h"

void TextArea::clear() {
    for (auto* line : lines) {
        delete line;
    }
    lines.clear();
}

TextLine& TextArea::addLine(float size, uint32_t lineDefaultColor, const char* initialText) {
    auto* line = new TextLine(*this, size, lineDefaultColor);
    lines.emplace_back(line);
    if (initialText) {
        line->addSpan(lineDefaultColor, initialText);
    }
    return *line;
}

void TextArea::addSpan(uint32_t color, const char* text) {
    if (!text) { return; }
    if (lines.empty()) {
        addLine(defaultSize, color, text);
    } else {
        lines[lines.size() - 1u]->addSpan(color, text);
    }
}

void TextLine::addSpan(uint32_t color, const char* text) {
    if (!text) { return; }
    spans.emplace_back(color, text);
}

float TextLine::width() const {
    float w = 0.0f;
    for (const auto& span : spans) {
        w += parent.renderer.textWidth(span.text.c_str()) * size;
    }
    return w;
}

float TextArea::width() const {
    float w = 0.0f;
    for (const auto& line : lines) {
        w = std::max(w, line->width());
    }
    return w;
}

float TextArea::height() const {
    if (lines.empty()) { return 0.0f; }
    float h = -lines[0]->marginTop;
    for (const auto& line : lines) {
        h += line->marginTop + line->size + line->marginBottom;
    }
    return h - lines[lines.size() - 1u]->marginBottom;
}

void TextLine::draw(float x, float y) {
    for (auto& span : spans) {
        x = parent.renderer.text(x, y, size, span.text.c_str(), 0u, span.color);
    }
}

void TextArea::draw(float x, float y) {
    if (lines.empty()) { return; }
    y -= lines[0]->marginTop;
    for (const auto& line : lines) {
        y += line->marginTop;
        line->draw(x, y);
        y += line->size + line->marginBottom;
    }
}
