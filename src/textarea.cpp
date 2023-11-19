// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cctype>

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
        w += parent->renderer.textWidth(span.text.c_str()) * size;
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
        x = parent->renderer.text(x, y, size, span.text.c_str(), 0u, span.color);
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

void TextArea::ingest(TextArea& source) {
    for (auto& line : source.lines) {
        line->parent = this;
    }
    lines.insert(lines.end(), source.lines.begin(), source.lines.end());
    source.lines.clear();
}

void TextArea::addWrappedLine(float maxWidth, float size, uint32_t color, const char* text) {
    if (!text) { return; }
    std::string s;
    do {
        const char* pos = text;
        const char* safeEnd = nullptr;
        do {
            // search for next place where we could split the line
            while (*pos && !std::isspace(*pos) && (*pos != '-') && (*pos != '/')) { ++pos; }
            // trim trailing whitespace (but keep dash or slash!)
            const char *end = pos;
            while (*end && (end > text) && std::isspace(end[-1])) { --end; }
            if (*end && ((*end == '-') || (*end == '/'))) { ++end; }
            // check for valid width
            s.assign(text, (end - text));
            float width = renderer.textWidth(s.c_str()) * size;
            if (!safeEnd || (width <= maxWidth)) { safeEnd = end; }
            if (width > maxWidth) { break; }
            // continue
            if (*pos) { ++pos; }
        } while (*pos);
        // add the relevant part of the line
        s.assign(text, (safeEnd - text));
        addLine(size, color, s.c_str());
        // continue after the end of the line (skipping initial whitespace)
        text = safeEnd;
        while (*text && std::isspace(*text)) { ++text; }
    } while (*text);
}
