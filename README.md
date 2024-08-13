![TrackMeister](logo/tm_logo.svg)

This application is a player for [tracker music files](https://en.wikipedia.org/wiki/Module_file) ("module" files) as they were &ndash;and still are&ndash; common in the [demoscene](https://en.wikipedia.org/wiki/Demoscene). With its fullscreen interface and limited interaction options, it's specifically targeted towards presenting tracked music in a competition ("compo") or for random background playback.


## Features

- based on [libopenmpt](https://lib.openmpt.org/libopenmpt/), a very well-regarded library for module file playback
- plays all common formats: MOD, XM, IT, S3M, and a dozen others
- fullscreen interface based on OpenGL
- metadata display (title, artist, technical info, module comment, instrument and sample names) with smooth scrolling
- pattern display for visualization, including channel names (if present in the module file)
- optional custom logo in the background of the pattern display
- fake VU meters (based on note velocity and channel, not the actual audio samples)
- smooth fade out (triggered manually, automatically after looping, or after a configurable time)
- loudness normalization, with a built-in EBU R128 (a.k.a. ReplayGain 2.0) loudness analyzer
- cross-platform (tested on Windows and Linux)
- single executable; no extra DLLs/`.so`s needed
- open source (MIT license)


## Usage

Just drag a module file onto the executable, or into the window once the player has already been started. If a directory is opened this way, the first playable file therein is loaded. Note that this is *not* recursive; TrackMeister won't play entire directory hierarchies. If no file is specified upfront, but there are playable files in the current directory when TrackMeister starts, the (lexicographically) first file is loaded.

The screen is split into three parts: The pattern display, the info bar at the top (containing basic information about the module format as well as filename, title and artist), and the metadata bar at the right (with the free-text module message, instrument and sample names). If the content doesn't fit in the metadata bar, it slowly scrolls down during playback, so it reaches the bottom end at the end of the track or after four minutes, whatever comes first.

If a module's filename starts with exactly two decimal digits, followed by a space, dash (`-`) or underscore (`_`), the digits will be removed from the filename display and shown as a "track number" in a much larger font in the info bar instead. This can be used to curate playlists by simply prefixing each filename with `01_`, `02_` and so on.

The following other controls are available:

| Input | Action |
|-------|--------|
| **Q** or **Alt+F4** | quit the application
| **Esc** | cancel current operation: <br> - stops loudness scan if one is running, <br> - pauses audio if it's playing, <br> - quits the application after pressing _twice_ otherwise
| **Space** | pause / continue playback
| **Tab** | show / hide the info and metadata bars
| **Enter** | show / hide the fake VU meters
| **N** | show / hide the channel name display
| Cursor **Left** / **Right** | seek backward / forward one order
| **Page Up** | load previous module in the directory
| **Page Down** | load next module in the directory
| **Ctrl+Home** | load first module in the directory
| **Ctrl+End** | load last module in the directory
| Mouse Wheel | manually scroll through the metadata bar (stops autoscrolling)
| **A** | stop / resume autoscrolling
| **F** | slowly fade out the song
| **V** | show the TrackMeister and libopenmpt version numbers
| **F5** | reload the current module and the application's configuration
| **F11** | toggle fullscreen mode
| **+** / **-** | adjust volume; this adjustment will _not_ be saved (i.e. restarting TrackMeister will start with the default volume again); furthermore, making the sound louder can lead to audio distortion
| **Ctrl+L** | start (or cancel) EBU R128 loudness scan for the currently loaded module
| **Ctrl+Shift+L** | start EBU R128 loudness scan for the currently loaded module and all following modules in the current directory <br> (this ignores shuffle mode; it's recommended to press **Ctrl+Home** first!)
| **Ctrl+Shift+S** | save `tm_default.ini` (see below)

For directory navigation, "previous" and "next" refer to case-insensitive lexicographical ordering.

To use the loudness normalization feature, perform a loudness scan on the desired module(s); this will write a small `.tm` file next to the module file that contains the measured EBU R128 loudness for the currently set up rendering parameters (i.e. filter, stereo separation etc.). The next time that module is loaded, TrackMeister picks up this loudness value and automatically computes a suitable gain to normalize the volume levels to a target of -18 LUFS. (The target can be adjusted with the `target loudness` configuration setting.)


## Configuration

TrackMeister can be configured using configuration files with an INI-like syntax.

The following aspects can be configured:
- display colors, font sizes and font
  - font selection is limited to a few "baked-in" presets
- windowed/fullscreen mode and window size (*)
- audio sample rate and buffer size (*)
- audio interpolation filter
- amount of stereo separation

All items can be changed at runtime when loading a new module or pressing the **F5** key, _except_ those marked with an asterisk (*); these are only evaluated once on startup.

The following locations are searched for configuration files:
- `tm.ini` in the program's directory (i.e. directly next to `tm.exe`)
- `tm.ini` in the currently opened module file's directory
- a file with the same name as the currently opened module file, but with an extra suffix of `.tm`; for example, for `foo.mod`, the configuration file will be `foo.mod.tm`

Configuration files are processed in this exact order, line by line. Options specified later override options specified earlier.

The configuration files can contain multiple sections, delimited by lines containing the section name in square brackets, `[like so]`. The following sections are evaluated, and all other sections are ignored:
- the unnamed section at the beginning of the file
- the `[TrackMeister]` or `[TM]` sections
- sections that match the current module's file name, e.g. `[foo*.mod]`;
  the following rules apply for those:
  - only the filename is matched, no directory names
  - matching is case-insensitive
  - exactly one '`*`' may be used as a wildcard (no '`?`'s, no multiple '`*`'s)

All other lines contain key-value pairs of the form "`key = value`" or "`key: value`". Spaces, dashes (`-`) and underscores (`_`) in key names are ignored. All parts of a line following a semicolon (`;`) are ignored. It's allowed to put comments at the end of key/value lines.

To get a list of all possible settings, along with documentation and the default values for each setting, run TrackMeister normally and press **Ctrl+Shift+S**, or run `tm --save-default-config` from a command line. This will generate a file `tm_default.ini` in the current directory (usually the program directory) that also be used as a template for an individual configuration.

All sizes (font sizes, margins etc.) are specified in 1/1000s of the display width, so they are more or less resolution-independent. <br>
Boolean values can use any of the `1`/`0`, `false`/`true`, `yes`/`no` or `enabled`/`disabled` nomenclatures. <br>
Colors are specified in HTML/CSS-style hexadecimal RGB notation, but with optional alpha, i.e. in the form `#rrggbb`, `#rrggbbaa`, `#rgb` or `#rgba`.

Here's an example for a useful INI file as a compo organizer would set it up for hosting a competition:

    [TM]
    ; generic options, specified in classic INI syntax

    ; when hosting an actual competition, we want to start playback manually
    autoplay=disabled
    ; and we want fullscreen too!
    fullscreen=yes
    ; if the track has a loop, we want to fade out automatically at this point
    fade out after loop = true

    [*.mod]
        ; this section is only active for MOD format files, which tend to be
        ; written with little stereo separation and no volume ramping in mind
        stereo-separation: 20;
        volume-ramping: 0;

    [03*]
        ; the entry with the track number 3 in the filename has a loop;
        ; we want to play the loop, but fade out directly after it;
        ; the fade has been configured above already, now set up the loop:
        loop: true

Note that in practice, the track-specific options would rather be written into the `tm.ini` file in the directory where the entries reside, or the even into the `.tm` file next to the module file itself, and maybe not into the "global" config file that's located next to `tm.exe`.

Another example is this INI file for a typical "random module jukebox" setup:

    [TM]
    ; play all files continuously in random order
    autoplay = yes
    auto advance = yes
    shuffle = yes
    ; and in fullscreen, of course!
    fullscreen = true

Options can also be specified on the command line, in the syntax "`+key=value`" or "`+key:value`". No extra spaces are allowed around the value. Command-line options take precedence over all configuration files.


## FAQ

**Q:** TrackMeister quits with an OpenGL error message or runs extremely slowly on Windows. What can I do? <br>
**A:** This is most likely due to an improperly installed graphics driver. Windows Update automatically installs drivers, but those are often incomplete and sometimes lack OpenGL support. Please install the proper driver from [AMD](https://www.amd.com/en/support/download/drivers.html), [Intel](https://www.intel.com/content/www/us/en/support/detect.html) or [NVidia](https://www.nvidia.com/en-us/drivers/), depending on your hardware.

**Q:** Can I run TrackMeister on a Raspberry Pi? <br>
**A:** Turns out, you can! You will need a Raspberry Pi 4 or 5 and some magical incantations to run it, but it's possible. You need to paste the following line in a terminal: <br>
`    export MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330`
<br> and then run TrackMeister from there (e.g. `./tm`).

**Q:** OK, so what do I need to do to host a demoparty competition with TrackMeister? <br>
**A:** The most basic flow is as follows:
- Put `tm.exe` and the contestant's module files into a directory.
- Create a `tm.ini` file with general options in the `[TM]` section. The chapter above has a nice example to get started with.
- Listen through the files (simply double-clicking `tm.exe` should suffice). Make mental or written notes about the play order as you want to have it in the compo.
- Rename the module files by prepending exactly two digits with the running order number, followed by a space, underscore or dash. For example, if you want to play `pt.mod` at position 8 in the compo, it becomes `08_pt.mod`, `08 pt.mod` or `08-pt.mod`.
- Listen to the tracks again and set track-specific options in `tm.ini`. For each track you want to set special options for, you create a new section based on the running order, e.g. `[08*]` for the options of the file in the aforementioned example. <br> Typical per-track options are:
  - If the track shall loop, set `loop = true`. If you also set `fade out after loop = true`, the track will also fade out at this point.
  - If the track shall fade out at some position, set `fade out at =` to the number of seconds after which fading shall start (e.g. `fade out at = 123` fades out after 123 seconds, i.e. 2 minutes and 3 seconds).
  - If the track shall be played in mono, set `stereo separation = 0`.
  - If the track shall be played with some volume ramping, set `volume ramping = 10`; if it shall not use ramping, set `volume ramping = 0`.
  - You can also override the track's title/artist metadata by setting it manually, e.g. `title = Professional Tracker` and `artist = H0ffman feat. Daytripper`.
  - If the instrument/sample texts in the sidebar are utterly uninteresting, you can hide the sidebar completely with `meta enabled = false`.
  - In general, you can override almost every option that TrackMeister has! Custom colors per entry? No problem. Or even an image? Sure. Have a look into `tm_default.ini` for a list of all possible settings.
  - When tuning per-track options, it can be useful to put TrackMeister into windowed mode if it was running fullscreen (**F11**), have an editor with `tm.ini` open and use the **F5** key to re-load the configuration and re-start the currently playing track.
  - Do **not** set a per-track `gain` (volume) yet; we're going to use TrackMeister's assistance for that!
- When done with the per-track configuration, navigate to track 1 and press **Ctrl**+**Shift**+**L**. This will start loudness measurement of the entire playlist. After restarting TrackMeister, the tracks should play a bit quieter on average, but without loudness jumps between tracks.
  - If you really want to fine-tune the volume for each track, this is the time. Use the `gain =` option with the desired adjustment in decibels.
- For the compo itself, make sure that the options `autoplay = false` is set, and that `auto advance` is *not* set (or set to `false` too).
- During the compo:
  - Show the first entry's slide on the bigscreen. In the meantime, run `tm.exe`; it should start with entry #1 loaded, but in paused state.
  - Switch the bigscreen over to the TrackMeister computer and press **Space** to start playback.
  - After the track is finished, switch over to the bigscreen again.
  - While switching the bigscreen from entry #1's slide to entry #2, also press **PageDown** on TrackMeister to cue the next track.
  - Switch the bigscreen over to the TrackMeister computer and press **Space** to start playback.
  - Rinse and repeat.


## Building

- prerequisites:
  - a C++17 compatible compiler (GCC 7.1 or later, Clang 6 or later, Microsoft Visual Studio 2019 or later)
  - CMake 3.15 or later
  - Python 3.8 or later
  - SDL2 development packages (only required on non-Windows systems; on Windows, the SDL2 SDK will be downloaded automatically during building)
- make sure you cloned the repository recursively, as it pulls in a few libraries as submodules; if you forgot to do that, run "`git submodule update --init`"
- building itself is done using standard CMake (e.g. "`cmake -S . -B build && cmake --build build`")


## Acknowledgements

TrackMeister was written by Martin J. Fiedler a.k.a. KeyJ of TRBL.
It is built upon the following third-party libraries:
- [SDL 2](http://libsdl.org) is the basic framework for window management, event handling and audio playback
- [GLAD](https://glad.dav1d.de) is used as the OpenGL interface generator
- [MSDF](https://github.com/Chlumsky/msdf-atlas-gen) is what TrackMeister's font rendering is based upon
- [Inconsolata](https://levien.com/type/myfonts/inconsolata.html) is the default vector font that's used in the UI and logo
- [Iosevka](https://typeof.net/Iosevka/) is an alternate vector font that can be used
- [LodePNG](https://lodev.org/lodepng/) is used to decode PNG files
- [libopenmpt](https://lib.openmpt.org/libopenmpt/) is doing all the heavy lifting concerning module file parsing and audio rendering
- [libebur128](https://github.com/jiixyj/libebur128) is used for loudness normalization
