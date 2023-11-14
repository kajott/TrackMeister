// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace FontData {
    struct Box { float x0, y0, x1, y1; };

    struct Glyph {
        uint32_t codepoint;
        float advance;
        bool space;
        Box pos;
        Box tc;
    };

    extern const int TexWidth;
    extern const int TexHeight;
    extern const int TexDataSize;
    extern const uint8_t TexData[];

    extern const float Baseline;
    extern const int NumGlyphs;
    extern const int FallbackGlyphIndex;

    extern const Glyph GlyphData[];
};
