// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

enum class InterpolationMethod {
    None = 0,  //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 1
    Linear,    //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 2
    Cubic,     //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 4
    Sinc,      //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 8
    Amiga,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=auto
    A500,      //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a500
    A1200,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a1200
    Auto       //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 0
};

struct Config {
    bool     fullscreen               = false;
    int      windowWidth              = 1920;
    int      windowHeight             = 1080;
    int      sampleRate               = 48000;
    int      audioBufferSize          = 512;
    InterpolationMethod interpolation = InterpolationMethod::Auto;
    int      stereoSeparationPercent  = 20;
    float    maxScrollDuration        = 4.f * 60.f;
    bool     enableAutoScroll         = true;

    uint32_t emptyBackground          = 0xFF503010u;
    uint32_t patternBackground        = 0xFF503010u;
    uint32_t infoBackground           = 0xFF333333u;
    uint32_t metaBackground           = 0xFF1A1E4Du;
    uint32_t shadowColor              = 0x80000000u;

    int      emptyTextSize            = 32;
    uint32_t emptyTextColor           = 0x80FFFFFFu;

    bool     infoEnabled              = true;
    int      infoMarginX              = 16;
    int      infoMarginY              = 8;
    int      infoLineSpacing          = 4;
    int      infoShadowSize           = 8;
    int      infoTextSize             = 48;
    int      infoDetailsTextSize      = 24;
    uint32_t infoKeyColor             = 0xFF00FFFFu;
    uint32_t infoColonColor           = 0x80FFFFFFu;
    uint32_t infoValueColor           = 0xFFFFFFFFu;
    uint32_t infoDetailsColor         = 0xC0FFFFFFu;

    bool     metaEnabled              = true;
    bool     metaShowMessage          = true;
    bool     metaShowInstrumentNames  = true;
    bool     metaShowSampleNames      = true;
    int      metaMarginX              = 16;
    int      metaMarginY              = 8;
    int      metaTextSize             = 32;
    int      metaShadowSize           = 8;
    int      metaSectionMargin        = 24;
    uint32_t metaHeadingColor         = 0xFF00FFFFu;
    uint32_t metaTextColor            = 0xFFFFFFFFu;
    uint32_t metaIndexColor           = 0xC000FF00u;
    uint32_t metaColonColor           = 0x40FFFFFFu;

    int      patternTextSize          = 32;
    int      patternMinTextSize       = 10;
    int      patternLineSpacing       = 0;
    int      patternMarginX           = 8;
    int      patternBarPaddingX       = 4;
    int      patternBarBorderPercent  = 20;
    uint32_t patternBarBackground     = 0x40FFFFFFu;
    uint32_t patternTextColor         = 0x80FFFFFFu;
    uint32_t patternDotColor          = 0x30FFFFFFu;
    uint32_t patternNoteColor         = 0xFFFFFFFFu;
    uint32_t patternSpecialColor      = 0xFFFFFFC0u;
    uint32_t patternInstrumentColor   = 0xE080FF80u;
    uint32_t patternVolEffectColor    = 0xFFFF8080u;
    uint32_t patternVolParamColor     = 0xFFFF8080u;
    uint32_t patternEffectColor       = 0xC08080FFu;
    uint32_t patternEffectParamColor  = 0xC08080FFu;
    uint32_t patternPosOrderColor     = 0x80F0FFFFu;
    uint32_t patternPosPatternColor   = 0x80FFFFEFu;
    uint32_t patternPosRowColor       = 0x80FFF0FFu;
    uint32_t patternPosDotColor       = 0x40FFFFFFu;
    uint32_t patternSepColor          = 0x10FFFFFFu;
    float    patternAlphaFalloff      = 1.0f;
    float    patternAlphaFalloffShape = 1.5f;

    inline Config() {}
};
