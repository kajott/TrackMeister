// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <list>
#include <string>

enum class FilterMethod {
    None = 0,  //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 1
    Linear,    //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 2
    Cubic,     //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 4
    Sinc,      //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 8
    Amiga,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=auto
    A500,      //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a500
    A1200,     //!< OpenMPT render.resampler.emulate_amiga=1, emulate_amiga_type=a1200
    Auto       //!< OpenMPT INTERPOLATIONFILTER_LENGTH = 0
};

//! value that designates that no loudness has been measured yet
constexpr float InvalidLoudness = -999.0f;
//! check whether a loudness value is valid
constexpr bool isValidLoudness(float db) { return (db > -100.0f); }

//! TrackMeister default configuration.
//!
//! Unless explicitly noted otherwise, the unit for all lengths (text sizes,
//! margins etc.) is 1/1000th of the screen or window height, i.e. roughly
//! in the ballpark of (but *not* identical to!) pixels on a 1080p display.
//! All sizes will be rounded to full display pixels internally.
//!
//! In the INI file, colors are specified in hexadecimal RGB HTML/CSS notation,
//! e.g. #123abc or #f00 (which is equivalent to #ff0000). An optional fourth
//! component specifies alpha, with 00 = fully transparent and ff = fully
//! opaque. For example, #ff000080 is half-transparent red. (This is consistent
//! with the notation used e.g. by Inkscape.) If alpha is not specified, the
//! color is assumed to be fully opaque.
//! In code, all colors are in the format 0xAABBGGRRu.
struct Config {
    bool     fullscreen               = false;        //!< whether to run in fullscreen mode
    int      windowWidth              = 1920;         //!< initial window width  in non-fullscreen mode, in pixels
    int      windowHeight             = 1080;         //!< initial window height in non-fullscreen mode, in pixels

    int      sampleRate               = 48000;        //!< audio sampling rate
    int      audioBufferSize          = 512;          //!< size of the audio buffer, in samples; if there are dropouts, try doubling this value
    FilterMethod filter        = FilterMethod::Auto;  //!< audio resampling filter to be used
    int      stereoSeparation         = 50;           //!< amount of stereo separation, in percent (0 = mono, 100 = full stereo, higher = fake surround)
    int      volumeRamping            = -1;           //!< volume ramping strength (0 = no ramping, 10 = softest ramping, -1 = recommended default)
    float    gain                     = 0.0f;         //!< global gain to apply, in decibels
    float    loudness             = InvalidLoudness;  //!< the current track's measured loudness, in decibels; values < -100 mean "no loudness measured"
    float    targetLoudness           = -18.0f;       //!< target loudness, in decibels (or LUFS); if the 'loudness' parameter is valid, an extra gain will be applied (in addition to 'gain') so that the loudness is corrected to this value

    bool     autoPlay                 = true;         //!< automatically start playing when loading a module; you may want to turn this off for actual competitions
    bool     loop                     = false;        //!< whether to loop the song after it's finished, or play the song's programmed loop if it there is one
    bool     fadeOutAfterLoop         = false;        //!< whether to trigger a slow fade-out after the song looped
    float    fadeOutAt                = 0.0f;         //!< number of seconds after which the song shall be slowly faded out automatically (0 = no auto-fade)
    float    fadeDuration             = 10.f;         //!< duration of a fade-out, in seconds

    bool     autoScrollEnabled        = true;         //!< whether to enable automatic scrolling in the metadata sidebar after loading a module
    float    maxScrollDuration        = 4.f * 60.f;   //!< maximum duration after which automatic metadata scrolling reaches the end, in seconds; if the module is shorter than that, the module's duration will be used instead

    uint32_t emptyBackground          = 0xFF503010u;  //!< background color of "no module loaded" screen
    uint32_t patternBackground        = 0xFF503010u;  //!< background color of pattern display
    uint32_t infoBackground           = 0xFF333333u;  //!< background color of the top information bar
    uint32_t metaBackground           = 0xFF1A1E4Du;  //!< background color of the metadata sidebar
    uint32_t shadowColor              = 0x80000000u;  //!< color of the info and metadata bar's shadows

    bool     logoEnabled              = true;         //!< whether to show a logo at all
    std::string logo;                                 //!< custom logo file; must be a grayscale PNG file with high-contrast black-on-white artwork; will be downscaled by a power of two so it fits into the canvas
    int      logoMargin               = 8;            //!< minimum distance between the logo image and the surrounding screen or panel edges
    int      logoPosX                 = 50;           //!< horizontal logo position, in percent of the available area (0 = left, 50 = center, 100 = right)
    int      logoPosY                 = 50;           //!< vertical logo position, in percent of the available area (0 = top, 50 = center, 100 = bottom)

