// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <libopenmpt/libopenmpt.hpp>
#include <ebur128.h>

#include "system.h"
#include "renderer.h"
#include "textarea.h"
#include "config.h"
#include "pathutil.h"
#include "util.h"
#include "app.h"
#include "version.h"

constexpr const char* baseWindowTitle = "Tracked Music Compo Player";
constexpr float scrollAnimationSpeed = -10.f;
constexpr size_t scanBufferSize = 4096;

////////////////////////////////////////////////////////////////////////////////

///// init + shutdown

void Application::init(int argc, char* argv[]) {
    // load initial configuration (required for video and audio parameters)
    m_cmdline = Config::prepareCommandLine(argc, argv);
    m_mainIniFile.assign(argv[0]);
    PathUtil::dirnameInplace(m_mainIniFile);
    PathUtil::joinInplace(m_mainIniFile, "tmcp.ini");
    m_config.load(m_mainIniFile.c_str());
    m_config.load(m_cmdline);

    // initialize everything
    m_sys.initVideo(baseWindowTitle,
        #ifdef NDEBUG
            m_config.fullscreen,
        #else
            false,
        #endif
        m_config.windowWidth, m_config.windowHeight);
    m_sampleRate = m_sys.initAudio(true, m_config.sampleRate, m_config.audioBufferSize);
    if (!m_renderer.init()) {
        m_sys.fatalError("initialization failed", "could not initialize text box renderer");
    }

    // populate playable extension list
    m_playableExts.clear();
    for (auto& ext : openmpt::get_supported_extensions()) {
        m_playableExts.push_back(makeFourCC(ext.c_str()));
    }
    m_playableExts.push_back(0);

    // load module from command line
    loadModule((argc > 1) ? argv[1] : nullptr);
    if (m_fullpath.empty()) { toastVersion(); }
}

void Application::shutdown() {
    unloadModule();
    m_renderer.shutdown();
}

////////////////////////////////////////////////////////////////////////////////

///// event handlers and related code

bool Application::renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate) {
    if (!m_mod || m_scanning) { return false; }

    // retrieve samples from libopenmpt
    int16_t* pos = data;
    int done, remain = sampleCount;
    bool hadNullRead = false;
    while (remain > 0) {
        // render a fragment
        if (stereo) {
            done = int(m_mod->read_interleaved_stereo(sampleRate, remain, pos));
        } else {
            done = int(m_mod->read(sampleRate, remain, pos));
        }
        // advance pointers
        if (!done) {
            // null read -> indicator for end of track or looping position
            if (hadNullRead) {
                // second null read in succession -> we're truly at the end of the track
                m_endReached = true;
                break;
            }
            hadNullRead = true;
        } else if ((done < 0) || (done > remain)) {
            Dprintf("openmpt::module::read() returned %d samples, requested were %d samples\n", done, remain);
            break;  // panic!
        } else {
            remain -= done;
            pos += stereo ? (done << 1) : done;
        }
    }

    // fill in missing samples with silence
    if (remain) {
        ::memset(static_cast<void*>(pos), 0, stereo ? (remain << 2) : (remain << 1));
    }

    // apply fade-out
    if (m_fadeActive) {
        remain = stereo ? (sampleCount << 1) : sampleCount;
        pos = data;
        int gain16b = m_fadeGain >> 15;
        while (remain--) {
            *pos = int16_t((int(*pos) * gain16b + 32767) >> 16);
            ++pos;
            m_fadeGain = std::max(0, m_fadeGain - m_fadeRate);
        }
    }

    // start fade-out after loop, if so desired
    if (hadNullRead && m_config.loop && m_config.fadeOutAfterLoop && !m_autoFadeInitiated) {
        fadeOut();
        m_autoFadeInitiated = true;
    }
    return true;
}

void Application::fadeOut() {
    if (!m_mod) { return; }
    if (m_fadeActive) {
        m_sys.pause();
        m_fadeActive = false;
        return;
    }
    AudioMutexGuard mtx_(m_sys);
    m_fadeGain = 0x7FFFFFFF;
    m_fadeRate = int(double(m_fadeGain) / (double(m_sampleRate) * 2.0 * double(m_config.fadeDuration)) + 0.5);
    Dprintf("fadeOut(): fade rate = %d\n", m_fadeRate);
    m_fadeActive = true;
}

