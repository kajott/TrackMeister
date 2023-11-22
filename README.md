Tracked Music Compo Player
==========================

This application is a player for [tracker music files](https://en.wikipedia.org/wiki/Module_file) ("module" files) as they were &ndash;and still are&ndash; common in the [demoscene](https://en.wikipedia.org/wiki/Demoscene). With its fullscreen interface and limited interaction options, it's specifically targeted towards presenting tracked music in a competition ("compo").

## Features

- based on [libopenmpt](https://lib.openmpt.org/libopenmpt/), a very well-regarded library for module file playback
- plays all common formats: MOD, XM, IT, S3M, and a dozen others
- fullscreen interface based on OpenGL
- metadata display (title, artist, technical info, module comment, intstrument and sample names)
- pattern display for visualization
- cross-platform (tested on Windows and Linux)
- single executable; no extra DLLs/`.so`s needed
- open source (MIT license)

## Building

- prerequisites:
  - a C++17 compatible compiler (GCC 7.1 or later, Clang 6 or later, Microsoft Visual Studio 2019 or later)
  - CMake 3.15 or later
  - Python 3.8 or later
  - SDL2 development packages (only required on non-Windows systems; on Windows, the SDL2 SDK will be downloaded automatically during building)
- make sure you cloned the repository recursively, as it pulls libopenmpt in as a submodule; if you forgot that, run "`git submodule update --init`"
- building itself is done using standard CMake (e.g. "`cmake -S . -B build && cmake --build build`")

## Usage

Just drag a module file onto the executable or into the window once the player has already been started.

Playback is always paused after loading a module; this is by design. Press the **Space** key to start playback.

The screen is split into three parts: The pattern display, the info bar at the top (containing basic information about the module format as well as filename, title and artist), and the metadata bar at the right (with the free-text module message, instrument and sample names). If the content doesn't fit in the metadata bar, it slowly scrolls down during playback, so it reaches the bottom end at the end of the track or after four minutes, whatever comes first.

The following other controls are available:

| Input | Action |
|-------|--------|
| **Q** or **Alt+F4** | quit the application
| **Space** | pause / continue playback
| **Tab** | hide / unhide the info and metadata bars
| Cursor **Left** / **Right** | seek backward / forward one order
| Mouse Wheel | manually scroll through the metadata bar (stops autoscrolling)
| **A** | stop / resume autoscrolling
| **V** | show the TMCP and libopenmpt version numbers
| **F11** | toggle fullscreen mode

> **Note:**
> The project is WIP at this moment. More key bindings will follow.
