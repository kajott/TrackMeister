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