void Application::handleKey(int key, bool ctrl, bool shift, bool alt) {
    (void)alt;
    if (key != 27) { m_escapePressedOnce = false; }
    switch (key) {
        case 'Q':  // quit immediately
            m_sys.quit();
            break;
        case 27:  // [Esc] cancel whatever is currently going on
            if (m_scanning) {  // cancel loudness scan
                m_multiScan = false;
                stopScan();
            } else if (m_mod && m_sys.isPlaying()) {  // pause
                m_sys.pause();
            } else if (!m_escapePressedOnce) {  // first Esc while paused -> do nothing (yet)
                m_escapePressedOnce = true;
            } else {
                m_sys.quit();  // second Esc -> quit
            }
            break;
        case ' ':  // [Space] pause/play
            if (m_mod) { m_fadeActive = false; m_sys.togglePause(); }
            break;
        case '\t':  // [Tab] show/hide info
            cycleBoxVisibility();
            break;
        case '\r':  // [Enter] show/hide fake VU meters
            m_vuVisible = !m_vuVisible;
            break;
        case 'N':  // [N] show/hide channel names
            m_namesVisible = !m_namesVisible && namesValid();
            break;
        case 'A':  // toggle autoscroll
            m_metaTextAutoScroll = !m_metaTextAutoScroll;
            break;
        case 'S':  // [Ctrl+Shift+S] save config
            if (ctrl && shift) {
                Config defaultConfig;
                if (defaultConfig.save("tmcp_default.ini")) {
                    toast("saved tmcp_default.ini");
                } else {
                    toast("saving tmcp_default.ini failed");
                }
            }
            break;
        case 'L':  // [Ctrl+L] run (or stop) loudness scan
            if (ctrl) {
                bool wasScanning = m_scanning;
                m_multiScan = false;
                stopScan();
                if (!wasScanning) {
                    m_multiScan = shift;
                    startScan();
                }
            }
            break;
        case 'V':  // show version
            toastVersion();
            break;
        case 'F':  // start fade-out
            fadeOut();
            break;
        case 0xF5: {  // [F5] reload module
            std::string savePath(m_fullpath);
            loadModule(savePath.c_str());
            break; }
        case 0xFB:  // [F11] toggle fullscreen
            m_sys.toggleFullscreen();
            break;
        case makeFourCC("Left"):  // previous pattern
            if (m_mod) {
                AudioMutexGuard mtx_(m_sys);
                int dest = m_mod->get_current_order() - 1;
                Dprintf("seeking to order %d\n", dest);
                m_mod->set_position_order_row(dest, 0);
            } break;
        case makeFourCC("Right"):  // next pattern
            if (m_mod) {
                AudioMutexGuard mtx_(m_sys);
                int dest = m_mod->get_current_order() + 1;
                Dprintf("seeking to order %d\n", dest);
                m_mod->set_position_order_row(dest, 0);
            } break;
        case makeFourCC("PgUp"): {  // previous module
            std::string newPath(PathUtil::findSibling(m_fullpath, false, m_playableExts.data()));
            if (!newPath.empty()) { loadModule(newPath.c_str()); }
            break; }
        case makeFourCC("PgDn"): {  // next module
            std::string newPath(PathUtil::findSibling(m_fullpath, true, m_playableExts.data()));
            if (!newPath.empty()) { loadModule(newPath.c_str()); }
            break; }
        case makeFourCC("Home"):  // first module in directory
            if (ctrl) {
                std::string newPath(PathUtil::findSibling(PathUtil::dirname(m_fullpath) + "/", true, m_playableExts.data()));
                if (!newPath.empty()) { loadModule(newPath.c_str()); }
            }   break;
        case makeFourCC("End"):  // last module in directory
            if (ctrl) {
                std::string newPath(PathUtil::findSibling(PathUtil::dirname(m_fullpath) + "/", false, m_playableExts.data()));
                if (!newPath.empty()) { loadModule(newPath.c_str()); }
            }   break;
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

void Application::toast(const char *msg) {
    Dprintf("TOAST: %s\n", msg);
    m_toastMessage.assign(msg);
    m_toastAlpha = 1.0f;
}

void Application::toastVersion() {
    std::string ver;
    ver.reserve(128);
    ver.assign(g_ProductName);
    ver.append(" ");
    ver.append(g_ProductVersion);
    ver.append(" / libopenmpt ");
    ver.append(openmpt::string::get("library_version"));
    #define STRINGIFY2(x) #x
    #define STRINGIFY(x) STRINGIFY2(x)
    #if defined(__clang__)
        ver.append(" / Clang " STRINGIFY(__clang_major__) "." STRINGIFY(__clang_minor__));
    #elif defined(_MSC_VER)
        ver.append(" / MSVC " STRINGIFY(_MSC_VER));
    #elif defined(__GNUC__)
        ver.append(" / GCC " STRINGIFY(__GNUC__) "." STRINGIFY(__GNUC_MINOR__));
    #else
        ver.append(" /");
    #endif
    #ifdef NDEBUG
        ver.append(" Release");
    #else
        ver.append(" Debug");
    #endif
    toast(ver);
}

////////////////////////////////////////////////////////////////////////////////

///// drawing

void Application::draw(float dt) {
    float fadeAlpha = 1.0f;

    // handle loudness scan end
    if (m_scanning && m_endReached) {
        stopScan();
    }

    // latch current position
    if (m_mod) {
        AudioMutexGuard mtx_(m_sys);
        m_currentOrder = m_mod->get_current_order();
        int pat = m_mod->get_current_pattern();
        if (pat != m_currentPattern) { m_patternLength = m_mod->get_pattern_num_rows(pat); }
        m_currentPattern = pat;
        m_currentRow = m_mod->get_current_row();
        m_position = float(m_mod->get_position_seconds());
        if (m_fadeActive) {
            fadeAlpha = float(float(m_fadeGain) / float(0x7FFFFFFF));
        }
    }

    // start auto-fading, if applicable
    if ((m_config.fadeOutAt > 0.0f) && !m_autoFadeInitiated && (m_position > m_config.fadeOutAt)) {
        fadeOut();
        m_autoFadeInitiated = true;
    }

    // handle animations
    if (m_metaTextAutoScroll) {
        setMetadataScroll(m_metaTextMinY + (m_metaTextMaxY - m_metaTextMinY) * m_position / m_scrollDuration);
    }
    m_metaTextY += (1.0f - std::exp2f(scrollAnimationSpeed * dt)) * (m_metaTextTargetY - m_metaTextY);

    // set background color
    uint32_t clearColor = m_mod ? m_config.patternBackground : m_config.emptyBackground;
    glClearColor(float( clearColor        & 0xFF) * float(1.f/255.f),
                 float((clearColor >>  8) & 0xFF) * float(1.f/255.f),
                 float((clearColor >> 16) & 0xFF) * float(1.f/255.f), 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw VU meters
    if (m_mod && m_vuVisible && m_sys.isPlaying() && !m_endReached
    && (m_vuHeight > 0.0f) && ((m_config.vuLowerColor | m_config.vuUpperColor) & 0xFF000000u)) {
        for (int ch = 0;  ch < m_numChannels;  ++ch) {
            int x = m_pdChannelX0 + ch * m_pdChannelDX;
            float vu = std::min(1.0f, m_mod->get_current_channel_vu_mono(ch)) * fadeAlpha;
            if (vu > 0.0f) {
                m_renderer.box(x, m_pdTextY0 - int(vu * m_vuHeight + 0.5f),
                               x + m_pdNoteWidth, m_pdTextY0,
                               m_config.vuUpperColor,
                               m_config.vuLowerColor);
            }
        }
    }

    // draw pattern display
    if (m_mod) {
        uint32_t barColor = m_renderer.extraAlpha(m_config.patternBarBackground, fadeAlpha);
        m_renderer.box(m_pdBarStartX, m_pdTextY0, m_pdBarEndX, m_pdTextY0 + m_pdTextSize,
                       barColor, barColor, false, m_pdBarRadius);
        CacheItem tempItem;
        for (int dRow = -m_pdRows;  dRow <= m_pdRows;  ++dRow) {
            int row = dRow + m_currentRow;
            if ((row < 0) || (row >= m_patternLength)) { continue; }
            float alpha = fadeAlpha * (1.0f - std::pow(std::abs(float(dRow) / float(m_pdRows + 1)), m_config.patternAlphaFalloffShape) * m_config.patternAlphaFalloff);
            float y = float(m_pdTextY0 + dRow * m_pdTextDY);
            if (m_pdPosChars) {
                formatPosition(m_currentOrder, m_currentPattern, row, tempItem.text, tempItem.attr, m_pdPosChars);
                drawPatternDisplayCell(float(m_pdPosX), y, tempItem.text, tempItem.attr, alpha, false);
            }
            for (int ch = 0;  ch < m_numChannels;  ++ch) {
                float x = float(m_pdChannelX0 + ch * m_pdChannelDX);
                bool pipe = (m_pdPosChars > 0) || (ch > 0);
                #if USE_PATTERN_CACHE
                    CacheKey key = makeCacheKey(m_currentPattern, row, ch);
                    CacheItem *item;
                    auto entry = m_patternCache.find(key);
                    if (entry == m_patternCache.end()) {
                        ::strcpy(tempItem.text,    m_mod->format_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str());
                        ::strcpy(tempItem.attr, m_mod->highlight_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str());
                        m_patternCache[key] = tempItem;
                        item = &tempItem;
                    } else {
                        item = &entry->second;
                    }
                    drawPatternDisplayCell(x, y, item->text, item->attr, alpha, pipe);
                #else
                    drawPatternDisplayCell(x, y,
                           m_mod->format_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str(),
                        m_mod->highlight_pattern_row_channel(m_currentPattern, row, ch, m_pdChannelChars).c_str(),
                        alpha, pipe);
                #endif
            }
        }
    }

    // draw channel names
    if (m_namesVisible && namesValid()) {
        for (int ch = 0;  ch < m_numChannels;  ++ch) {
            if (m_channelNames[ch].empty()) { continue; }
            int x = m_pdChannelX0 + ch * m_pdChannelDX;
            m_renderer.box(x, m_channelNameBarStartY, x + m_pdChannelWidth, m_screenSizeY,
                           m_config.channelNameUpperColor, m_config.channelNameLowerColor);
            m_renderer.text(float(x) + m_channelNameOffsetX, float(m_channelNameTextY), float(m_pdTextSize),
                            m_channelNames[ch].substr(0, m_pdChannelChars).c_str(), Align::Center,
                            m_config.channelNameTextColor);
        }
    }

    // draw info box
    if (m_infoVisible) {
        m_renderer.box(0, 0, m_metaStartX, m_infoEndY, m_config.infoBackground);
        if (m_infoShadowEndY > m_infoEndY) {
            m_renderer.box(0, m_infoEndY, m_screenSizeX, m_infoShadowEndY, m_config.shadowColor, m_config.shadowColor & 0x00FFFFFFu, false);
        }
        if (trackValid()) {
            m_renderer.text(m_trackX, m_trackY, float(m_trackTextSize), m_track, 0u, m_config.infoTrackColor);
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
        if (m_progSize > 0) {
            if (m_progOuterDXY > 0) {
                m_renderer.box(m_progX0, m_progY0, m_progX1, m_progY1,
                               m_config.progressBorderColor, m_config.progressBorderColor, false, m_progSize);
            }
            m_renderer.box(m_progX0 + m_progOuterDXY, m_progY0 + m_progOuterDXY, m_progX1 - m_progOuterDXY, m_progY1 - m_progOuterDXY,
                           m_config.progressOuterColor, m_config.progressOuterColor, false, m_progSize);
            m_renderer.box(m_progX0 + m_progInnerDXY, m_progY0 + m_progInnerDXY, m_progPosX0 + int(std::min(m_position / m_duration, 1.0f) * float(m_progPosDX) + 0.5f), m_progY1 - m_progInnerDXY,
                           m_config.progressInnerColor, m_config.progressInnerColor, false, m_progSize);
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
            float(m_emptyTextSize), "No module loaded.",
            Align::Center + Align::Middle, m_config.emptyTextColor);
    }

    // draw toast message
    if (!m_toastMessage.empty() && (m_toastAlpha > 0.0f)) {
        int cx = m_screenSizeX >> 1;
        int w = (int(std::ceil(m_renderer.textWidth(m_toastMessage.c_str()) * float(m_toastTextSize))) >> 1) + m_toastDX;
        uint32_t color = m_renderer.extraAlpha(m_config.toastBackgroundColor, m_toastAlpha);
        m_renderer.box(cx - w, m_toastY - m_toastDY, cx + w, m_toastY + m_toastDY, color, color, false, m_toastDY);
        m_renderer.text(float(cx), float(m_toastY), float(m_toastTextSize),
                        m_toastMessage.c_str(), Align::Center + Align::Middle,
                        m_renderer.extraAlpha(m_config.toastTextColor, m_toastAlpha));
        m_toastAlpha -= dt / m_config.toastDuration;
        if (m_toastAlpha <= 0.0f) { m_toastMessage.clear(); }
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

////////////////////////////////////////////////////////////////////////////////

///// module loading

void Application::unloadModule() {
    m_sys.pause();
    m_cancelScanning = true;
    if (m_scanThread) {
        m_scanThread->join();
        delete m_scanThread;
        m_scanThread = nullptr;
    }
    m_scanning = false;
    m_config.loudness = InvalidLoudness;
    {
        AudioMutexGuard mtx_(m_sys);
        delete m_mod;
        m_mod = nullptr;
        m_mod_data.clear();
    }
    m_fullpath.clear();
    m_filename.clear();
    m_track[0] = '\0';
    m_title.clear();
    m_artist.clear();
    m_details.clear();
    m_metadata.clear();
    m_channelNames.clear();
    m_numChannels = 0;
    m_currentPattern = -1;
    m_patternLength = 0;
    m_autoFadeInitiated = true;
    m_sys.setWindowTitle(baseWindowTitle);
    m_escapePressedOnce = false;
    updateLayout(true);
    Dprintf("module unloaded\n");
}

bool Application::loadModule(const char* path, bool forScanning) {
    // unload module
    unloadModule();

    // set filename metadata
    if (path) { m_fullpath.assign(path); }
    bool dirFail = false;
    if (!m_fullpath.empty() && PathUtil::isDir(m_fullpath)) {
        // directory opened -> try to open first file *inside* the directory instead
        std::string newPath(PathUtil::findSibling(m_fullpath + "/", true, m_playableExts.data()));
        if (newPath.empty()) { dirFail = true; }
        else { m_fullpath.assign(newPath); }
    }
    m_filename.assign(PathUtil::basename(m_fullpath));

    // load configuration files
    m_config.reset();
    m_config.load(m_mainIniFile.c_str(), m_filename.c_str());
    m_config.load(PathUtil::join(PathUtil::dirname(m_fullpath), "tmcp.ini").c_str(), m_filename.c_str());
    m_config.load((m_fullpath + ".tmcp").c_str());
    m_config.load(m_cmdline);

    // split off track number
    if (m_config.trackNumberEnabled
    && (m_filename.size() > 3)
    && isDigit(m_filename[0]) && isDigit(m_filename[1])
    && ((m_filename[2] == '-') || (m_filename[2] == '_') || (m_filename[2] == ' '))) {
        m_track[0] = m_filename[0];
        m_track[1] = m_filename[1];
        m_track[2] = '\0';
        m_filename.erase(0, 3);
    }

    // stop here if there's no file to load
    auto fail = [&] (const std::string& msg) -> bool {
        if (!msg.empty()) { Dprintf("loadModule() fail: %s\n", msg.c_str()); }
        m_details.assign(msg);
        updateLayout(true);
        return false;
    };
    if (dirFail) { return fail("directory doesn't contain playable files"); }
    if (m_fullpath.empty()) { return fail(""); }

    // load file into memory
    Dprintf("loading module: %s\n", m_fullpath.c_str());
    FILE *f = fopen(m_fullpath.c_str(), "rb");
    if (!f) { return fail("could not open file"); }
    // fopen() may still succeed on directories, giving us a broken file
    // descriptor with erratic behavior; try to detect this as best as we can
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return fail("invalid file"); }
    size_t size = size_t(ftell(f));
    if (size >= (size_t(-1) >> 1)) { fclose(f); return fail("invalid file"); }
    if (size >= (64u << 20)) { fclose(f); return fail("file too large"); }  // 64 MiB ought to be enough for everybody
    m_mod_data.resize(size);
    fseek(f, 0, SEEK_SET);
    if (fread(m_mod_data.data(), 1, size, f) != size) { fclose(f); return fail("could not read file"); }
    fclose(f);

    // load and setup OpenMPT instance
    AudioMutexGuard mtx_(m_sys);
    std::map<std::string, std::string> ctls;
    ctls["play.at_end"] = (m_config.loop && !forScanning) ? "continue" : "stop";
    switch (m_config.filter) {
        case FilterMethod::Amiga:
            ctls["render.resampler.emulate_amiga"] = "1";
            break;
        case FilterMethod::A500:
            ctls["render.resampler.emulate_amiga"] = "1";
            ctls["render.resampler.emulate_amiga_type"] = "a500";
            break;
        case FilterMethod::A1200:
            ctls["render.resampler.emulate_amiga"] = "1";
            ctls["render.resampler.emulate_amiga_type"] = "a1200";
            break;
        default: break;  // no Amiga resampler -> set later using set_render_param
    }
    try {
        m_mod = new openmpt::module(m_mod_data, std::clog, ctls);
    } catch (openmpt::exception& e) {
        return fail(std::string("invalid module - ") + e.what());
    }
    if (!m_mod) { return fail("invalid module data"); }
    Dprintf("module loaded successfully.\n");
    switch (m_config.filter) {
        case FilterMethod::None:   m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 1); break;
        case FilterMethod::Linear: m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 2); break;
        case FilterMethod::Cubic:  m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 4); break;
        case FilterMethod::Sinc:   m_mod->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, 8); break;
        default: break;  // Auto or Amiga -> no need to set anything up
    }
    m_mod->set_render_param(openmpt::module::render_param::RENDER_STEREOSEPARATION_PERCENT, m_config.stereoSeparation);
    if (!forScanning) {
        float gain = m_config.gain;
        if (isValidLoudness(m_config.loudness)) {
            gain += m_config.targetLoudness - m_config.loudness;
        }
        Dprintf("master gain: %.2f dB\n", gain);
        m_mod->set_render_param(openmpt::module::render_param::RENDER_MASTERGAIN_MILLIBEL, int(gain * 100.0f + 0.5f));
    }

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
        bool firstLine = true;
        size_t start = 0, end;
        do {
            end = msgStr.find('\n', start);
            if (end == std::string::npos) { end = msgStr.size(); }
            size_t realEnd = end;
            while ((realEnd > start) && isSpace(msgStr[realEnd - 1u])) { --realEnd; }
            if (realEnd > start) {
                if (precedingEmptyLine && !firstLine) { msgLines.emplace_back(""); }
                msgLines.emplace_back(msgStr.substr(start, realEnd - start));
                precedingEmptyLine = false;
                firstLine = false;
            } else {
                precedingEmptyLine = true;
            }
            start = end + 1u;
        } while (end != msgStr.size());
        // add lines to metadata block (with wrapping)
        std::string placeholder(m_config.metaMessageWidth, 'x');
        float maxWidth = std::max(meta2.width(), m_renderer.textWidth(placeholder.c_str()) * meta2.defaultSize);
        for (const auto& line : msgLines) {
            m_metadata.addWrappedLine(maxWidth, line);
        }
    }
    m_metadata.ingest(meta2);

    // get channel names
    m_numChannels = m_mod->get_num_channels();
    int ch = 0;
    bool anyNameValid = false;
    for (const auto& name : m_mod->get_channel_names()) {
        if (!name.empty()) { anyNameValid = true; }
        m_channelNames.push_back(name);
        if (++ch >= m_numChannels) { break; }
    }
    if (anyNameValid) {
        while (int(m_channelNames.size()) < m_numChannels) { m_channelNames.push_back(""); }
    } else {
        m_channelNames.clear();
    }

    // done!
    m_sys.setWindowTitle((PathUtil::basename(m_fullpath) + " - " + baseWindowTitle).c_str());
    m_duration = std::max(float(m_mod->get_duration_seconds()), 0.001f);
    m_scrollDuration = std::min(float(m_mod->get_duration_seconds()), m_config.maxScrollDuration);
    m_metaTextAutoScroll = m_config.autoScrollEnabled;
    m_fadeActive = m_autoFadeInitiated = m_endReached = m_cancelScanning = false;
    m_scanning = forScanning;
    updateLayout(true);
    if (m_config.autoPlay && !forScanning) { m_sys.play(); }
    return true;
}

