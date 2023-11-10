cmake_minimum_required (VERSION 3.15)

project (TMCP)

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
set (CMAKE_CXX_STANDARD 11)

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

# set sources for main program and third-party libs
add_executable (tmcp
    src/main.cpp
)
add_library (tmcp_external STATIC
    external/glad/src/glad.c
)

# set include directories
target_include_directories (tmcp_external PUBLIC
    src
    ${SDL2_INCLUDE_DIRS}
    external/glad/include
)
target_include_directories (tmcp PRIVATE src)

# set library dependencies
target_link_libraries (tmcp PUBLIC tmcp_external ${SDL2_LIBRARIES})
if (WIN32)
    target_link_libraries (tmcp PUBLIC opengl32)
else ()
    target_link_libraries (tmcp PUBLIC m dl GL)
endif ()

# platform-dependent additional sources and options
if (WIN32)
    target_sources (tmcp PRIVATE
        src/utf8.manifest
    )
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties (tmcp PROPERTIES WIN32_EXECUTABLE ON)
    endif ()
    set_target_properties (tmcp PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    target_link_libraries (tmcp PRIVATE SDL2main)
else ()
    set (THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package (Threads REQUIRED)
    target_link_libraries (tmcp Threads::Threads)
endif ()

# compiler options
if (NOT MSVC)
    target_compile_options (tmcp PRIVATE -Wall -Wextra -pedantic -Werror -fwrapv)
else ()
    target_compile_options (tmcp PRIVATE /W4 /WX)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT MSVC)
        message (STATUS "Debug build, enabling Address Sanitizer")
        target_compile_options (tmcp PRIVATE "-fsanitize=address")
        target_compile_options (tmcp_external PUBLIC "-fsanitize=address")
        target_link_options (tmcp PRIVATE "-fsanitize=address")
    elseif (MSVC_VERSION GREATER 1627)
        message (STATUS "Debug build and MSVC 16.8 or greater detected, enabling Address Sanitizer")
        target_compile_options (tmcp PRIVATE "/fsanitize=address")
        target_compile_options (tmcp_external PUBLIC "/fsanitize=address")
        target_link_options (tmcp PRIVATE "/DEBUG")
        # ASAN isn't compatible with the /RTC switch and incremental linking,
        # both of which CMake enables by default
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string (REGEX REPLACE "/RTC(su|[1su])?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string (REGEX REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    endif ()
endif ()

# make the binary appear in the project's root directory
set_target_properties (tmcp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_CURRENT_LIST_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_NAME_DEBUG                       "tmcp_debug"
)
