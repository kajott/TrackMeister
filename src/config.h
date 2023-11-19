// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

constexpr inline uint32_t makeRGB(int r, int g, int b) {
    return  ((r < 0) ? 0u : (r > 255) ? 255u : uint32_t(r))
         | (((g < 0) ? 0u : (g > 255) ? 255u : uint32_t(g)) <<  8)
         | (((b < 0) ? 0u : (b > 255) ? 255u : uint32_t(b)) << 16)
         | 0xFF000000u;
}

constexpr inline uint32_t makeRGBA(int r, int g, int b, int a) {
    return  ((r < 0) ? 0u : (r > 255) ? 255u : uint32_t(r))
         | (((g < 0) ? 0u : (g > 255) ? 255u : uint32_t(g)) <<  8)
         | (((b < 0) ? 0u : (b > 255) ? 255u : uint32_t(b)) << 16)
         | (((a < 0) ? 0u : (a > 255) ? 255u : uint32_t(a)) << 24);
}

constexpr inline uint32_t makeRGB(float r, float g, float b) {
    return  ((r <= 0.f) ? 0u : (r >= 1.f) ? 255u : uint32_t(r * 255.f + .5f))
         | (((g <= 0.f) ? 0u : (g >= 1.f) ? 255u : uint32_t(g * 255.f + .5f)) <<  8)
         | (((b <= 0.f) ? 0u : (b >= 1.f) ? 255u : uint32_t(b * 255.f + .5f)) << 16)
         | 0xFF000000u;
}

constexpr inline uint32_t makeRGBA(float r, float g, float b, float a) {
    return  ((r <= 0.f) ? 0u : (r >= 1.f) ? 255u : uint32_t(r * 255.f + .5f))
         | (((g <= 0.f) ? 0u : (g >= 1.f) ? 255u : uint32_t(g * 255.f + .5f)) <<  8)
         | (((b <= 0.f) ? 0u : (b >= 1.f) ? 255u : uint32_t(b * 255.f + .5f)) << 16)
         | (((a <= 0.f) ? 0u : (a >= 1.f) ? 255u : uint32_t(a * 255.f + .5f)) << 24);
}

enum class InterpolationMethod {
    None = 0,  //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 1
    Auto,      //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 0
    Linear,    //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 2
    Cubic,     //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 4
    Sinc,      //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 8
    Amiga,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=auto
    A500,      //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a500
    A1200,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a1200
    Default = Auto
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

    uint32_t emptyBackground          = makeRGB(.1f, .2f, .3f);
    uint32_t patternBackground        = emptyBackground;
    uint32_t infoBackground           = makeRGB(.2f, .2f, .2f);
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
    uint32_t infoKeyColor             = makeRGB(1.f, 1.f, 0.f);
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
    uint32_t metaHeadingColor         = makeRGB(1.f, 1.f, 0.f);
    uint32_t metaTextColor            = 0xFFFFFFFFu;
    uint32_t metaIndexColor           = makeRGBA(0.f, 1.f, 0.f, .75f);
    uint32_t metaColonColor           = 0x40FFFFFFu;

    int      patternTextSize          = 32;
    int      patternMinTextSize       = 10;
    int      patternLineSpacing       = 0;
    int      patternMarginX           = 8;
    int      patternBarPaddingX       = 4;
    int      patternBarBorderPercent  = 20;
    uint32_t patternBarBackground     = 0x40FFFFFFu;
    uint32_t patternTextColor         = 0xC0FFFFFFu;
    uint32_t patternDotColor          = 0x40FFFFFFu;
    uint32_t patternNoteColor         = 0xFFFFFFFFu;
    uint32_t patternSpecialColor      = patternNoteColor;
    uint32_t patternInstrumentColor   = makeRGBA(.5f, 1.f, .5f, .875f);
    uint32_t patternVolEffectColor    = makeRGBA(.5f, .5f, 1.f, 1.f);
    uint32_t patternVolParamColor     = patternVolEffectColor;
    uint32_t patternEffectColor       = makeRGBA(1.f, .5f, .5f, .75f);
    uint32_t patternEffectParamColor  = patternEffectColor;
    uint32_t patternPosOrderColor     = patternTextColor;
    uint32_t patternPosPatternColor   = patternTextColor;
    uint32_t patternPosRowColor       = patternTextColor;
    uint32_t patternPosDotColor       = 0x80FFFFFFu;
    uint32_t patternSepColor          = 0x20FFFFFFu;
    float    patternAlphaFalloff      = 1.0f;
    float    patternAlphaFalloffShape = 1.5f;

    inline Config() {}
};