    int      emptyTextSize            = 32;           //!< size of the "no module loaded" text
    int      emptyLogoPosY            = 500;          //!< vertical position of the center of the logo on the "no module loaded" screen
    int      emptyTextPosY            = 900;          //!< vertical position of the "no module loaded" text
    uint32_t emptyTextColor           = 0x80FFFFFFu;  //!< color of the "no module loaded" text
    uint32_t emptyLogoColor           = 0x80FFFFFFu;  //!< logo color on the "no module loaded" screen

    bool     infoEnabled              = true;         //!< whether to enable the top information bar by default after loading a module
    bool     trackNumberEnabled       = true;         //!< whether to extract and display the track number from the filename; used if the filename starts with two digits followed by a dash (-), underscore (_) or space
    int      infoMarginX              = 16;           //!< outer left margin inside the info bar
    int      infoMarginY              = 8;            //!< upper and lower margin inside the info bar
    int      infoTrackTextSize        = 233;          //!< text size of the track number
    int      infoTextSize             = 48;           //!< text size of the filename, title and artist lines
    int      infoDetailsTextSize      = 24;           //!< text size of the technical details line
    int      infoLineSpacing          = 4;            //!< extra space between the info bar's lines
    int      infoTrackPaddingX        = 24;           //!< horitontal space between the track number and the other information in the info bar
    int      infoKeyPaddingX          = 8;            //!< horizontal space between the "File", "Artist" and "Title" heading and the content text
    uint32_t infoTrackColor           = 0xFF00C0FFu;  //!< color of the track number
    uint32_t infoKeyColor             = 0xC000FFFFu;  //!< color of the "File", "Artist" and "Title" headings
    uint32_t infoColonColor           = 0x40FFFFFFu;  //!< color of the colon following the headings
    uint32_t infoValueColor           = 0xFFFFFFFFu;  //!< color of the file, artist and title texts
    uint32_t infoDetailsColor         = 0xC0FFFFFFu;  //!< color of the technical details line
    int      infoShadowSize           = 8;            //!< width of the shadow below the info bar

    bool     progressEnabled          = true;         //!< whether to show a progress bar
    int      progressHeight           = 16;           //!< height ("thickness") of the progress bar
    int      progressMarginTop        = 4;            //!< extra space to insert above the progress bar
    int      progressBorderSize       = 2;            //!< size/thickness/width of the progress bar's border (0 = no border)
    int      progressBorderPadding    = 2;            //!< inside padding between the actual progress indicator and the progress bar's border
    uint32_t progressBorderColor      = 0xFF888888u;  //!< color of the progress bar's border
    uint32_t progressOuterColor       = 0xFF222222u;  //!< color of the progress bar's empty area (note: this is drawn on top of the border, so be careful with alpha!)
    uint32_t progressInnerColor       = 0xFF888888u;  //!< color of the actual progress indicator (note: this is drawn on top of the other two progress bar elements, so be careful with alpha!)

    bool     metaEnabled              = true;         //!< whether to enable the metadata sidebar by default after loading a module
    bool     metaShowMessage          = true;         //!< whether the metadata sidebar shall include the module message section
    bool     metaShowInstrumentNames  = true;         //!< whether the metadata sidebar shall include the instrument names section
    bool     metaShowSampleNames      = true;         //!< whether the metadata sidebar shall include the sample names section
    int      metaMarginX              = 16;           //!< left and right margin inside the metadata sidebar
    int      metaMarginY              = 8;            //!< upper and lower margin inside the metadata sidebar
    int      metaTextSize             = 32;           //!< text size in the metadata sidebar
    int      metaMessageWidth         = 32;           //!< approximate number of characters per line to allocate for the module message
    int      metaSectionMargin        = 32;           //!< vertical gap between sections in the metadata sidebar
    uint32_t metaHeadingColor         = 0xFF00FFFFu;  //!< color of a section heading in the metadata sidebar
    uint32_t metaTextColor            = 0xFFFFFFFFu;  //!< color of normal text in the metadata sidebar
    uint32_t metaIndexColor           = 0xC000FF00u;  //!< color of the instrument/sample numbers in the metadata sidebar
    uint32_t metaColonColor           = 0x40FFFFFFu;  //!< color of the colon between instrument/sample number and name in the metadata sidebar
    int      metaShadowSize           = 8;            //!< width of the shadow left to the the metadata sidebar

