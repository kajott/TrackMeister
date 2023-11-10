#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <vector>

#include <SDL.h>
#include <glad/glad.h>
#include <libopenmpt/libopenmpt.hpp>

[[noreturn]] static void fatal(const char *what, const char *how) {
    #if defined(_WIN32) && defined(NDEBUG)
        MessageBoxA(nullptr, how, what, MB_OK | MB_ICONERROR);
    #else
        fprintf(stderr, "FATAL: %s - %s\n", what, how);
    #endif
    std::exit(1);
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

    FILE *f = fopen("D:\\Sound\\_mod\\krunkd.mod", "rb");
    if (!f) { fatal("sorry", "could not open file"); }
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<std::byte> modData(fsize);
    fread(modData.data(), 1, fsize, f);
    fclose(f);
    openmpt::module mod(modData);

    f = fopen("abfall.raw", "wb");
    for (int frame = 1024;  frame;  --frame) {
        short buf[4096];
        mod.read_interleaved_stereo(48000, 2048, buf);
        fwrite(buf, 2, 4096, f);
    }
    fclose(f);

    bool active = true;
    while (active) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_q) { active = false; }
                    break;
                case SDL_QUIT:
                    active = false;
                    break;
                default:
                    break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(win);
    }

    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
