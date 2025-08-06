# 1.4.1

It's again just before [some larger party](https://2024.evoke.eu), so here's a release that collects all the changes of the past months and ships with the latest OpenMPT libraries:

- updated to libopenmpt-0.8.2; for details, see [here](https://lib.openmpt.org/libopenmpt/2025/05/31/release-0.8.0/), [here](https://lib.openmpt.org/libopenmpt/2025/06/14/releases-0.8.1-0.7.14-0.6.23-0.5.37-0.4.49/) and [here](https://lib.openmpt.org/libopenmpt/2025/07/19/security-updates-0.8.2-0.7.15-0.6.24-0.5.37-0.4.50/)
- thanks to a new API in libopenmpt-0.8, TrackMeister now offers an `auto loop` option that tries to detect whether a module is supposed to be looped or not
  - it's not 100% reliable, nor can it ever be, but at least it's a useful heuristic
- fixed a few bugs in the configuration editor:
  - changes to string values were lost when pressing Ctrl+S without de-focussing the edit field before
  - fixed slight config file corruption when changing a string value from empty to non-empty


# 1.4.0

Another pre-party release, just in time for Revision. The main feature this time is a fully built-in configuration UI, making preparations and customizations quite a bit easier for compo organizers and theme designers.

Here's the full list of features:
- updated to libopenmpt 0.7.13 with several fixes - see [here](https://lib.openmpt.org/libopenmpt/2024/10/26/releases-0.7.11-0.6.20-0.5.34-0.4.46/), [here](https://lib.openmpt.org/libopenmpt/2024/12/01/releases-0.7.12-0.6.21-0.5.35-0.4.47/) and [here](https://lib.openmpt.org/libopenmpt/2025/01/06/releases-0.7.13-0.6.22-0.5.36-0.4.48/)
- configuration system changed: there's now a clear separation between global configuration (global section of `tm.ini`) and per-file configuration (filename-pattern sections of `tm.ini` and sidecar `.mod.tm` files), with the latter taking precedence over the former
- added configuration UI (F2/F3 keys)
  - can modify almost all settings, both global and file-specific
  - can revert settings to their defaults
  - changes become effective immediately, if possible (most visual settings do), or after reloading the current module (F5)
  - can save `tm.ini` and `.tm` sidecar files (Ctrl+S)
- added built-in help screen (F1 key)


# 1.3.0

This release packs together all the small changes and (literal!) last-minute fixes that happened at Evoke, and a few others:
- updated to libopenmpt 0.7.10 with several fixes - see [here](https://lib.openmpt.org/libopenmpt/2024/09/22/security-update-0.6.19-releases-0.7.10-0.5.33-0.4.45/)
- more font rendering changes
  - small fonts are now *way* more readable
  - aliasing in scrolling texts is now really minimal
- added option to permanently the currently elapsed track time in the info bar: `show time = true`
- metadata auto-scrolling now only starts 10 seconds into the track and stops 10 seconds before the end, at latest
  - makes it easier for the audience to read longer scrolltexts
  - the delay can be adjusted with the `scroll delay` option (setting it to zero restores the old behavior)


# 1.2.2

A quick service release just before [some specific party](https://2024.evoke.eu), to make the compo organizer's life a bit easier:
- new key binding: **P** shows the current track time in seconds; useful to determine the values for the `fade out at` option


# 1.2.1

Another small release:
- updated to libopenmpt 0.7.9 with several fixes - see [here](https://lib.openmpt.org/libopenmpt/2024/07/21/security-updates-0.7.9-0.6.18-0.5.32-0.4.44/)
- some changes regarding font rendering
  - flickering / shimmering in scrolling info texts and missing lines in small texts should now be greatly reduced
  - caveat: slightly "thicker" fonts


# 1.2.0

Just a service release to get the changes from the last few months out the door:
- updated to libopenmpt 0.7.8 with several fixes - see [here](https://lib.openmpt.org/libopenmpt/2024/05/12/releases-0.7.7-0.6.16-0.5.30-0.4.42/) and [here](https://lib.openmpt.org/libopenmpt/2024/06/09/security-update-0.7.8-releases-0.6.17-0.5.31-0.4.43/)
- more verbose pattern display, now showing effects whenever possible
- new scalable vector font: `font = iosevka`, which is even narrower and more geometric than the default (Inconsolata)
- can now use an arbitrary full-color PNG file as a background image: `background image = foo.png`
- file name display in the info bar can now omit the file extension (`hide file ext = true`), or can be hidden completely if there's title/artist metadata available (`auto-hide file name = true`)
- can now override displayed title and artist in the config files
- interactive volume controls (`+`/`-` keys)
- clipping indicator that warns when the configured gain is too high (optional, and disabled by default)
- drag&drop and command line now ignores the `.tm` file suffix and instead plays the associated module
- visual fine tuning:
  - no more scrolling of 31-instrument MOD sample texts at 1080p resolution
  - Windows HighDPI support
  - option to allow downscaling the black&white logo to arbitrary sizes instead of just powers of two
- the usual assortment of small bugfixes


# 1.1.0

Right before this year's Revision party (which will be the first bigscreen showing of TrackMeister ever, woohoo!), we packed together a new release. The main features are:
- updated to libopenmpt 0.7.6 with a few security and minor functionality bugfixes - for details, see [here](https://lib.openmpt.org/libopenmpt/2024/03/03/releases-0.7.4-0.6.13-0.5.27-0.4.39/), [here](https://lib.openmpt.org/libopenmpt/2024/03/17/security-updates-0.7.5-0.6.14-0.5.28-0.4.40/) and [here](https://lib.openmpt.org/libopenmpt/2024/03/24/security-updates-0.7.6-0.6.15-0.5.29-0.4.41/)
- the released Windows executable has been signed by the OpenMPT developers (thanks for that!); this should make Windows Defender slightly less nervous about `tm.exe`
- the default filter setting now automatically activates the Amiga-specific filter for Amiga modules like ProTracker MODs
- when opening directories or using PageUp/PageDown for navigation, old-school `mod.*` files are now also picked up; no need to rename them to `*.mod`
- new options for continuous, jukebox-style playback - try `auto advance = true` and `shuffle = true`
- added a Python script that downloads a few example modules from the internet (This is only present in the Linux release, as we can't rely on Python being available on Windows - but if you do have Windows and Python installed, you can of course get the script [here](https://raw.githubusercontent.com/kajott/TrackMeister/main/download_examples.py))
- some classic Amiga and PC bitmap fonts can now be selected instead of the default (quasi-)vector font - try `font = topaz`, `font = topaz500` or `font = pc`
- configurable volume ramping (e.g. `volume ramping = 0` to disable it)
- configurable logo position (e.g. `logo pos Y = 100` to put the logo to the bottom)
- a more meaningful error message is now produced if the graphics driver's supported OpenGL version is too old
- a few minor bugfixes


# 1.0.0: initial public release

First version, nothing to mention here except what's written in README.md.
