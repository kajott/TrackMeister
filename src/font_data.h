// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace FontData {
    struct Box { float x0, y0, x1, y1; };

    //! glyph descriptor
    struct Glyph {
        uint32_t codepoint;
        float advance;
        bool space;
        Box pos;
        Box tc;
    };

    //! font descriptor
    struct Font {
        const char *name;     //!< font name (nullptr = end of font list)
        int bitmapHeight;     //!< 0: scalable MSDF font; >0: bitmap font with defined height
        float baseline;       //!< baseline position relative to glyph cell height
        const Glyph *glyphs;  //!< glyph table
        int numGlyphs;        //!< number of entries in the glyph table
        int fallbackIndex;    //!< index of the fallback glyph (typically U+FFFD) in the glyph table
    };

    //! font registry
    extern const Font Fonts[];

    extern const int TexDataSize;
    extern const uint8_t TexData[];
};