    int      patternTextSize          = 32;           //!< desired size of the pattern display text
    int      patternMinTextSize       = 10;           //!< minimum allowed size of the pattern display text (if the pattern still doesn't fit with this, some channels won't be visible)
    int      patternLineSpacing       = 0;            //!< extra vertical gap between rows in the pattern display
    int      patternMarginX           = 8;            //!< left and right margin inside the pattern display
    int      patternBarPaddingX       = 4;            //!< extra left and right padding of the current row bar in the pattern display
    int      patternBarBorderPercent  = 20;           //!< border radius of the current row bar, in percent of the text size
    uint32_t patternLogoColor         = 0x18000000u;  //!< color of the background logo
    uint32_t patternBarBackground     = 0x40FFFFFFu;  //!< fill color of the current row bar
    uint32_t patternTextColor         = 0x80FFFFFFu;  //!< color of normal text in the pattern display (not used, as everything in the pattern display is covered by the following highlighting colors)
    uint32_t patternDotColor          = 0x30FFFFFFu;  //!< text color of the dots indicating unset notes/instruments/effects etc.
    uint32_t patternNoteColor         = 0xFFFFFFFFu;  //!< text color of normal notes (e.g. "G#4")
    uint32_t patternSpecialColor      = 0xFFFFFFC0u;  //!< text color of special notes (e.g. "===")
    uint32_t patternInstrumentColor   = 0xE080FF80u;  //!< text color of the instrument/sample index column
    uint32_t patternVolEffectColor    = 0xFFFF8080u;  //!< text color of the volume effect column (e.g. the 'v' before the volume)
    uint32_t patternVolParamColor     = 0xFFFF8080u;  //!< text color of the volume effect parameter column
    uint32_t patternEffectColor       = 0xC08080FFu;  //!< text color of the effect type column
    uint32_t patternEffectParamColor  = 0xC08080FFu;  //!< text color of the effect parameter column
    uint32_t patternPosOrderColor     = 0x80F0FFFFu;  //!< text color of the order number
    uint32_t patternPosPatternColor   = 0x80FFFFEFu;  //!< text color of the pattern number
    uint32_t patternPosRowColor       = 0x80FFF0FFu;  //!< text color of the row number
    uint32_t patternPosDotColor       = 0x40FFFFFFu;  //!< text color of the colon or dot between the order/pattern/row numbers
    uint32_t patternSepColor          = 0x10FFFFFFu;  //!< text color of the bar ('|') between channels
    float    patternAlphaFalloff      = 1.0f;         //!< amount of alpha falloff for the outermost rows in the pattern display; 0.0 = no falloff, 1.0 = falloff to full transparency
    float    patternAlphaFalloffShape = 1.5f;         //!< shape (power) of the alpha falloff in the pattern display; the higher, the more rows will retain a relatively high opacity

    bool     channelNamesEnabled      = true;         //!< whether to enable the channel name displays by default after loading a module
    int      channelNamePaddingY      = 0;            //!< extra vertical padding in the channel name boxes
    uint32_t channelNameUpperColor    = 0x00000000u;  //!< color of the upper end of the channel name boxes
    uint32_t channelNameLowerColor    = 0xFF000000u;  //!< color of the lower end of the channel name boxes
    uint32_t channelNameTextColor     = 0xFFC0FF40u;  //!< channel name text color

    bool     vuEnabled                = true;         //!< whether to enable the fake VU meters by default after loading a module
    int      vuHeight                 = 200;          //!< height of the fake VU meters
    uint32_t vuUpperColor             = 0x10FF80FFu;  //!< color of the upper end of the fake VU meters
    uint32_t vuLowerColor             = 0x50FF00FFu;  //!< color of the lower end of the fake VU meters

    int      toastTextSize            = 24;           //!< text size of a "toast" status message
    int      toastMarginX             = 0;            //!< left and right margin inside a "toast" status message (not including the rounded borders)
    int      toastMarginY             = 6;            //!< top and bottom margin inside a "toast" status message
    int      toastPositionY           = 800;          //!< vertical position of a "toast" status message, relative to the top of the display
    uint32_t toastBackgroundColor     = 0xFF404040u;  //!< background color of a "toast" status message
    uint32_t toastTextColor           = 0xFFFFFFFFu;  //!< text color of a "toast" status message
    float    toastDuration            = 2.0f;         //!< time a "toast" status message shall be visible until it's completely faded out

    inline Config() {}
    inline void reset() { Config defaultConfig; *this = defaultConfig; }
    using PreparedCommandLine = std::list<std::string>;
    static PreparedCommandLine prepareCommandLine(int& argc, char** argv);
    void load(const PreparedCommandLine& cmdline);
    bool load(const char* filename, const char* matchName=nullptr);
    bool save(const char* filename);
    bool saveLoudness(const char* filename);
};
