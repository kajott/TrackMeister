// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

#include <SDL.h>
#include <glad/glad.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "system.h"
#include "util.h"
#include "app.h"

struct SystemInterfacePrivateData {
    bool sysInitDone = false;
    Application* app = nullptr;
    SDL_Window* win = nullptr;
    SDL_GLContext ctx = nullptr;
    ImGuiIO* io = nullptr;
    SDL_AudioDeviceID audio = 0;
    int sampleRate = 0;
    bool stereo = false;
    bool paused = false;
    bool fullscreen = false;
};

[[noreturn]] void SystemInterface::fatalError(const char *what, const char *how) {
    #if defined(_WIN32) && defined(NDEBUG)
        MessageBoxA(nullptr, how, what, MB_OK | MB_ICONERROR);
    #else
        fprintf(stderr, "FATAL: %s - %s\n", what, how);
    #endif
    std::exit(1);
}

void SystemInterface::initSystem() {
    if (!m_priv->sysInitDone) {
        #ifdef _WIN32
            SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
        #endif
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            fatalError("SDL initialization failed", SDL_GetError());
        }
        m_priv->sysInitDone = true;
    }
}

#ifndef NDEBUG
    void APIENTRY glDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        const char *sSource, *sType, *sSev;  (void)id, (void)length, (void)userParam;
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;  // ignore these, they are *far* too verbose on some drivers
        }
        switch (source) {
            case GL_DEBUG_SOURCE_API:               sSource = "API  "; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     sSource = "Win  "; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:   sSource = "GLSL "; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:       sSource = "Aux  "; break;
            case GL_DEBUG_SOURCE_APPLICATION:       sSource = "App  "; break;
            case GL_DEBUG_SOURCE_OTHER:             sSource = "Other"; break;
            default:                                sSource = "?????"; break;
        }
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               sType = "Error "; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: sType = "Deprec"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  sType = "Undef "; break;
            case GL_DEBUG_TYPE_PORTABILITY:         sType = "Port  "; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         sType = "Perf  "; break;
            case GL_DEBUG_TYPE_MARKER:              sType = "Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          sType = "PushG "; break;
            case GL_DEBUG_TYPE_POP_GROUP:           sType = "PopG  "; break;
            case GL_DEBUG_TYPE_OTHER:               sType = "Other "; break;
            default:                                sType = "??????"; break;
        }
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:            sSev = "High"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:          sSev = "Med "; break;
            case GL_DEBUG_SEVERITY_LOW:             sSev = "Low "; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:    sSev = "Note"; break;
            default:                                sSev = "????"; break;
        }
        fprintf(stderr, "[GL Debug] %s | %s | %s | %s\n", sSource, sType, sSev, message);
    }
#endif

void SystemInterface::initVideo(const char* title, bool fullscreen, int windowWidth, int windowHeight) {
    initSystem();
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,            0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,          0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    #ifndef NDEBUG
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,     SDL_GL_CONTEXT_DEBUG_FLAG);
    #endif

    m_priv->win = SDL_CreateWindow(title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI |
        (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE));
    if (!m_priv->win) {
        fatalError("could not create window", SDL_GetError());
    }
    m_priv->fullscreen = fullscreen;

    m_priv->ctx = SDL_GL_CreateContext(m_priv->win);
    if (!m_priv->ctx) {
        fatalError("could not create OpenGL context", SDL_GetError());
    }
    SDL_GL_MakeCurrent(m_priv->win, m_priv->ctx);
    SDL_GL_SetSwapInterval(1);
    if (!gladLoadGL()) {
        fatalError("could not initialize OpenGL", "failed to load OpenGL functions");
    }
    #ifndef NDEBUG
        printf("OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
        printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
        printf("OpenGL version:  %s\n", glGetString(GL_VERSION));
        printf("GLSL   version:  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        if (glDebugMessageCallback != nullptr) {
            glDebugMessageCallback(glDebugProc, nullptr);
            glEnable(GL_DEBUG_OUTPUT);
        }
    #endif
    if ((GLVersion.major < 3) || ((GLVersion.major == 3) && (GLVersion.minor < 3))) {
        fatalError("could not initialize OpenGL", "at least OpenGL 3.3 is required");
    }

    ImGui::CreateContext();
    m_priv->io = &ImGui::GetIO();
    m_priv->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    m_priv->io->IniFilename = nullptr;
    ImGui_ImplSDL2_InitForOpenGL(m_priv->win, m_priv->ctx);
    ImGui_ImplOpenGL3_Init("#version 330");

    if (fullscreen) { SDL_ShowCursor(SDL_DISABLE); }
}

static void sysRenderAudio(void* userdata, Uint8* stream, int len) {
    auto *priv = static_cast<SystemInterfacePrivateData*>(userdata);
    bool ok = false;
    if (priv && priv->app) {
        ok = priv->app->renderAudio((int16_t*)stream, priv->stereo ? (len >> 2) : (len >> 1), priv->stereo, priv->sampleRate);
    }
    if (!ok) {
        SDL_memset(stream, 0, len);
    }
}

