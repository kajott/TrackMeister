cmake_minimum_required (VERSION 3.15)

project (TrackMeister)

# set default build type
set (default_build_type "Debug")
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set (CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "build type" FORCE)
        set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    endif ()
    message (STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
endif ()

# set C/C++ language standards for compilers that respect them
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 17)

# ensure we have a working SDL2 library; on Win32, bootstrap it ourselves
if (WIN32)
    # first, download the sources
    set (SDL2_VERSION 2.28.5)
    set (SDL2_DIR "${CMAKE_CURRENT_LIST_DIR}/external/SDL2-${SDL2_VERSION}")
    if (NOT EXISTS "${SDL2_DIR}/src")
        message (STATUS "${SDL2_DIR} does not exist")
        set (SDL2_ZIPFILE "SDL2-${SDL2_VERSION}.zip")
        set (SDL2_ARCHIVE "${CMAKE_CURRENT_LIST_DIR}/external/${SDL2_ZIPFILE}")
        if (NOT EXISTS "${SDL2_ARCHIVE}")
            message (STATUS "downloading SDL sources ...")
            file (DOWNLOAD "http://www.libsdl.org/release/${SDL2_ZIPFILE}" "${SDL2_ARCHIVE}")
        endif ()
        message (STATUS "unpacking SDL sources ...")
        execute_process (COMMAND powershell Expand-Archive "${SDL2_ARCHIVE}" -DestinationPath "${CMAKE_CURRENT_LIST_DIR}/external")
    endif ()
    # set options for a minimal static SDL2 build
    foreach (disable_
        SDL_RENDER DIRECTX RENDER_D3D
        SDL_ATOMIC SDL_CPUINFO SDL_FILE SDL_FILESYSTEM SDL_POWER SDL_LOCALE
        SDL_HAPTIC SDL_SENSOR
        ASSEMBLY MMX 3DNOW SSE SSE2 SSE3 SSEMATH
        OPENGLES VULKAN XINPUT
    )
        option ("${disable_}" OFF)
    endforeach ()
    set (SDL_SHARED OFF CACHE BOOL "" FORCE)
    set (SDL_STATIC ON  CACHE BOOL "" FORCE)
    # include SDL build directory and publish the required settings
    message (STATUS "v v v v v v v v v v v SDL2 configure messages begin here v v v v v v v v v v")
    add_subdirectory (${SDL2_DIR})
    message (STATUS "^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ SDL2 configure messages end here ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^")
    set (SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
    set (SDL2_LIBRARIES SDL2-static SDL2main)
else ()
    # on systems that tend to have proper package managers,
    # we can just use the system's default SDL2 installation
    find_package (SDL2 REQUIRED)
endif ()

###############################################################################

# provide libopenmpt (which doesn't have a CMake build system yet, so it's all
# manual by now)
add_library (libopenmpt STATIC
    external/openmpt/common/ComponentManager.cpp
    external/openmpt/common/Logging.cpp
    external/openmpt/common/Profiler.cpp
    external/openmpt/common/mptFileIO.cpp
    external/openmpt/common/mptFileTemporary.cpp
    external/openmpt/common/mptFileType.cpp
    external/openmpt/common/mptPathString.cpp
    external/openmpt/common/mptRandom.cpp
    external/openmpt/common/mptStringBuffer.cpp
    external/openmpt/common/mptTime.cpp
    external/openmpt/common/serialization_utils.cpp
    external/openmpt/common/version.cpp
    external/openmpt/libopenmpt/libopenmpt_c.cpp
    external/openmpt/libopenmpt/libopenmpt_cxx.cpp
    external/openmpt/libopenmpt/libopenmpt_ext_impl.cpp
    external/openmpt/libopenmpt/libopenmpt_impl.cpp
    external/openmpt/sounddsp/AGC.cpp
    external/openmpt/sounddsp/DSP.cpp
    external/openmpt/sounddsp/EQ.cpp
    external/openmpt/sounddsp/Reverb.cpp
    external/openmpt/soundlib/AudioCriticalSection.cpp
    external/openmpt/soundlib/ContainerMMCMP.cpp
    external/openmpt/soundlib/ContainerPP20.cpp
    external/openmpt/soundlib/ContainerUMX.cpp
    external/openmpt/soundlib/ContainerXPK.cpp
    external/openmpt/soundlib/Dlsbank.cpp
    external/openmpt/soundlib/Fastmix.cpp
    external/openmpt/soundlib/ITCompression.cpp
    external/openmpt/soundlib/ITTools.cpp
    external/openmpt/soundlib/InstrumentExtensions.cpp
    external/openmpt/soundlib/Load_667.cpp
    external/openmpt/soundlib/Load_669.cpp
    external/openmpt/soundlib/Load_amf.cpp
    external/openmpt/soundlib/Load_ams.cpp
    external/openmpt/soundlib/Load_c67.cpp
    external/openmpt/soundlib/Load_dbm.cpp
    external/openmpt/soundlib/Load_digi.cpp
    external/openmpt/soundlib/Load_dmf.cpp
    external/openmpt/soundlib/Load_dsm.cpp
    external/openmpt/soundlib/Load_dsym.cpp
    external/openmpt/soundlib/Load_dtm.cpp
    external/openmpt/soundlib/Load_far.cpp
    external/openmpt/soundlib/Load_fmt.cpp
    external/openmpt/soundlib/Load_gdm.cpp
    external/openmpt/soundlib/Load_gt2.cpp
    external/openmpt/soundlib/Load_imf.cpp
    external/openmpt/soundlib/Load_it.cpp
    external/openmpt/soundlib/Load_itp.cpp
    external/openmpt/soundlib/Load_mdl.cpp
    external/openmpt/soundlib/Load_med.cpp
    external/openmpt/soundlib/Load_mid.cpp
    external/openmpt/soundlib/Load_mo3.cpp
    external/openmpt/soundlib/Load_mod.cpp
    external/openmpt/soundlib/Load_mt2.cpp
    external/openmpt/soundlib/Load_mtm.cpp
    external/openmpt/soundlib/Load_mus_km.cpp
    external/openmpt/soundlib/Load_okt.cpp
    external/openmpt/soundlib/Load_plm.cpp
    external/openmpt/soundlib/Load_psm.cpp
    external/openmpt/soundlib/Load_ptm.cpp
    external/openmpt/soundlib/Load_s3m.cpp
    external/openmpt/soundlib/Load_sfx.cpp
    external/openmpt/soundlib/Load_stm.cpp
    external/openmpt/soundlib/Load_stp.cpp
    external/openmpt/soundlib/Load_symmod.cpp
    external/openmpt/soundlib/Load_uax.cpp
    external/openmpt/soundlib/Load_ult.cpp
    external/openmpt/soundlib/Load_wav.cpp
    external/openmpt/soundlib/Load_xm.cpp
    external/openmpt/soundlib/Load_xmf.cpp
    external/openmpt/soundlib/MIDIEvents.cpp
    external/openmpt/soundlib/MIDIMacros.cpp
    external/openmpt/soundlib/MPEGFrame.cpp
    external/openmpt/soundlib/Message.cpp
    external/openmpt/soundlib/MixFuncTable.cpp
    external/openmpt/soundlib/MixerLoops.cpp
    external/openmpt/soundlib/MixerSettings.cpp
    external/openmpt/soundlib/ModChannel.cpp
    external/openmpt/soundlib/ModInstrument.cpp
    external/openmpt/soundlib/ModSample.cpp
    external/openmpt/soundlib/ModSequence.cpp
    external/openmpt/soundlib/OPL.cpp
    external/openmpt/soundlib/OggStream.cpp
    external/openmpt/soundlib/Paula.cpp
    external/openmpt/soundlib/RowVisitor.cpp
    external/openmpt/soundlib/S3MTools.cpp
    external/openmpt/soundlib/SampleFormatBRR.cpp
    external/openmpt/soundlib/SampleFormatFLAC.cpp
    external/openmpt/soundlib/SampleFormatMP3.cpp
    external/openmpt/soundlib/SampleFormatMediaFoundation.cpp
    external/openmpt/soundlib/SampleFormatOpus.cpp
    external/openmpt/soundlib/SampleFormatSFZ.cpp
    external/openmpt/soundlib/SampleFormatVorbis.cpp
    external/openmpt/soundlib/SampleFormats.cpp
    external/openmpt/soundlib/SampleIO.cpp
    external/openmpt/soundlib/Snd_flt.cpp
    external/openmpt/soundlib/Snd_fx.cpp
    external/openmpt/soundlib/Sndfile.cpp
    external/openmpt/soundlib/Sndmix.cpp
    external/openmpt/soundlib/SoundFilePlayConfig.cpp
    external/openmpt/soundlib/Tables.cpp
    external/openmpt/soundlib/Tagging.cpp
    external/openmpt/soundlib/TinyFFT.cpp
    external/openmpt/soundlib/UMXTools.cpp
    external/openmpt/soundlib/UpgradeModule.cpp
    external/openmpt/soundlib/WAVTools.cpp
    external/openmpt/soundlib/WindowedFIR.cpp
    external/openmpt/soundlib/XMTools.cpp
    external/openmpt/soundlib/load_j2b.cpp
    external/openmpt/soundlib/mod_specifications.cpp
    external/openmpt/soundlib/modcommand.cpp
    external/openmpt/soundlib/modsmp_ctrl.cpp
    external/openmpt/soundlib/pattern.cpp
    external/openmpt/soundlib/patternContainer.cpp
    external/openmpt/soundlib/plugins/DigiBoosterEcho.cpp
    external/openmpt/soundlib/plugins/LFOPlugin.cpp
    external/openmpt/soundlib/plugins/PlugInterface.cpp
    external/openmpt/soundlib/plugins/PluginManager.cpp
    external/openmpt/soundlib/plugins/SymMODEcho.cpp
    external/openmpt/soundlib/plugins/dmo/Chorus.cpp
    external/openmpt/soundlib/plugins/dmo/Compressor.cpp
    external/openmpt/soundlib/plugins/dmo/DMOPlugin.cpp
    external/openmpt/soundlib/plugins/dmo/DMOUtils.cpp
    external/openmpt/soundlib/plugins/dmo/Distortion.cpp
    external/openmpt/soundlib/plugins/dmo/Echo.cpp
    external/openmpt/soundlib/plugins/dmo/Flanger.cpp
    external/openmpt/soundlib/plugins/dmo/Gargle.cpp
    external/openmpt/soundlib/plugins/dmo/I3DL2Reverb.cpp
    external/openmpt/soundlib/plugins/dmo/ParamEq.cpp
    external/openmpt/soundlib/plugins/dmo/WavesReverb.cpp
    external/openmpt/soundlib/tuning.cpp
    external/openmpt/soundlib/tuningCollection.cpp
    external/openmpt/include/miniz/miniz.c
    external/openmpt/include/minimp3/minimp3.c
    external/openmpt/include/stb_vorbis/stb_vorbis.c
)
target_include_directories (libopenmpt PUBLIC
    external/openmpt
)
target_include_directories (libopenmpt PRIVATE
    external/openmpt/common
    external/openmpt/src
    external/openmpt/include
)
target_compile_definitions (libopenmpt PRIVATE
    LIBOPENMPT_BUILD
    MPT_CHECK_CXX_IGNORE_PREPROCESSOR
    MPT_CHECK_WINDOWS_IGNORE_WARNING_NO_UNICODE
    MPT_WITH_MINIZ
    MPT_WITH_MINIMP3
    MPT_WITH_STBVORBIS
    STB_VORBIS_HEADER_ONLY
    STB_VORBIS_NO_PULLDATA_API
    STB_VORBIS_NO_STDIO
)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions (libopenmpt PRIVATE _DEBUG)
endif ()

###############################################################################

# set sources for main program and third-party libs
add_executable (tm
    src/main_sdl2.cpp
    src/app.cpp
    src/app_layout.cpp
    src/config.cpp
    src/config_data.cpp
    src/textarea.cpp
    src/pathutil.cpp
    src/renderer.cpp
    font/font_data.cpp
    logo/logo_data.cpp
)
add_library (tm_external STATIC
    external/glad/src/glad.c
    external/lodepng/lodepng.cpp
    external/libebur128/ebur128/ebur128.c
    external/openmpt/include/stb_vorbis/stb_vorbis.c  # only ever gets included as a header in libopenmpt itself
)

# produce auto-generated files
add_custom_command (
    COMMENT         "updating configuration option registry"
    COMMAND         "${CMAKE_CURRENT_SOURCE_DIR}/src/generate_config_data.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/src/config_data.cpp"
    DEPENDS         "${CMAKE_CURRENT_SOURCE_DIR}/src/generate_config_data.py"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h"
    OUTPUT          "${CMAKE_CURRENT_SOURCE_DIR}/src/config_data.cpp"
)

# set include directories
target_include_directories (tm_external PUBLIC
    src
    ${SDL2_INCLUDE_DIRS}
    external/glad/include
    external/lodepng
    external/libebur128/ebur128
)
target_include_directories (tm_external PRIVATE
    external/libebur128/ebur128/queue
)
if (MSVC)
    target_compile_definitions (tm_external PRIVATE _USE_MATH_DEFINES)
endif ()
target_compile_definitions (tm_external PRIVATE
    LODEPNG_NO_COMPILE_ENCODER LODEPNG_NO_COMPILE_DISK
    LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS LODEPNG_NO_COMPILE_ERROR_TEXT
)
target_include_directories (tm PRIVATE src)

# set library dependencies
target_link_libraries (tm PUBLIC libopenmpt tm_external ${SDL2_LIBRARIES})
if (WIN32)
    target_link_libraries (tm PUBLIC opengl32)
else ()
    target_link_libraries (tm PUBLIC m dl GL)
endif ()

# platform-dependent additional sources and options
if (WIN32)
    target_sources (tm PRIVATE
        src/utf8.manifest
        src/icon.rc
    )
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties (tm PROPERTIES WIN32_EXECUTABLE ON)
    endif ()
    set_target_properties (tm PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    target_link_libraries (tm PRIVATE SDL2main)
else ()
    set (THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package (Threads REQUIRED)
    target_link_libraries (tm PRIVATE Threads::Threads)
endif ()

# compiler options
if (NOT MSVC)
    target_compile_options (tm PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (tm PRIVATE /W4 /WX)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT MSVC)
        message (STATUS "Debug build, enabling Address Sanitizer")
        target_compile_options (tm PRIVATE "-fsanitize=address")
        target_compile_options (tm_external PUBLIC "-fsanitize=address")
        target_link_options (tm PRIVATE "-fsanitize=address")
    elseif (MSVC_VERSION GREATER 1627)
        message (STATUS "Debug build and MSVC 16.8 or greater detected, enabling Address Sanitizer")
        target_compile_options (tm PRIVATE "/fsanitize=address")
        target_compile_options (tm_external PUBLIC "/fsanitize=address")
        target_link_options (tm PRIVATE "/DEBUG")
        # ASAN isn't compatible with the /RTC switch and incremental linking,
        # both of which CMake enables by default
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string (REGEX REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    endif ()
endif ()

# make the binary appear in the project's root directory
set_target_properties (tm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_NAME_DEBUG                       "tm_debug"
)

# documentation stuff
add_custom_target (doc
    DEPENDS         "${CMAKE_CURRENT_SOURCE_DIR}/tm.html"
)
add_custom_command (
    COMMENT         "generating HTML documentation"
    COMMAND         "${CMAKE_CURRENT_SOURCE_DIR}/src/md2html.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
                 -o "${CMAKE_CURRENT_SOURCE_DIR}/tm.html"
                 -T "TrackMeister Documentation"
                 -x building
    DEPENDS         "${CMAKE_CURRENT_SOURCE_DIR}/src/md2html.py"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    OUTPUT          "${CMAKE_CURRENT_SOURCE_DIR}/tm.html"
)
