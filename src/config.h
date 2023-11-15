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
    int     sampleRate                = 48000;
    int     audioBufferSize           = 512;
    InterpolationMethod interpolation = InterpolationMethod::Auto;
    int     stereoSeparationPercent   = 20;

    uint32_t emptyBackground          = makeRGB(.1f, .2f, .3f);
    uint32_t patternBackground        = emptyBackground;
    uint32_t infoBackground           = makeRGB(.2f, .2f, .2f);
    uint32_t shadowColor              = 0x80000000u;

    int      emptyTextSize            = 32;
    uint32_t emptyTextColor           = 0x80FFFFFFu;

    int      infoMarginX              = 16;
    int      infoMarginY              = 8;
    int      infoLineSpacing          = 4;
    int      infoShadowSize           = 16;
    int      infoTextSize             = 48;
    int      infoDetailsTextSize      = 24;
    uint32_t infoKeyColor             = makeRGB(.9f, .9f, .1f);
    uint32_t infoColonColor           = 0x80FFFFFFu;
    uint32_t infoValueColor           = 0xFFFFFFFFu;
    uint32_t infoDetailsColor         = 0xC0FFFFFFu;

    inline Config() {}
};
