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

int Application::textWidth(int size, const char* text) const {
    return int(std::ceil(m_renderer.textWidth(text) * float(size)));
}

int Application::toPixels(int value) const {
    return int(m_screenSizeY * float(value) * .001f + .5f);
}

int Application::toTextSize(int value) const {
    value = toPixels(value);
    int g = m_renderer.textSizeGranularity();
    if (g) { value -= value % g; }
    return value;
}

void Application::updateLayout(bool resetBoxVisibility) {
    m_screenSizeX = m_renderer.viewportWidth();
    m_screenSizeY = m_renderer.viewportHeight();
    m_renderer.setFont(m_config.font.c_str());

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
    m_emptyTextSize = toTextSize(m_config.emptyTextSize);
    m_emptyTextPos = toPixels(m_config.emptyTextPosY);

    // set up info box geometry
    if (!m_infoVisible) {
        // no info box at all
        m_infoEndY = m_infoShadowEndY = m_progSize = 0;
    } else {
        // layout info box
        m_infoTextSize = toTextSize(m_config.infoTextSize);
        m_infoDetailsSize = toTextSize(m_config.infoDetailsTextSize);
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
        if ((!m_shortDetails.empty() && !m_longDetails.empty()) || !m_details.empty()) {
            m_infoEndY += lineSpacing;
            m_infoDetailsY = m_infoEndY;
            m_infoEndY += m_infoDetailsSize;
        }
        if (m_config.progressEnabled && m_mod) {
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
        float baselinePos = m_renderer.textBaseline();
        float numUpperPos = baselinePos - m_renderer.textNumberHeight();
        m_trackY = float(toPixels(m_config.infoMarginY))  // indicated top position
                 + float(m_infoTextSize)  * numUpperPos   // where the top of the normal lines ends up at
                 - float(m_trackTextSize) * numUpperPos;  // where we need to put the top
        if (trackValid()) {
            m_infoKeyX += textWidth(m_trackTextSize, m_track) + toPixels(m_config.infoTrackPaddingX);
            // for the bottom end of the track number, do some font metrics magic again
            int trackBottom = int(m_trackY                // upper edge of text box
                + float(m_trackTextSize) * baselinePos    // end of track number text
                + (m_config.progressEnabled ? 0.0f : (float(m_infoDetailsSize) * (1.0f - baselinePos)))  // extra margin for visual equalization
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
        float textSize = float(toTextSize(m_config.metaTextSize));
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
        m_metaTextTargetY = m_metaTextMinY;
        if (resetBoxVisibility) { m_metaTextY = m_metaTextTargetY; }
    }

    // compile final details string
    int maxDetails = int(std::min(m_shortDetails.size(), m_longDetails.size()));
    if (maxDetails > 0) {
        int maxWidth = m_metaStartX - toPixels(m_config.infoMarginX) - m_infoKeyX;
        // try successively less long details
        for (int maxLong = maxDetails;  maxLong >= 0;  --maxLong) {
            m_details.clear();
            for (int i = 0;  i < maxDetails;  ++i) {
                const std::string& s = (i >= maxLong) ? m_shortDetails[i] : m_longDetails[i];
                if (!s.empty()) {
                    if (!m_details.empty()) { m_details.append(", "); }
                    m_details.append(s);
                }
            }
            // stop if it fits
            if (textWidth(m_infoDetailsSize, m_details.c_str()) <= maxWidth) { break; }
        }
    }

    // set up progress bar geometry
    if (m_infoVisible && m_config.progressEnabled) {
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
    m_pdTextSize = toTextSize(m_config.patternTextSize);
    int pdMaxWidth = m_metaStartX - 2 * toPixels(m_config.patternMarginX);
    // try most compact format first
    int pdWidth = doFormat(pdFormats[0]);
    if (pdWidth > pdMaxWidth) {
        // if the most compact format doesn't fit, we need to shrink the text size
        int step = std::max(m_renderer.textSizeGranularity(), 1);
        int minSize = std::max(toPixels(m_config.patternMinTextSize), step);
        // make a first guess; shrink later if neccessary
        m_pdTextSize = std::max(minSize, m_pdTextSize * pdMaxWidth / pdWidth);
        for (;;) {
            pdWidth = doFormat(pdFormats[0]);
            if ((pdWidth <= pdMaxWidth) || (m_pdTextSize <= minSize)) { break; }
            m_pdTextSize -= step;
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

    // set up background image geometry
    if (m_background.tex) {
        int bgWidth  = m_screenSizeX;
        int bgHeight = (bgWidth * m_background.size.height + (m_background.size.width >> 1)) / m_background.size.width;
        if (bgHeight < m_screenSizeY) {
            bgHeight = m_screenSizeY;
            bgWidth = (bgHeight * m_background.size.width + (m_background.size.height >> 1)) / m_background.size.height;
        }
        m_background.x0 = (m_screenSizeX - bgWidth  + 1) >> 1;
        m_background.y0 = (m_screenSizeY - bgHeight + 1) >> 1;
        m_background.x1 = m_background.x0 + bgWidth;
        m_background.y1 = m_background.y0 + bgHeight;
    }

    // set up logo geometry
    m_usedLogoTex = !m_config.logoEnabled ? 0 : m_logo.tex ? m_logo.tex : m_defaultLogoTex;
    if (m_usedLogoTex) {
        TextBoxRenderer::TextureDimensions &texSize = (m_usedLogoTex == m_logo.tex) ? m_logo.size : m_defaultLogoSize;
        int margin    = toPixels(m_config.logoMargin);
        int maxWidth  = m_metaStartX - 2 * margin;
        int maxHeight = m_screenSizeY - m_infoEndY - 2 * margin;
        int logoWidth = texSize.width, logoHeight = texSize.height;
        if (!m_config.logoScaling) {
            // scale down to next smaller power of two
            while ((logoWidth > maxWidth) || (logoHeight > maxHeight)) {
                logoWidth  >>= 1;
                logoHeight >>= 1;
            }
        } else if ((logoWidth > maxWidth) || (logoHeight > maxHeight)) {
            // scale to fit
            int s = (logoHeight * maxWidth + (logoWidth >> 1)) / logoWidth;
            if (s <= maxHeight) {
                logoWidth = maxWidth;
                logoHeight = s;
            } else {
                logoWidth = (logoWidth * maxHeight + (logoHeight >> 1)) / logoHeight;
                logoHeight = maxHeight;
            }
        }
        if (m_mod) {
            int minX0 = margin;
            int maxX0 = m_metaStartX - margin - logoWidth;
            int minY0 = m_infoEndY + margin;
            int maxY0 = m_screenSizeY - margin - logoHeight;
            m_logo.x0 = (minX0 * (100 - m_config.logoPosX) + maxX0 * m_config.logoPosX + 50) / 100;
            m_logo.y0 = (minY0 * (100 - m_config.logoPosY) + maxY0 * m_config.logoPosY + 50) / 100;
        }
        else {
            m_logo.x0 = (m_screenSizeX - logoWidth) >> 1;
            m_logo.y0 = toPixels(m_config.emptyLogoPosY) - (logoHeight >> 1);
        }
        m_logo.x1 = m_logo.x0 + logoWidth;
        m_logo.y1 = m_logo.y0 + logoHeight;
    }

    // set up clip indicator geometry
    if (m_config.clipEnabled) {
        int clipSize = toPixels(m_config.clipSize);
        int clipMargin = toPixels(m_config.clipMargin);
        m_clipX0 = clipMargin + ((m_screenSizeX - 2 * clipMargin - clipSize) * m_config.clipPosX + 50) / 100;
        m_clipY0 = clipMargin + ((m_screenSizeY - 2 * clipMargin - clipSize) * m_config.clipPosY + 50) / 100;
        m_clipX1 = m_clipX0 + clipSize;
        m_clipY1 = m_clipY0 + clipSize;
    }

    // set up toast geometry
    m_toastTextSize = toTextSize(m_config.toastTextSize);
    m_toastY = toPixels(m_config.toastPositionY);
    m_toastDY = ((m_toastTextSize + 1) >> 1) + toPixels(m_config.toastMarginY);
    m_toastDX = m_toastDY + toPixels(m_config.toastMarginX);

    // done!
    #if USE_PATTERN_CACHE
        m_patternCache.clear();
    #endif
    Dprintf("updateLayout(): channels=%d pdTextSize=%d pdRows=%d\n", m_numChannels, m_pdTextSize, m_pdRows);
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
