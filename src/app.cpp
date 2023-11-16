// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <libopenmpt/libopenmpt.hpp>

#include "system.h"
#include "renderer.h"
#include "config.h"
#include "app.h"

constexpr const char* baseWindowTitle = "Tracked Music Compo Player";

void Application::init(int argc, char* argv[]) {
    m_sys.initVideo(baseWindowTitle);
    m_sys.initAudio(true, m_config.sampleRate, m_config.audioBufferSize);
    if (!m_renderer.init()) {
        m_sys.fatalError("initialization failed", "could not initialize text box renderer");
    }
    updateLayout();
    if (argc > 1) { loadModule(argv[1]); }
}

void Application::draw(float dt) {
    (void)dt;

    // set background color
    uint32_t clearColor = m_mod ? m_config.patternBackground : m_config.emptyBackground;
    glClearColor(float( clearColor        & 0xFF) * float(1.f/255.f),
                 float((clearColor >>  8) & 0xFF) * float(1.f/255.f),
                 float((clearColor >> 16) & 0xFF) * float(1.f/255.f), 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw info box
    if (m_infoEndY > 0) {
        m_renderer.box(0, 0, m_screenSizeX, m_infoEndY, m_config.infoBackground);
        if (m_infoShadowEndY > m_infoEndY) {
            m_renderer.box(0, m_infoEndY, m_screenSizeX, m_infoShadowEndY, m_config.shadowColor, m_config.shadowColor & 0xFFFFFFu);
        }
        if (!m_filename.empty()) {
            float x = m_renderer.text(float(m_infoKeyX), float(m_infoFilenameY), float(m_infoTextSize), "File", 0, m_config.infoKeyColor);
            m_renderer.text(x, float(m_infoFilenameY), float(m_infoTextSize), ":", 0, m_config.infoColonColor);
            m_renderer.text(float(m_infoValueX), float(m_infoFilenameY), float(m_infoTextSize), m_filename.c_str(), 0, m_config.infoValueColor);
        }
        if (!m_artist.empty()) {
            float x = m_renderer.text(float(m_infoKeyX), float(m_infoArtistY), float(m_infoTextSize), "Artist", 0, m_config.infoKeyColor);
            m_renderer.text(x, float(m_infoArtistY), float(m_infoTextSize), ":", 0, m_config.infoColonColor);
            m_renderer.text(float(m_infoValueX), float(m_infoArtistY), float(m_infoTextSize), m_artist.c_str(), 0, m_config.infoValueColor);
        }
        if (!m_title.empty()) {
            float x = m_renderer.text(float(m_infoKeyX), float(m_infoTitleY), float(m_infoTextSize), "Title", 0, m_config.infoKeyColor);
            m_renderer.text(x, float(m_infoTitleY), float(m_infoTextSize), ":", 0, m_config.infoColonColor);
            m_renderer.text(float(m_infoValueX), float(m_infoTitleY), float(m_infoTextSize), m_title.c_str(), 0, m_config.infoValueColor);
        }
        if (!m_details.empty()) {
            m_renderer.text(float(m_infoKeyX), float(m_infoDetailsY), float(m_infoDetailsSize), m_details.c_str(), 0, m_config.infoDetailsColor);
        }
    }

    // draw "no module loaded" screen
    if (!m_mod) {
        m_renderer.text(
            float(m_screenSizeX >> 1), float((m_infoEndY + m_screenSizeY) >> 1),
            float(m_emptyTextSize), "Nothing to play.",
            Align::Center + Align::Middle, m_config.emptyTextColor);
        m_renderer.flush();
        return;
    }

if (m_mod && m_sys.isPlaying()) { printf("@ %03d.%02X \r", m_mod->get_current_order(), m_mod->get_current_row()); fflush(stdout); }
    m_renderer.flush();
}

void Application::shutdown() {
    unloadModule();
    m_renderer.shutdown();
}

bool Application::renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate) {
    if (!m_mod) { return false; }
    int done;
    if (stereo) {
        done = int(m_mod->read_interleaved_stereo(sampleRate, sampleCount, data));
    } else {
        done = int(m_mod->read(sampleRate, sampleCount, data));
    }
    if (done < sampleCount) {
        data += stereo ? (done << 1) : done;
        sampleCount -= done;
        ::memset(static_cast<void*>(data), 0, stereo ? (sampleCount << 2) : (sampleCount << 1));
    }
    return true;
}

