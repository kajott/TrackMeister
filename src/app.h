// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>
#include <string>

#include "system.h"
#include "renderer.h"
#include "textarea.h"
#include "config.h"

namespace openmpt {
    class module;
};

class Application {
    // core data
    SystemInterface& m_sys;
    TextBoxRenderer m_renderer;
    Config m_config;
    openmpt::module* m_mod = nullptr;
    std::vector<std::byte> m_mod_data;

    // metadata
    std::string m_filename;
    std::string m_artist;
    std::string m_title;
    std::string m_details;
    TextArea m_metadata;
    float m_duration;

    // current position and size information
    int m_currentOrder;
    int m_currentPattern;
    int m_currentRow;
    int m_numChannels;
    int m_patternLength;
    float m_position;

    // computed layout information (from updateLayout())
    int m_screenSizeX, m_screenSizeY;
    int m_emptyTextSize;
    int m_infoTextSize, m_infoDetailsSize;
    int m_infoEndY, m_infoShadowEndY;
    int m_infoKeyX, m_infoValueX;
    int m_infoFilenameY, m_infoArtistY, m_infoTitleY, m_infoDetailsY;
    int m_metaStartX, m_metaShadowStartX;
    float m_metaTextX, m_metaTextMinY, m_metaTextMaxY;
    int m_pdPosChars, m_pdChannelChars;
    int m_pdTextSize, m_pdTextY0, m_pdTextDY, m_pdRows;
    int m_pdPosX, m_pdChannelX0, m_pdChannelDX;
    float m_pdPipeDX;
    int m_pdBarStartX, m_pdBarEndX, m_pdBarRadius;

    // current view state
    float m_metaTextY, m_metaTextTargetY;
    bool m_metaTextAutoScroll = true;
    bool m_infoVisible, m_metaVisible;

public:  // interface from SystemInterface
    explicit inline Application(SystemInterface& sys) : m_sys(sys), m_metadata(m_renderer) {}

    void init(int argc, char* argv[]);
    void draw(float dt);
    void shutdown();

    bool renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate);

    void handleKey(int key);
    void handleDropFile(const char* path);
    void handleResize(int w, int h);
    void handleMouseWheel(int delta);

private:  // business logic
    void unloadModule();
    bool loadModule(const char* path);
    void cycleBoxVisibility();
    int toPixels(int value) const;
    int textWidth(int size, const char* text) const;
    void updateLayout(bool resetBoxVisibility=false);
    void drawPatternDisplayCell(float x, float y, const char* text, const char* attr, float alpha=1.0f, bool pipe=true);
    static void formatPosition(int order, int pattern, int row, char* text, char* attr, int size);
    void addMetadataGroup(TextArea& block, const std::vector<std::string>& data, const char* title, bool numbering=true);
    void setMetadataScroll(float y);
    inline bool infoValid() const { return !m_filename.empty() || !m_title.empty() || !m_artist.empty() || !m_details.empty(); }
    inline bool metaValid() const { return !m_metadata.empty(); }
};
