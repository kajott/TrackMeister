// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#ifndef NDEBUG
    #define DEFAULT_FULLSCREEN false
#else
    #define DEFAULT_FULLSCREEN true
#endif

#include <functional>

struct SystemInterfacePrivateData;

class SystemInterface {
    SystemInterfacePrivateData* m_priv;
    bool m_active = true;
public:
    explicit inline SystemInterface(SystemInterfacePrivateData& priv) : m_priv(&priv) {}

    [[noreturn]] void fatalError(const char* what, const char* how);

    void initSystem();
    void initVideo(const char* title, bool fullscreen=DEFAULT_FULLSCREEN, int windowWidth=1920, int windowHeight=1080);
    int initAudio(bool stereo, int sampleRate=48000, int bufferSize=512);

    void lockAudioMutex();
    void unlockAudioMutex();

    bool isPaused();
    inline bool isPlaying()   { return !isPaused(); }
    bool setPaused(bool paused);
    inline bool pause()       { return setPaused(true); }
    inline bool play()        { return setPaused(false); }
    inline bool togglePause() { return setPaused(!isPaused()); }

    inline void quit() { m_active = false; }
    inline bool active() { return m_active; }

    void setWindowTitle(const char* title);
    void toggleFullscreen();
};

class AudioMutexGuard {
    SystemInterface& m_sys;
public:
    explicit inline AudioMutexGuard(SystemInterface& sys) : m_sys(sys) { m_sys.lockAudioMutex(); }
    inline ~AudioMutexGuard() { m_sys.unlockAudioMutex(); }
};