void Application::handleKey(int key) {
    switch (key) {
        case 'Q':
            m_sys.quit();
            break;
        case ' ':
            if (m_mod) { m_sys.togglePause(); }
            break;
        case keyCode("Left"):
            if (m_mod) {
                AudioMutexGuard mtx_(m_sys);
                int dest = m_mod->get_current_order() - 1;
                Dprintf("seeking to order %d\n", dest);
                m_mod->set_position_order_row(dest, 0);
            } break;
        case keyCode("Right"):
            if (m_mod) {
                AudioMutexGuard mtx_(m_sys);
                int dest = m_mod->get_current_order() + 1;
                Dprintf("seeking to order %d\n", dest);
                m_mod->set_position_order_row(dest, 0);
            } break;
        default:
            break;
    }
}

void Application::handleDropFile(const char* path) {
    loadModule(path);
}

void Application::handleResize(int w, int h) {
    glViewport(0, 0, w, h);
    m_renderer.viewportChanged();
    updateLayout();
}

void Application::unloadModule() {
    m_sys.pause();
    {
        AudioMutexGuard mtx_(m_sys);
        delete m_mod;
        m_mod = nullptr;
        m_mod_data.clear();
    }
    m_filename.clear();
    m_title.clear();
    m_artist.clear();
    m_details.clear();
    m_sys.setWindowTitle(baseWindowTitle);
    Dprintf("module unloaded, playback paused\n");
}

bool Application::loadModule(const char* path) {
    bool res = false;
    unloadModule();
    if (!path || !path[0]) { return res; }

    // set filename metadata
    const char* fn = path;
    for (const char* s = path;  *s;  ++s) {
        if ((*s == '/') || (*s == '\\')) { fn = &s[1]; }
    }
    m_filename.assign(fn);

    // load file into memory
    Dprintf("loading module: %s\n", path);
    FILE *f = fopen(path, "rb");
    if (!f) {
        Dprintf("could not open module file.\n");
        m_details.assign("could not open file");
        return res;
    }
    fseek(f, 0, SEEK_END);
    m_mod_data.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    res = (fread(m_mod_data.data(), 1, m_mod_data.size(), f) == m_mod_data.size());
    fclose(f);

    // load and setup OpenMPT instance
    AudioMutexGuard mtx_(m_sys);
    if (res) {
        std::map<std::string, std::string> ctls;
        ctls["play.at_end"] = "stop";
        switch (m_config.interpolation) {
            case InterpolationMethod::Amiga:
                ctls["render.resampler.emulate_amiga"] = "1";
                break;
            case InterpolationMethod::A500:
                ctls["render.resampler.emulate_amiga"] = "1";
                ctls["render.resampler.emulate_amiga_type"] = "a500";
                break;
            case InterpolationMethod::A1200:
                ctls["render.resampler.emulate_amiga"] = "1";
                ctls["render.resampler.emulate_amiga_type"] = "a1200";
                break;
            default: break;  // no Amiga resampler -> set later using set_render_param
        }
        m_mod = new openmpt::module(m_mod_data, std::clog, ctls);
    }
    if (!m_mod) {
        Dprintf("module loading failed.\n");
        m_details.assign("invalid module data");
        updateLayout();
    }
    Dprintf("module loaded successfully.\n");
    switch (m_config.interpolation) {
        case InterpolationMethod::None:   m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 1); break;
        case InterpolationMethod::Linear: m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 2); break;
        case InterpolationMethod::Cubic:  m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 4); break;
        case InterpolationMethod::Sinc:   m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 8); break;
        default: break;  // Auto or Amiga -> no need to set anything up
    }
    m_mod->set_render_param(openmpt::module::render_param::RENDER_STEREOSEPARATION_PERCENT, m_config.stereoSeparationPercent);

    // get metadata
    m_artist.assign(m_mod->get_metadata("artist"));
    m_title.assign(m_mod->get_metadata("title"));
    auto addDetail = [&] (const std::string& s) {
        if (s.empty()) { return; }
        if (!m_details.empty()) { m_details.append(", "); }
        m_details.append(s);
    };
    addDetail(m_mod->get_metadata("type_long"));
    addDetail(std::to_string(m_mod->get_num_channels()) + " channels");
    addDetail(std::to_string(m_mod->get_num_patterns()) + " patterns");
    addDetail(std::to_string(m_mod->get_num_orders()) + " orders");
    if (m_mod->get_num_instruments()) { addDetail(std::to_string(m_mod->get_num_instruments()) + " instruments"); }
    addDetail(std::to_string(m_mod->get_num_samples()) + " samples");
    addDetail(std::to_string((m_mod_data.size() + 1023u) >> 10) + "K bytes");
    int sec = int(m_mod->get_duration_seconds());
    addDetail(std::to_string(sec / 60) + ":" + std::to_string((sec / 10) % 6) + std::to_string(sec % 10));

    // done!
    m_sys.setWindowTitle((m_filename + " - " + baseWindowTitle).c_str());
    updateLayout();
    return res;
}
