#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <vector>

#include <SDL.h>
#include <glad/glad.h>
#include <libopenmpt/libopenmpt.hpp>

#ifdef _DEBUG
    #define Dprintf printf
#else
    #define Dprintf (void)
#endif

[[noreturn]] static void fatal(const char *what, const char *how) {
    #if defined(_WIN32) && defined(NDEBUG)
        MessageBoxA(nullptr, how, what, MB_OK | MB_ICONERROR);
    #else
        fprintf(stderr, "FATAL: %s - %s\n", what, how);
    #endif
    std::exit(1);
}

///////////////////////////////////////////////////////////////////////////////

openmpt::module *mod = nullptr;
std::vector<std::byte> modData;
int sampleRate = 48000;
bool paused = true;
SDL_AudioDeviceID audioDevice = 0;

void SetPaused(bool newPaused) {
    paused = newPaused && (mod != nullptr);
    SDL_PauseAudioDevice(audioDevice, paused ? SDL_TRUE : SDL_FALSE);
}

void RenderAudio(void*, Uint8* stream, int len) {
    if (mod) {
        mod->read_interleaved_stereo(sampleRate, len >> 2, (int16_t*)stream);
    } else {
        SDL_memset(stream, 0, len);
    }
}

bool LoadModule(const char* path) {
    bool res = false;
    SetPaused(true);
    SDL_LockAudioDevice(audioDevice);
    delete mod;
    mod = nullptr;
    modData.clear();
    SDL_UnlockAudioDevice(audioDevice);
    Dprintf("module unloaded, playback paused\n");
    if (!path || !path[0]) { return res; }

    Dprintf("loading module: %s\n", path);
    FILE *f = fopen(path, "rb");
    if (!f) { Dprintf("could not open module file.\n"); return res; }
    fseek(f, 0, SEEK_END);
    modData.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    res = (fread(modData.data(), 1, modData.size(), f) == modData.size());
    fclose(f);
    if (res) { mod = new openmpt::module(modData); }
    if (mod) {
        Dprintf("module loaded successfully, starting playback.\n");
        SetPaused(false);
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    (void) argc, (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fatal("SDL initialization failed", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,            0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,          0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    #ifdef _DEBUG
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,     SDL_GL_CONTEXT_DEBUG_FLAG);
    #endif

    SDL_Window *win = SDL_CreateWindow(
        "Tracked Music Compo Player",
        #ifdef _DEBUG
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0
        #else
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_FULLSCREEN_DESKTOP
        #endif
        | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) {
        fatal("could not create window", SDL_GetError());
    }
    #ifdef NDEBUG
        SDL_ShowCursor(SDL_DISABLE);
    #endif

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        fatal("could not create OpenGL context", SDL_GetError());
    }
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);
    if (!gladLoadGL()) {
        fatal("could not initialize OpenGL", "failed to load OpenGL functions");
    }
    #ifdef _DEBUG
        printf("OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
        printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
        printf("OpenGL version:  %s\n", glGetString(GL_VERSION));
        printf("GLSL   version:  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    #endif

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    SDL_AudioSpec requestedFormat, audioFormat;
    SDL_memset(&requestedFormat, 0, sizeof(requestedFormat));
    requestedFormat.freq = 48000;
    requestedFormat.format = AUDIO_S16SYS;
    requestedFormat.channels = 2;
    requestedFormat.samples = 512;
    requestedFormat.callback = RenderAudio;
    audioDevice = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &requestedFormat, &audioFormat, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (!audioDevice) {
        fatal("could not open audio device", SDL_GetError());
    }
    sampleRate = audioFormat.freq;

    if (argc > 1) { LoadModule(argv[1]); }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    bool active = true;
    while (active) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_q:
                            active = false;
                            break;
                        case SDLK_SPACE:
                            SetPaused(!paused);
                            break;
                        case SDLK_LEFT:
                            if (mod) {
                                int32_t dest = mod->get_current_order() - 1;
                                Dprintf("seeking to order %d\n", dest);
                                if (dest >= 0) {
                                    SDL_LockAudioDevice(audioDevice);
                                    mod->set_position_order_row(dest, 0);
                                    SDL_UnlockAudioDevice(audioDevice);
                                }
                            }
                            break;
                        case SDLK_RIGHT:
                            if (mod) {
                                int32_t dest = mod->get_current_order() + 1;
                                Dprintf("seeking to order %d/%d\n", dest, mod->get_num_orders());
                                if (dest < mod->get_num_orders()) {
                                    SDL_LockAudioDevice(audioDevice);
                                    mod->set_position_order_row(dest, 0);
                                    SDL_UnlockAudioDevice(audioDevice);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    if (ev.key.keysym.sym == SDLK_q) { active = false; }
                    break;
                case SDL_DROPFILE:
                    LoadModule(ev.drop.file);
                    SDL_free(ev.drop.file);
                    break;
                case SDL_QUIT:
                    active = false;
                    break;
                default:
                    break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

if (mod && !paused) { printf("@ %03d.%02X \r", mod->get_current_order(), mod->get_current_row()); fflush(stdout); }
        SDL_GL_SwapWindow(win);
    }

    LoadModule(nullptr);
    SDL_CloseAudioDevice(audioDevice);
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