void Application::addMetadataGroup(TextArea& block, const std::vector<std::string>& data, const char* title, bool numbering, int indexStart) {
    int precedingEmptyLine = -1;
    bool titleSent = false;
    int lineIndex = indexStart;
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

////////////////////////////////////////////////////////////////////////////////

///// scan mode

void Application::startScan(const char* specificFile) {
    m_scanning = true;
    std::string modulePath(specificFile ? specificFile : m_fullpath.c_str());
    if (loadModule(modulePath.c_str(), true)) {
        m_scanThread = new std::thread(&Application::runScan, this);
        if (!m_multiScan || m_toastMessage.empty()) {
            // write message, unless the previous file's loudness result is still on screen
            toast("started EBU R128 loudness scan");
        }
    } else {
        m_scanning = false;
        toast("can not perform EBU R128 loudness scan");
    }
}

void Application::runScan() {
    int16_t buffer[scanBufferSize * 2];
    m_config.loudness = InvalidLoudness;
    ebur128_state *r128 = ebur128_init(2, m_sampleRate, EBUR128_MODE_I);
    if (!r128) { return; }
    while (!m_cancelScanning && !m_endReached && m_mod) {
        //m_sys.lockAudioMutex();  // <- would be more correct, but we can also generate deadlocks this way, so don't do it
        size_t count = m_mod->read_interleaved_stereo(m_sampleRate, scanBufferSize, buffer);
        //m_sys.unlockAudioMutex();
        if (!count) {
            m_endReached = true;
            break;
        }
        ebur128_add_frames_short(r128, buffer, count);
    }
    if (m_endReached) {
        double res = InvalidLoudness;
        ebur128_loudness_global(r128, &res);
        m_config.loudness = float(res);
    }
    ebur128_destroy(&r128);
}

void Application::stopScan() {
    m_cancelScanning = true;
    if (!m_scanning) { return; }
    if (m_scanThread) {
        m_scanThread->join();
        delete m_scanThread;
        m_scanThread = nullptr;
    }
    std::string modulePath;
    Dprintf("stopScan(): result loudness = %.2f dB\n", m_config.loudness);
    if (!isValidLoudness(m_config.loudness)) {
        toast("EBU R128 loudness scan cancelled");
    } else if (m_config.saveLoudness((m_fullpath + ".tmcp").c_str())) {
        char message[128];
        snprintf(message, 128, "EBU R128 loudness scan result (%.2f dB) saved", m_config.loudness);
        toast(message);
        if (m_multiScan) {
            // if everything went fine, we may proceed to the next file
            modulePath.assign(PathUtil::findSibling(m_fullpath, true, m_playableExts.data()));
            if (!modulePath.empty()) {
                startScan(modulePath.c_str());
                return;
            }
        }
    } else {
        char message[128];
        snprintf(message, 128, "could not save EBU R128 loudness scan result (%.2f dB)", m_config.loudness);
        toast(message);
    }
    // re-load module in normal mode
    modulePath.assign(m_fullpath);
    loadModule(modulePath.c_str());
}