int SystemInterface::initAudio(bool stereo, int sampleRate, int bufferSize) {
    initSystem();
    SDL_AudioSpec want, got;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = sampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = stereo ? 2 : 1;
    want.samples = Uint16(bufferSize);
    want.callback = sysRenderAudio;
    want.userdata = static_cast<void*>(m_priv);
    m_priv->paused = true;
    m_priv->audio = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &want, &got, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (!m_priv->audio) {
        fatalError("could not open audio device", SDL_GetError());
    }
    m_priv->stereo = (got.channels > 1);
    m_priv->sampleRate = got.freq;
    return m_priv->sampleRate;
}

void SystemInterface::lockAudioMutex() {
    if (m_priv->audio) {
        SDL_LockAudioDevice(m_priv->audio);
    }
}
void SystemInterface::unlockAudioMutex() {
    if (m_priv->audio) {
        SDL_UnlockAudioDevice(m_priv->audio);
    }
}

bool SystemInterface::isPaused() {
    return m_priv->paused;
}

bool SystemInterface::setPaused(bool paused) {
    if (m_priv->audio) {
        SDL_PauseAudioDevice(m_priv->audio, paused ? SDL_TRUE : SDL_FALSE);
        m_priv->paused = paused;
    }
    return m_priv->paused;
}

void SystemInterface::setWindowTitle(const char* title) {
    SDL_SetWindowTitle(m_priv->win, title);
}

void SystemInterface::toggleFullscreen() {
    m_priv->fullscreen = !m_priv->fullscreen;
    SDL_SetWindowFullscreen(m_priv->win, m_priv->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_ShowCursor(m_priv->fullscreen ? SDL_DISABLE : SDL_ENABLE);
}

int main(int argc, char* argv[]) {
    SystemInterfacePrivateData priv;
    SystemInterface sys(priv);
    Application app(sys);
    priv.app = &app;

    // initialization
    int ret = app.init(argc, argv);  // this will likely call initVideo() and initAudio()
    if (ret >= 0) { return ret; }
    sys.initSystem();

    // main loop
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    Uint64 tPrev = 0;
    while (sys.active()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            switch (ev.type) {
                case SDL_KEYDOWN: {
                    uint32_t key = uint32_t(ev.key.keysym.sym);
                    switch (key) {
                        case SDLK_LEFT:     key = makeFourCC("Left");  break;
                        case SDLK_RIGHT:    key = makeFourCC("Right"); break;
                        case SDLK_UP:       key = makeFourCC("Up");    break;
                        case SDLK_DOWN:     key = makeFourCC("Down");  break;
                        case SDLK_PAGEUP:   key = makeFourCC("PgUp");  break;
                        case SDLK_PAGEDOWN: key = makeFourCC("PgDn");  break;
                        case SDLK_HOME:     key = makeFourCC("Home");  break;
                        case SDLK_END:      key = makeFourCC("End");   break;
                        case SDLK_INSERT:   key = makeFourCC("Ins");   break;
                        case SDLK_DELETE:   key = makeFourCC("Del");   break;
                        case SDLK_KP_PLUS:  key = makeFourCC("KP+");   break;
                        case SDLK_KP_MINUS: key = makeFourCC("KP-");   break;
                        default:
                            if ((key >= 'a') && (key <= 'z')) { key -= 32; }
                            else if ((key >= SDLK_F1) && (key <= SDLK_F12)) { key = key - SDLK_F1 + 0xF1; }
                    }
                    auto mods = SDL_GetModState();
                    if (!priv.io || !priv.io->WantCaptureKeyboard) {
                        app.handleKey(key, !!(mods & KMOD_CTRL), !!(mods & KMOD_SHIFT), !!(mods & KMOD_ALT));
                    }
                    break; }
                case SDL_MOUSEWHEEL:
                    if (!priv.io || !priv.io->WantCaptureMouse) {
                        app.handleMouseWheel(ev.wheel.y);
                    }
                    break;
                case SDL_DROPFILE:
                    app.handleDropFile(ev.drop.file);
                    SDL_free(ev.drop.file);
                    break;
                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        int rw = 0, rh = 0;
                        // get actual drawable size of the window in pixels to account for high-DPI on macOS
                        SDL_GL_GetDrawableSize(priv.win, &rw, &rh);
                        app.handleResize(rw, rh);
                    }
                    break;
                case SDL_QUIT:
                    sys.quit();
                    break;
                default:
                    break;
            }   // end of event type switch
        }   // end of event poll loop

        // let the application render the frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        Uint64 tNow = SDL_GetPerformanceCounter();
        app.draw(tPrev ? float(double(tNow - tPrev) / double(SDL_GetPerformanceFrequency())) : 0.0f);
        tPrev = tNow;
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(priv.win);
    }

    // uninitialization
    sys.lockAudioMutex();
    priv.app = nullptr;  // stop feeding audio
    sys.unlockAudioMutex();
    app.shutdown();
    if (priv.audio) {
        SDL_CloseAudioDevice(priv.audio);
    }
    if (priv.io) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
    if (priv.ctx) {
        SDL_GL_MakeCurrent(nullptr, nullptr);
        SDL_GL_DeleteContext(priv.ctx);
    }
    if (priv.win) {
        SDL_DestroyWindow(priv.win);
    }
    SDL_Quit();
    return 0;
}
