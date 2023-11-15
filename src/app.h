// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>
#include <string>

#include "system.h"
#include "renderer.h"
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

    // computed layout information (from updateLayout())
    int m_screenSizeX, m_screenSizeY;
    int m_emptyTextSize;
    int m_infoTextSize, m_infoDetailsSize;
    int m_infoEndY, m_infoShadowEndY;
    int m_infoKeyX, m_infoValueX;
    int m_infoFilenameY, m_infoArtistY, m_infoTitleY, m_infoDetailsY;

public:  // interface from SystemInterface
    explicit inline Application(SystemInterface& sys) : m_sys(sys) {}

    void init(int argc, char* argv[]);
    void draw(float dt);
    void shutdown();

    bool renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate);

    void handleKey(int key);
    void handleDropFile(const char* path);
    void handleResize(int w, int h);

private:  // business logic
    void unloadModule();
    bool loadModule(const char* path);
    int toPixels(int value);
    int textWidth(int size, const char* text);
    void updateLayout();
};
