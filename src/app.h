// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>

#include "system.h"
#include "renderer.h"
#include "config.h"
#include "layout.h"

namespace openmpt {
    class module;
};

class Application {
    SystemInterface& m_sys;
    TextBoxRenderer m_renderer;
    Config m_config;
    Layout m_layout;
    openmpt::module* m_mod = nullptr;
    std::vector<std::byte> m_mod_data;

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
    void updateLayout();
};
