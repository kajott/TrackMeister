// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>

#include "system.h"

namespace openmpt {
    class module;
};

class Application {
    SystemInterface& m_sys;
    openmpt::module* m_mod = nullptr;
    std::vector<std::byte> m_mod_data;

public:  // interface from SystemInterface
    explicit inline Application(SystemInterface& sys) : m_sys(sys) {}

    void init(int argc, char* argv[]);
    void draw(float dt);
    void done();

    bool renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate);

    void handleKey(int key);
    void handleDropFile(const char* path);

private:  // business logic
    void unloadModule();
    bool loadModule(const char* path);
};
