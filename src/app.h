// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#define USE_PATTERN_CACHE 1

#include <cstdint>
#include <cstddef>

#include <utility>
#include <vector>
#include <string>
#if USE_PATTERN_CACHE
    #include <unordered_map>
#endif
#include <thread>
#include <atomic>

#include "system.h"
#include "renderer.h"
#include "textarea.h"
#include "pathutil.h"
#include "numset.h"
#include "config.h"

namespace openmpt {
    class module;
}

class Application {
    // core data
    SystemInterface& m_sys;
    TextBoxRenderer m_renderer;
    int m_sampleRate;
    bool m_scanning = false;
    bool m_loop = false;
    std::atomic_bool m_cancelScanning = false;
    std::atomic_bool m_clipped = false;
    openmpt::module* m_mod = nullptr;
    std::vector<std::byte> m_mod_data;
    std::vector<uint32_t> m_playableExts;
    std::thread* m_scanThread = nullptr;
    float m_instanceGain = 0.0f;

    // configuration
    std::string m_mainIniFile;
    std::string m_dirIniFile;
    std::string m_fileIniFile;
    Config m_globalConfig;
    Config m_fileConfig;
    Config m_cmdlineConfig;
    Config m_uiGlobalConfig;
    Config m_uiFileConfig;
    Config m_config;
    NumberSet m_fileReloadPending;
    NumberSet m_globalReloadPending;
    NumberSet m_restartPending;
    NumberSet m_uiGlobalReset;
    NumberSet m_uiFileReset;

    // metadata
    std::string m_fullpath;
    std::string m_basename;
    std::string m_previousFile;
    char m_track[3] = {0,0,0};
    std::vector<std::pair<std::string, std::string>> m_info;
    std::vector<std::string> m_shortDetails;
    std::vector<std::string> m_longDetails;
    std::string m_details;  // final string compiled in updateLayout()
    std::vector<std::string> m_channelNames;
    TextArea m_metadata;
    float m_duration, m_scrollDuration;

    // current position and size information
    int m_currentOrder;
    int m_currentPattern;
    int m_currentRow;
    int m_numChannels;
    int m_patternLength;
    float m_position;
    std::atomic<bool> m_endReached;

    // computed layout information (from updateLayout())
    int m_screenSizeX, m_screenSizeY;
    int m_emptyTextSize, m_emptyTextPos;
    int m_trackTextSize, m_infoTextSize, m_infoDetailsSize;
    int m_infoEndY, m_infoShadowEndY;
    float m_trackX, m_trackY;
    int m_infoStartY, m_infoLineSpacing, m_infoKeyX, m_infoValueX;
    int m_infoFilenameY, m_infoArtistY, m_infoTitleY, m_infoDetailsY;
    int m_progX0, m_progY0, m_progX1, m_progY1;
    int m_progOuterDXY, m_progInnerDXY, m_progSize;
    int m_progPosX0, m_progPosDX;
    int m_metaStartX, m_metaShadowStartX;
    float m_metaTextX, m_metaTextMinY, m_metaTextMaxY;
    int m_pdPosChars, m_pdChannelChars;
    int m_pdTextSize, m_pdTextY0, m_pdTextDY, m_pdRows;
    int m_pdPosX, m_pdChannelX0, m_pdChannelDX;
    float m_pdPipeDX;
    int m_pdNoteWidth, m_pdChannelWidth;
    int m_pdBarStartX, m_pdBarEndX, m_pdBarRadius;
    int m_toastTextSize, m_toastY, m_toastDX, m_toastDY;
    int m_channelNameBarStartY, m_channelNameTextY;
    float m_channelNameOffsetX, m_vuHeight;
    int m_clipX0, m_clipY0, m_clipX1, m_clipY1;

    // background image and logo data (texture, layout)
    struct ExternalImage {
        std::string path;
        int64_t mtime = 0;
        TextBoxRenderer::TextureDimensions size = {0,0};
        unsigned tex = 0;
        int x0, y0, x1, y1;
    };
    ExternalImage m_background, m_logo;
    unsigned m_defaultLogoTex = 0;
    TextBoxRenderer::TextureDimensions m_defaultLogoSize = {0,0};
    unsigned m_usedLogoTex = 0;

    // current view/playback state
    float m_metaTextY, m_metaTextTargetY;
    bool m_metaTextAutoScroll = true;
    bool m_infoVisible, m_metaVisible, m_namesVisible, m_vuVisible;
    bool m_fadeActive = false;
    int m_fadeGain, m_fadeRate;
    bool m_autoFadeInitiated = false;
    bool m_multiScan = false;
    bool m_mayAutoAdvance = false;
    bool m_escapePressedOnce = false;
    float m_clipAlpha = 0.0f;

    // pattern data cache
    struct CacheItem { char text[16], attr[16]; };
    #if USE_PATTERN_CACHE
        using CacheKey = uint32_t;
        static inline CacheKey makeCacheKey(int pattern, int row, int channel)
            { return uint32_t((pattern << 20) ^ (row << 10) ^ channel); }
        std::unordered_map<CacheKey, CacheItem> m_patternCache;
    #endif

    // toast message
    std::string m_toastMessage;
    float m_toastAlpha;

    // debug/config UI
    bool m_showDemo = false;
    bool m_showHelp = false;
    bool m_showConfig = false;
    bool m_uiConfigShowGlobal = true;

public:  // interface from SystemInterface
    explicit inline Application(SystemInterface& sys) : m_sys(sys), m_metadata(m_renderer) {}

    int init(int argc, char* argv[]);
    void draw(float dt);
    void shutdown();

    bool renderAudio(int16_t* data, int sampleCount, bool stereo, int sampleRate);

    void handleKey(int key, bool ctrl=false, bool shift=false, bool alt=false);
    void handleDropFile(const char* path);
    void handleResize(int w, int h);
    void handleMouseWheel(int delta);

private:  // business logic
    std::string findPlayableSibling(const std::string& base, PathUtil::FindMode mode);
    static bool hasTrackNumber(const char* basename);
    void unloadModule();
    bool loadModule(const char* path, bool forScanning=false);
    bool loadNextModule(bool reverse=false);
    void changeInstanceGain(float delta);
    void updateGain();
    void cycleBoxVisibility();
    int toPixels(int value) const;
    int toTextSize(int value) const;
    int textWidth(int size, const char* text) const;
    void reloadConfig();
    void updateConfig();
    void updateImages();
    void updateImage(ExternalImage& img, const std::string& path, int channels, const char* what);
    void updateLayout(bool resetBoxVisibility=false);
    void formatPatternDataCell(CacheItem& dest, int pat, int row, int ch) const;
    void drawPatternDisplayCell(float x, float y, const char* text, const char* attr, float alpha=1.0f, bool pipe=true);
    static void formatPosition(int order, int pattern, int row, char* text, char* attr, int size);
    void addMetadataGroup(TextArea& block, const std::vector<std::string>& data, const char* title, bool numbering=true, int indexStart=1);
    void setMetadataScroll(float y);
    inline bool trackValid() const { return (m_track[0] != '\0'); }
    inline bool infoValid()  const { return !m_info.empty() || !m_details.empty(); }
    inline bool metaValid()  const { return !m_metadata.empty(); }
    inline bool namesValid() const { return !m_channelNames.empty(); }
    void toast(const char* msg);
    inline void toast(const std::string& msg) { toast(msg.c_str()); }
    void toastVersion();
    void toastPosition();
    void fadeOut();
    void startScan(const char* specificFile=nullptr);
    void runScan();
    void stopScan();
    void uiHelpWindow();
    void uiConfigWindow();
    void uiSaveConfig();
};
