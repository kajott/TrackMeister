// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cmath>
#include <algorithm>

#include "config.h"
#include "renderer.h"
#include "app.h"

int Application::textWidth(int size, const char* text) {
    return int(std::ceil(m_renderer.textWidth(text) * float(size)));
}

int Application::toPixels(int value) {
    return int(m_screenSizeY * float(value) * .001f + .5f);
}

void Application::updateLayout() {
    m_screenSizeX = m_renderer.viewportWidth();
    m_screenSizeY = m_renderer.viewportHeight();

    m_emptyTextSize = toPixels(m_config.emptyTextSize);

    if (m_filename.empty() && m_title.empty() && m_artist.empty() && m_details.empty()) {
        // no info box at all
        m_infoEndY = m_infoShadowEndY = 0;
    } else {
        // layout info box
        m_infoTextSize = toPixels(m_config.infoTextSize);
        m_infoDetailsSize = toPixels(m_config.infoDetailsTextSize);
        m_infoKeyX = m_infoValueX = toPixels(m_config.infoMarginX);
        int lineSpacing = toPixels(m_config.infoLineSpacing);
        m_infoEndY = toPixels(m_config.infoMarginY) - lineSpacing;
        if (!m_filename.empty()) {
            m_infoEndY += lineSpacing;
            m_infoFilenameY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, m_infoKeyX + textWidth(m_infoTextSize, "File: "));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_artist.empty()) {
            m_infoEndY += lineSpacing;
            m_infoArtistY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, m_infoKeyX + textWidth(m_infoTextSize, "Artist: "));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_title.empty()) {
            m_infoEndY += lineSpacing;
            m_infoTitleY = m_infoEndY;
            m_infoValueX = std::max(m_infoValueX, m_infoKeyX + textWidth(m_infoTextSize, "Title: "));
            m_infoEndY += m_infoTextSize;
        }
        if (!m_details.empty()) {
            m_infoEndY += lineSpacing;
            m_infoDetailsY = m_infoEndY;
            m_infoEndY += m_infoDetailsSize;
        }
        m_infoEndY += lineSpacing + toPixels(m_config.infoMarginY);
        m_infoShadowEndY = m_infoEndY + m_config.infoShadowSize;
    }
}
