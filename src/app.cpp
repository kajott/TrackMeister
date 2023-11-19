// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>

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
constexpr float scrollAnimationSpeed = -10.f;

void Application::init(int argc, char* argv[]) {
    m_sys.initVideo(baseWindowTitle,
        #ifdef NDEBUG
            m_config.fullscreen,
        #else
            false,
        #endif
        m_config.windowWidth, m_config.windowHeight);
    m_sys.initAudio(true, m_config.sampleRate, m_config.audioBufferSize);
    if (!m_renderer.init()) {
        m_sys.fatalError("initialization failed", "could not initialize text box renderer");
    }
    updateLayout();
    if (argc > 1) { loadModule(argv[1]); }
}

void Application::draw(float dt) {
    // latch current position
    if (m_mod) {
        AudioMutexGuard mtx_(m_sys);
        m_currentOrder = m_mod->get_current_order();
        int pat = m_mod->get_current_pattern();
        if (pat != m_currentPattern) { m_patternLength = m_mod->get_pattern_num_rows(pat); }
        m_currentPattern = pat;
        m_currentRow = m_mod->get_current_row();
        m_position = float(m_mod->get_position_seconds());
    }

    // handle animations
    if (m_metaTextAutoScroll) {
        setMetadataScroll(m_metaTextMinY + (m_metaTextMaxY - m_metaTextMinY) * m_position / m_duration);
        m_metaTextY = m_metaTextTargetY;
    } else {
        m_metaTextY += (1.0f - std::exp2f(scrollAnimationSpeed * dt)) * (m_metaTextTargetY - m_metaTextY);
    }

    // set background color
    uint32_t clearColor = m_mod ? m_config.patternBackground : m_config.emptyBackground;
    glClearColor(float( clearColor        & 0xFF) * float(1.f/255.f),
                 float((clearColor >>  8) & 0xFF) * float(1.f/255.f),
                 float((clearColor >> 16) & 0xFF) * float(1.f/255.f), 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw pattern display
    if (m_mod) {
        m_renderer.box(m_pdBarStartX, m_pdTextY0, m_pdBarEndX, m_pdTextY0 + m_pdTextSize,
                       m_config.patternBarBackground, m_config.patternBarBackground, false,
                       m_pdBarRadius);
        char posText[16], posAttr[16];
        for (int dRow = -m_pdRows;  dRow <= m_pdRows;  ++dRow) {
            int row = dRow + m_currentRow;
            if ((row < 0) || (row >= m_patternLength)) { continue; }
            float alpha = 1.0f - std::pow(std::abs(float(dRow) / float(m_pdRows + 1)), m_config.patternAlphaFalloffShape) * m_config.patternAlphaFalloff;
            float y = float(m_pdTextY0 + dRow * m_pdTextDY);
            if (m_pdPosChars) {
                formatPosition(m_currentOrder, m_currentPattern, row, posText, posAttr, m_pdPosChars);
                drawPatternDisplayCell(float(m_pdPosX), y, posText, posAttr, alpha, false);
            }
            for (int ch = 0;  ch < m_numChannels;  ++ch) {
                drawPatternDisplayCell(float(m_pdChannelX0 + ch * m_pdChannelDX), y,
                       m_mod->format_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str(),
                    m_mod->highlight_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str(),
                    alpha, (m_pdPosChars > 0) || (ch > 0));
            }
        }
    }

    // draw info box
    if (m_infoVisible) {
        m_renderer.box(0, 0, m_metaStartX, m_infoEndY, m_config.infoBackground);
        if (m_infoShadowEndY > m_infoEndY) {
            m_renderer.box(0, m_infoEndY, m_screenSizeX, m_infoShadowEndY, m_config.shadowColor, m_config.shadowColor & 0x00FFFFFFu, false);
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

    // draw metadata sidebar
    if (m_metaVisible) {
        m_renderer.box(m_metaStartX, 0, m_screenSizeX, m_screenSizeY, m_config.metaBackground);
        if (m_metaShadowStartX < m_metaStartX) {
            m_renderer.box(m_metaShadowStartX, 0, m_metaStartX, m_screenSizeY, m_config.shadowColor & 0x00FFFFFFu, m_config.shadowColor, true);
        }
        m_metadata.draw(m_metaTextX, m_metaTextY);
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

    // done
    m_renderer.flush();
}

void Application::drawPatternDisplayCell(float x, float y, const char* text, const char* attr, float alpha, bool pipe) {
    const float sz = float(m_pdTextSize);
    if (pipe) {
        m_renderer.text(x - m_pdPipeDX, y, sz, "|", 0u, m_renderer.extraAlpha(m_config.patternSepColor, alpha));
    }
    char c[2] = " ";
    while (*text) {
        c[0] = *text++;
        char a = *attr;
        uint32_t color;
        switch (a) {
            case '.': color = m_config.patternDotColor;         break;
            case 'n': color = m_config.patternNoteColor;        break;
            case 'm': color = m_config.patternSpecialColor;     break;
            case 'i': color = m_config.patternInstrumentColor;  break;
            case 'u': color = m_config.patternVolEffectColor;   break;
            case 'v': color = m_config.patternVolParamColor;    break;
            case 'e': color = m_config.patternEffectColor;      break;
            case 'f': color = m_config.patternEffectParamColor; break;
            case 'O': color = m_config.patternPosOrderColor;    break;
            case 'P': color = m_config.patternPosPatternColor;  break;
            case 'R': color = m_config.patternPosRowColor;      break;
            case ':': color = m_config.patternPosDotColor;      break;
            default:  color = m_config.patternTextColor;        break;
        }
        x = m_renderer.text(x, y, sz, c, 0u, m_renderer.extraAlpha(color, alpha));
        if (a) { ++attr; }
    }
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
        case 9:  // Tab
            cycleBoxVisibility();
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

void Application::handleMouseWheel(int delta) {
    setMetadataScroll(m_metaTextTargetY + float(delta * 3 * m_metadata.defaultSize));
    m_metaTextAutoScroll = false;
}

void Application::setMetadataScroll(float y) {
    m_metaTextTargetY = std::max(m_metaTextMaxY, std::min(m_metaTextMinY, y));
}

void Application::cycleBoxVisibility() {
    if (m_infoVisible && m_metaVisible) {
        m_metaVisible = false;
    } else if (m_infoVisible) {
        m_infoVisible = false;
    } else if (m_metaVisible) {
        if (infoValid()) { m_infoVisible = true; } else { m_metaVisible = false; }
    } else {
        if (metaValid()) { m_metaVisible = true; } else { m_infoVisible = infoValid(); }
    }
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
    m_metadata.clear();
    m_numChannels = 0;
    m_currentPattern = -1;
    m_patternLength = 0;
    m_sys.setWindowTitle(baseWindowTitle);
    updateLayout(true);
    Dprintf("module unloaded, playback paused\n");
}

bool Application::loadModule(const char* path) {
    unloadModule();
    if (!path || !path[0]) { return false; }

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
        updateLayout(true);
        return false;
    }
    fseek(f, 0, SEEK_END);
    m_mod_data.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    if (fread(m_mod_data.data(), 1, m_mod_data.size(), f) != m_mod_data.size()) {
        Dprintf("could not read module file.\n");
        m_details.assign("could not read file");
        updateLayout(true);
        return false;
    }
    fclose(f);

    // load and setup OpenMPT instance
    AudioMutexGuard mtx_(m_sys);
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
    try {
        m_mod = new openmpt::module(m_mod_data, std::clog, ctls);
    } catch (openmpt::exception& e) {
        Dprintf("exception caught from OpenMPT: %s\n", e.what());
        m_details.assign(std::string("invalid module - ") + e.what());
        delete m_mod;
        m_mod = nullptr;
        updateLayout(true);
        return false;
    }
    if (!m_mod) {
        Dprintf("module loading failed.\n");
        m_details.assign("invalid module data");
        updateLayout(true);
        return false;
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

    // get info box metadata
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

    // get sidebar metadata: first instrument and sample names into a separate
    // buffer, and then using this buffer's width to line-wrap the module
    // message (if any)
    TextArea meta2(m_renderer);
    m_metadata.defaultSize = meta2.defaultSize;
    m_metadata.defaultColor = m_config.metaTextColor;
    if (m_config.metaShowInstrumentNames) {
        addMetadataGroup(meta2, m_mod->get_instrument_names(), "Instrument Names:");
    }
    if (m_config.metaShowSampleNames) {
        addMetadataGroup(meta2, m_mod->get_sample_names(), "Sample Names:");
    }
    std::string msgStr;
    if (m_config.metaShowMessage) {
        msgStr.assign(m_mod->get_metadata("message_raw"));
    }
    if (!msgStr.empty()) {
        // split string into lines, collapse multiple empty lines into single
        std::vector<std::string> msgLines;
        bool precedingEmptyLine = false;
        size_t start = 0, end;
        do {
            end = msgStr.find('\n', start);
            if (end == std::string::npos) { end = msgStr.size(); }
            size_t realEnd = end;
            while ((realEnd > start) && std::isspace(msgStr[realEnd - 1u])) { --realEnd; }
            if (realEnd > start) {
                if (precedingEmptyLine) { msgLines.emplace_back(""); }
                msgLines.emplace_back(msgStr.substr(start, realEnd - start));
                precedingEmptyLine = false;
            } else {
                precedingEmptyLine = true;
            }
            start = end + 1u;
        } while (end != msgStr.size());
        // add lines to metadata block (with wrapping)
        float maxWidth = std::max(meta2.width(), m_renderer.textWidth("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx") * meta2.defaultSize);
        for (const auto& line : msgLines) {
            m_metadata.addWrappedLine(maxWidth, line);
        }
    }
    m_metadata.ingest(meta2);

    // done!
    m_sys.setWindowTitle((m_filename + " - " + baseWindowTitle).c_str());
    m_numChannels = m_mod->get_num_channels();
    m_duration = std::min(float(m_mod->get_duration_seconds()), m_config.maxScrollDuration);
    m_metaTextAutoScroll = m_config.enableAutoScroll;
    updateLayout(true);
    return true;
}

void Application::addMetadataGroup(TextArea& block, const std::vector<std::string>& data, const char* title, bool numbering) {
    int precedingEmptyLine = -1;
    bool titleSent = false;
    int lineIndex = 0;
    auto emitLine = [&] (int index, const char* text) {
        auto& line = block.addLine();
        if (numbering) {
            char idxS[3];
            idxS[0] = "0123456789ABCDEF"[(index >> 4) & 15];
            idxS[1] = "0123456789ABCDEF"[ index       & 15];
            idxS[2] = '\0';
            line.addSpan(m_config.metaIndexColor, idxS);
            line.addSpan(m_config.metaColonColor, ":");
        }
        line.addSpan(m_config.metaTextColor, text);
    };
    for (const auto& line : data) {
        if (!line.empty()) {
            if (title && !titleSent) {
                block.addLine(m_config.metaHeadingColor, title).marginTop = 1.f;
                titleSent = true;
            }
            if (precedingEmptyLine >= 0) {
                emitLine(precedingEmptyLine, "");
                precedingEmptyLine = -1;
            }
            emitLine(lineIndex, line.c_str());
        } else if (precedingEmptyLine < 0) {
            precedingEmptyLine = lineIndex;
        }
        ++lineIndex;
    }
}
