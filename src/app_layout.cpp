// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstring>
#include <cmath>

#include <algorithm>

#include <libopenmpt/libopenmpt.hpp>

#include "util.h"
#include "config.h"
#include "renderer.h"
#include "textarea.h"
#include "app.h"

constexpr float FontBaselinePos  = 0.820f;  //!< relative vertical position of the font's baseline
constexpr float FontNumberHeight = 0.595f;  //!< relative height of a number in the font
constexpr float FontNumberUpperPos = FontBaselinePos - FontNumberHeight;  //!< relative vertical position of the top end of a number

int Application::textWidth(int size, const char* text) const {
    return int(std::ceil(m_renderer.textWidth(text) * float(size)));
}

int Application::toPixels(int value) const {
    return int(m_screenSizeY * float(value) * .001f + .5f);
}

void Application::updateLayout(bool resetBoxVisibility) {
    m_screenSizeX = m_renderer.viewportWidth();
    m_screenSizeY = m_renderer.viewportHeight();

    // set UI element visibility flags
    if (resetBoxVisibility) {
        m_infoVisible  = m_config.infoEnabled && infoValid();
        m_metaVisible  = m_config.metaEnabled && metaValid();
        m_namesVisible = m_config.channelNamesEnabled && namesValid();
        m_vuVisible    = m_config.vuEnabled;
    } else {
        m_infoVisible  = m_infoVisible  && infoValid();
        m_metaVisible  = m_metaVisible  && metaValid();
        m_namesVisible = m_namesVisible && namesValid();
    }

    // set up "no module loaded" screen geometry
    m_emptyTextSize = toPixels(m_config.emptyTextSize);

    // set up info box geometry
    if (!m_infoVisible) {
        // no info box at all
        m_infoEndY = m_infoShadowEndY = m_progSize = 0;
    } else {
        // layout info box
        m_infoTextSize = toPixels(m_config.infoTextSize);
        m_infoDetailsSize = toPixels(m_config.infoDetailsTextSize);
        m_infoKeyX = toPixels(m_config.infoMarginX);
        m_trackX = float(m_infoKeyX);
        m_infoValueX = 0;
        int lineSpacing = toPixels(m_config.infoLineSpacing);
        m_infoEndY = toPixels(m_config.infoMarginY) - lineSpacing;
        if (!m_filename.empty()) {
            m_infoEndY += lineSpacing;
            m_infoFilenameY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, toPixels(m_config.infoKeyPaddingX) + textWidth(m_infoTextSize, "File:"));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_artist.empty()) {
            m_infoEndY += lineSpacing;
            m_infoArtistY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, toPixels(m_config.infoKeyPaddingX) + textWidth(m_infoTextSize, "Artist:"));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_title.empty()) {
            m_infoEndY += lineSpacing;
            m_infoTitleY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, toPixels(m_config.infoKeyPaddingX) + textWidth(m_infoTextSize, "Title:"));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_details.empty()) {
            m_infoEndY += lineSpacing;
            m_infoDetailsY = m_infoEndY;
            m_infoEndY += m_infoDetailsSize;
        }
        if (m_config.progressEnabled) {
            // compute relative Y geometry
            m_progSize = toPixels(m_config.progressHeight);
            m_progOuterDXY = m_config.progressBorderSize ? std::max(1, toPixels(m_config.progressBorderSize)) : 0;
            m_progInnerDXY = m_progOuterDXY + (m_config.progressBorderPadding ? std::max(1, toPixels(m_config.progressBorderPadding)) : 0);
            // make the bar big enough to not disappear if the borders are too fat
            m_progSize = std::max(m_progSize, (m_progInnerDXY << 1) + 1);
            // set the absolute Y geometry
            m_infoEndY += lineSpacing + toPixels(m_config.progressMarginTop);
            m_progY0 = m_infoEndY;
            m_infoEndY += m_progSize;
            m_progY1 = m_infoEndY;
            // X geometry is computed later
        } else { m_progSize = 0; }
        m_infoEndY += lineSpacing;
        m_trackTextSize = toPixels(m_config.infoTrackTextSize);
        // for the track number, geometry is quite complicated; we try to align
        // the top of the number with the top of the other text, requiring some
        // font metrics and a bit of math
        m_trackY = float(toPixels(m_config.infoMarginY))         // indicated top position
                 + float(m_infoTextSize)  * FontNumberUpperPos   // where the top of the normal lines ends up at
                 - float(m_trackTextSize) * FontNumberUpperPos;  // where we need to put the top
        if (trackValid()) {
            m_infoKeyX += textWidth(m_trackTextSize, m_track) + toPixels(m_config.infoTrackPaddingX);
            // for the bottom end of the track number, do some font metrics magic again
            int trackBottom = int(m_trackY                     // upper edge of text box
                + float(m_trackTextSize) * FontBaselinePos     // end of track number text
                + (m_config.progressEnabled ? 0.0f : (float(m_infoDetailsSize) * (1.0f - FontBaselinePos)))  // extra margin for visual equalization
                + 0.5f  /* rounding */ );
            m_infoEndY = std::max(m_infoEndY, trackBottom);
        }
        m_infoValueX += m_infoKeyX;
        m_infoEndY += toPixels(m_config.infoMarginY);
        m_infoShadowEndY = m_infoEndY + m_config.infoShadowSize;
    }

    // set up metadata box geometry
    if (!m_metaVisible) {
        m_metaStartX = m_metaShadowStartX = m_screenSizeX;
    } else {
        // fix up sizes first
        float textSize = float(toPixels(m_config.metaTextSize));
        float gapHeight = float(toPixels(m_config.metaSectionMargin));
        m_metadata.defaultSize = textSize;
        for (auto& line : m_metadata.lines) {
            line->size = textSize;
            if (line->marginTop > 0.f) { line->marginTop = gapHeight; }
        }
        // then, get dimensions of the bar
        int margin = toPixels(m_config.metaMarginX);
        m_metaStartX = m_screenSizeX - int(std::ceil(m_metadata.width())) - 2 * margin;
        m_metaTextX = float(m_metaStartX + margin);
        m_metaShadowStartX = m_metaStartX - toPixels(m_config.metaShadowSize);
        m_metaTextMinY = float(toPixels(m_config.metaMarginY));
        m_metaTextMaxY = std::min(m_metaTextMinY, float(m_screenSizeY - margin) - m_metadata.height());
        Dprintf("metadata box scroll range: %.1f ... %.1f\n", m_metaTextMinY, m_metaTextMaxY);
        m_metaTextTargetY = m_metaTextMinY;
        if (resetBoxVisibility) { m_metaTextY = m_metaTextTargetY; }
    }

    // set up progress bar geometry
    if (m_infoVisible && m_config.progressEnabled) {
        m_progSize = m_progY1 - m_progY0;
        m_progX0 = m_infoKeyX;
        m_progX1 = m_metaStartX - toPixels(m_config.infoMarginX);
        int innerRadius = m_progSize - (m_progInnerDXY << 1);
        m_progPosX0 = m_progX0 + m_progInnerDXY + innerRadius;
        m_progPosDX = m_progX1 - m_progInnerDXY - m_progPosX0;
    }

    // set up pattern display geometry -- step 1: define possible formats
    struct PDFormat { const char *posFormat, *channelFormat, *sep; };
    static const PDFormat pdFormats[] = {
        { "",            "G#0",           "W"  },
        { "000",         "G#0",           "W"  },
        { "000",         "G#0 00",        "W"  },
        { "000",         "G#0 00",        "WW" },
        { "000:000",     "G#0 00",        "W"  },
        { "000:000",     "G#0 00",        "WW" },
        { "000:000",     "G#0 00v00",     "W"  },
        { "000:000",     "G#0 00v00",     "WW" },
        { "000:000.000", "G#0 00v00",     "W"  },
        { "000:000.000", "G#0 00v00",     "WW" },
        { "000:000.000", "G#0 00v00 C00", "W"  },
        { "000:000.000", "G#0 00v00 C00", "WW" },
        { nullptr, nullptr, nullptr }
    };

    // set up pattern display geometry -- step 2: the actual formatting function
    auto doFormat = [&] (const PDFormat &fmt) -> int {
        m_pdPosChars = int(strlen(fmt.posFormat));
        m_pdChannelChars = int(strlen(fmt.channelFormat));
        int gapWidth = textWidth(m_pdTextSize, fmt.sep);
        m_pdNoteWidth = textWidth(m_pdTextSize, "G#0");
        m_pdTextY0 = (m_infoEndY + m_screenSizeY - m_pdTextSize) >> 1;
        m_pdTextDY = m_pdTextSize + toPixels(m_config.patternLineSpacing);
        m_pdRows = (m_pdTextY0 - m_infoEndY + m_pdTextSize - 1) / m_pdTextSize;
        m_pdChannelX0 = m_pdPosChars ? (textWidth(m_pdTextSize, fmt.posFormat) + gapWidth) : 0;
        m_pdChannelWidth = textWidth(m_pdTextSize, fmt.channelFormat);
        m_pdChannelDX = m_pdChannelWidth + gapWidth;
        m_pdPipeDX = 0.5f * float(m_pdTextSize) * (m_renderer.textWidth(fmt.sep) + m_renderer.textWidth("|"));
        m_pdBarRadius = (m_pdTextSize * m_config.patternBarBorderPercent) / 100;
        return m_pdChannelX0 + m_numChannels * m_pdChannelDX - gapWidth;
    };

    // set up pattern display geometry -- step 3: try various formats to fit width
    m_pdTextSize = toPixels(m_config.patternTextSize);
    int pdMaxWidth = m_metaStartX - 2 * toPixels(m_config.patternMarginX);
    // try most compact format first
    int pdWidth = doFormat(pdFormats[0]);
    if (pdWidth > pdMaxWidth) {
        // if the most compact format doesn't fit, we need to shrink the text size
        int minSize = toPixels(m_config.patternMinTextSize);
        // make a first guess; shrink later if neccessary
        m_pdTextSize = std::max(minSize, m_pdTextSize * pdMaxWidth / pdWidth);
        for (;;) {
            pdWidth = doFormat(pdFormats[0]);
            if ((pdWidth <= pdMaxWidth) || (m_pdTextSize <= minSize)) { break; }
            m_pdTextSize--;
        }
    } else {
        // most compact format fits -> try successively less compact formats
        // until it stops fitting
        const PDFormat *format = pdFormats;
        while (format[1].channelFormat) {
            pdWidth = doFormat(format[1]);
            if (pdWidth > pdMaxWidth) { break; }
            ++format;
        }
        // use the least compact format that did fit
        if (pdWidth > pdMaxWidth) { pdWidth = doFormat(format[0]); }
    }

    // set up pattern display geometry -- step 4: center
    int pdXoffset = (m_metaStartX - pdWidth) >> 1;
    m_pdPosX = pdXoffset;
    m_pdChannelX0 += pdXoffset;
    m_pdBarStartX = pdXoffset - toPixels(m_config.patternBarPaddingX);
    m_pdBarEndX = pdXoffset + pdWidth + toPixels(m_config.patternBarPaddingX);

    // set up channel name and VU meter geometry
    int cnGap = toPixels(m_config.channelNamePaddingY);
    m_channelNameTextY = m_screenSizeY - m_pdTextSize - cnGap;
    m_channelNameBarStartY = m_channelNameTextY - cnGap;
    m_channelNameOffsetX = float(m_pdChannelWidth) * 0.5f;
    m_vuHeight = float(toPixels(m_config.vuHeight));

    // set up toast geometry
    m_toastTextSize = toPixels(m_config.toastTextSize);
    m_toastY = toPixels(m_config.toastPositionY);
    m_toastDY = ((m_toastTextSize + 1) >> 1) + toPixels(m_config.toastMarginY);
    m_toastDX = m_toastDY + toPixels(m_config.toastMarginX);

    // done!
    #if USE_PATTERN_CACHE
        m_patternCache.clear();
    #endif
    Dprintf("updateLayout(): pdTextSize=%d pdRows=%d\n", m_pdTextSize, m_pdRows);
}

void Application::formatPosition(int order, int pattern, int row, char* text, char* attr, int size) {
    auto makeNum = [&] (int num, char a, char sep) {
        if  (size >= 3) { *text++ = char((num / 100) % 10) + '0';  *attr++ = a;  --size; }
        if  (size >= 2) { *text++ = char((num /  10) % 10) + '0';  *attr++ = a;  --size; }
        if  (size >= 1) { *text++ = char( num        % 10) + '0';  *attr++ = a;  --size; }
        if ((size >= 1) && sep) { *text++ = sep;                 *attr++ = ':';  --size; }
    };
    if (size >= 7) { makeNum(order,   'O', ':'); }
    if (size >= 7) { makeNum(pattern, 'P', '.'); }
                     makeNum(row,     'R', 0);
    while (size > 0) { *text++ = ' ';  *attr++ = ' ';  --size; }
    *text = '\0';
    *attr = '\0';
}
