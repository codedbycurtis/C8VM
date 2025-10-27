#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_SDL3.c"

#include "arena.h"
#include "core.h"
#include "layouts.h"

static constexpr char WINDOW_TITLE[]     = "C8VM";
static constexpr int  WINDOW_WIDTH       = 1280;
static constexpr int  WINDOW_HEIGHT      = 720;
static constexpr int  DEFAULT_CLOCK_RATE = 600;

typedef struct
{
    uint64_t iterationsPerSecond;
    uint64_t cyclesPerSecond;
    uint64_t framesPerSecond;
    uint64_t ticksLastIteration;
    uint64_t ticksLastCycle;
    uint64_t ticksLastFrame;
} PerformanceMetrics;

typedef struct
{
    C8VM virtualMachine;

    SDL_Window *window;
    Clay_SDL3RendererData rendererData;

    SDL_AudioStream *audioStream;
    int currentAudioSample;

    Arena frameArena;
    Layout layout;
    LayoutData layoutData;

    PerformanceMetrics metrics;
} AppState;

Clay_Dimensions SDL_MeasureText(const Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    int width, height;
    TTF_Font **fonts = userData;
    TTF_Font *font = fonts[config->fontId];
    TTF_SetFontSize(font, config->fontSize);
    TTF_GetStringSize(font, text.chars, text.length, &width, &height);
    return (Clay_Dimensions){ (float)width, (float)height };
}

void OnClayError(const Clay_ErrorData errorData)
{
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Clay Error: [%d] %s", errorData.errorType, errorData.errorText.chars);
}

void OnAudioDataRequested(void *userData, SDL_AudioStream *stream, const int additionalAmount, const int totalAmount)
{
    if (additionalAmount == 0)
        return;

    AppState *state = userData;

    constexpr int sampleCount = 1024;
    float samples[sampleCount];

    constexpr int sampleRate = 44100;
    for (int i = 0; i < sampleCount; ++i)
    {
        constexpr int frequency = 512;
        const float phase = (float)(state->currentAudioSample * frequency) / sampleRate;
        samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
        ++state->currentAudioSample;
    }

    state->currentAudioSample %= sampleRate;

    SDL_PutAudioStreamData(stream, samples, sizeof(samples));
}

void NotifyVirtualMachineKeyEvent(AppState *state, const SDL_Scancode scancode, const bool isPressed)
{
    uint8_t key;

    switch (scancode)
    {
        case SDL_SCANCODE_1:
            key = 0x1;
            break;
        case SDL_SCANCODE_2:
            key = 0x2;
            break;
        case SDL_SCANCODE_3:
            key = 0x3;
            break;
        case SDL_SCANCODE_4:
            key = 0xC;
            break;
        case SDL_SCANCODE_Q:
            key = 0x4;
            break;
        case SDL_SCANCODE_W:
            key = 0x5;
            break;
        case SDL_SCANCODE_E:
            key = 0x6;
            break;
        case SDL_SCANCODE_R:
            key = 0xD;
            break;
        case SDL_SCANCODE_A:
            key = 0x7;
            break;
        case SDL_SCANCODE_S:
            key = 0x8;
            break;
        case SDL_SCANCODE_D:
            key = 0x9;
            break;
        case SDL_SCANCODE_F:
            key = 0xE;
            break;
        case SDL_SCANCODE_Z:
            key = 0xA;
            break;
        case SDL_SCANCODE_X:
            key = 0x0;
            break;
        case SDL_SCANCODE_C:
            key = 0xB;
            break;
        case SDL_SCANCODE_V:
            key = 0xF;
            break;
        default:
            return;
    }

    C8_NotifyKeyEvent(&state->virtualMachine.instance, key, isPressed);
}

void OnKeyEvent(void *appstate, const SDL_Scancode scancode, const bool isPressed)
{
    AppState *state = appstate;

    switch (scancode)
    {
        case SDL_SCANCODE_F11:
            // Toggle between fullscreen and windowed mode
            SDL_SetWindowFullscreen(state->window, isPressed ^ (SDL_GetWindowFlags(state->window) ^ SDL_WINDOW_FULLSCREEN) & SDL_WINDOW_FULLSCREEN);
            break;
        case SDL_SCANCODE_ESCAPE:
        {
            if (!isPressed)
                break;

            switch (state->layout)
            {
                // Back to select layout
                case LAYOUT_SETTINGS:
                case LAYOUT_CONTROLS:
                case LAYOUT_CREDITS:
                    state->layout = LAYOUT_SELECT;
                    return;
                // Pause/resume execution
                case LAYOUT_MAIN:
                    state->virtualMachine.isRunning = !state->virtualMachine.isRunning;
                    break;
                default:
                    break;
            }
        }
        default:
            break;
    }

    NotifyVirtualMachineKeyEvent(state, scancode, isPressed);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_calloc failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &state->window, &state->rendererData.renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_Init failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts = SDL_calloc(3, sizeof(TTF_Font *));
    if (!state->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_calloc failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.textEngine = TTF_CreateRendererTextEngine(state->rendererData.renderer);
    if (!state->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_CreateRendererTextEngine failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    TTF_Font *fontPixeloidSans16pt = TTF_OpenFont("./assets/Pixeloid/PixeloidSans.ttf", 16);
    TTF_Font *fontPixeloidSansBold32pt = TTF_OpenFont("./assets/Pixeloid/PixeloidSans-Bold.ttf", 32);
    TTF_Font *fontPixeloidSansBold48pt = TTF_OpenFont("./assets/Pixeloid/PixeloidSans-Bold.ttf", 48);
    if (!fontPixeloidSans16pt || !fontPixeloidSansBold32pt || !fontPixeloidSansBold48pt) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_OpenFont failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts[FONT_PIXELOID_SANS_16PT] = fontPixeloidSans16pt;
    state->rendererData.fonts[FONT_PIXELOID_SANS_BOLD_32PT] = fontPixeloidSansBold32pt;
    state->rendererData.fonts[FONT_PIXELOID_SANS_BOLD_48PT] = fontPixeloidSansBold48pt;

    const uint32_t memorySize = Clay_MinMemorySize();
    const Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, SDL_malloc(memorySize));

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);

    Clay_Initialize(arena, (Clay_Dimensions){ (float)width, (float)height }, (Clay_ErrorHandler){ .errorHandlerFunction = OnClayError });
    Clay_SetMeasureTextFunction(SDL_MeasureText, state->rendererData.fonts);

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_InitSubSystem failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const SDL_AudioSpec spec = {
        .channels = 1,
        .format = SDL_AUDIO_F32,
        .freq = 44100
    };
    state->audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, OnAudioDataRequested, state);

    state->frameArena = CreateArena(1024);

    state->layoutData = (LayoutData){
        .frameArena = &state->frameArena,
        .layout = &state->layout,
        .virtualMachine = &state->virtualMachine,
    };

    state->virtualMachine.cyclesPerSecond = DEFAULT_CLOCK_RATE;
    state->virtualMachine.instance.config = (C8_Config){
        .useParameterisedShift = true,
        .useParameterisedJump = true,
        .useTemporaryIndex = true
    };

    *appstate = state;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            OnKeyEvent(appstate, event->key.scancode, event->key.down);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            Clay_SetPointerState((Clay_Vector2){ event->button.x, event->button.y }, event->button.button == SDL_BUTTON_LEFT && event->button.down);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Clay_SetPointerState((Clay_Vector2){ event->motion.x, event->motion.y }, event->motion.state & SDL_BUTTON_LMASK);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Clay_SetLayoutDimensions((Clay_Dimensions){ (float)event->window.data1, (float)event->window.data2 });
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    constexpr uint64_t ticksPerSecond = 1000000000;
    constexpr uint64_t drawsPerSecond = 60;
    constexpr uint64_t ticksPerDraw = ticksPerSecond / drawsPerSecond;

    AppState *state = appstate;

    ++state->metrics.iterationsPerSecond;

    const Uint64 ticksPerCycle = ticksPerSecond / state->virtualMachine.cyclesPerSecond;
    const Uint64 ticksNow = SDL_GetTicksNS();

    if (ticksNow - state->metrics.ticksLastIteration > ticksPerSecond)
    {
        char title[256];
        sprintf_s(title, 256, "%s [Iterations: %llu/s, Cycles: %llu/s, Frames: %llu/s]", WINDOW_TITLE, state->metrics.iterationsPerSecond, state->metrics.cyclesPerSecond, state->metrics.framesPerSecond);
        SDL_SetWindowTitle(state->window, title);
        state->metrics.iterationsPerSecond = state->metrics.cyclesPerSecond = state->metrics.framesPerSecond = 0;
        state->metrics.ticksLastIteration = ticksNow;
    }

    if (ticksNow - state->metrics.ticksLastCycle >= ticksPerCycle && state->virtualMachine.isRunning)
    {
        ++state->metrics.cyclesPerSecond;
        state->metrics.ticksLastCycle = ticksNow;
        C8_FetchExecute(&state->virtualMachine.instance);
    }

    if (ticksNow - state->metrics.ticksLastFrame > ticksPerDraw)
    {
        ++state->metrics.framesPerSecond;
        state->metrics.ticksLastFrame = ticksNow;

        if (state->virtualMachine.isRunning)
            C8_UpdateTimers(&state->virtualMachine.instance);

        const bool isAudioDevicePaused = SDL_AudioStreamDevicePaused(state->audioStream);
        if (state->virtualMachine.instance.st > 0 && isAudioDevicePaused)
            SDL_ResumeAudioStreamDevice(state->audioStream);
        else if (state->virtualMachine.instance.st <= 0 && !isAudioDevicePaused)
            SDL_PauseAudioStreamDevice(state->audioStream);

        Clay_RenderCommandArray renderCommands;
        switch (state->layout)
        {
            case LAYOUT_SELECT:
                renderCommands = SelectLayout_CreateLayout(&state->layoutData);
                break;
            case LAYOUT_SETTINGS:
                renderCommands = SettingsLayout_CreateLayout(&state->layoutData);
                break;
            case LAYOUT_CONTROLS:
                renderCommands = ControlsLayout_CreateLayout(&state->layoutData);
                break;
            case LAYOUT_CREDITS:
                renderCommands = CreditsLayout_CreateLayout(&state->layoutData);
                break;
            case LAYOUT_MAIN:
                renderCommands = MainLayout_CreateLayout(&state->layoutData);
                break;
        }

        SDL_SetRenderDrawColor(state->rendererData.renderer, 0, 0, 0, 255);
        SDL_RenderClear(state->rendererData.renderer);

        SDL_Clay_RenderClayCommands(&state->rendererData, &renderCommands);

        SDL_RenderPresent(state->rendererData.renderer);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState *state = appstate;

    if (state)
    {
        if (state->rendererData.renderer)
            SDL_DestroyRenderer(state->rendererData.renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        if (state->rendererData.fonts)
        {
            for (size_t i = 0; i < sizeof(state->rendererData.fonts) / sizeof(*state->rendererData.fonts); ++i)
                TTF_CloseFont(state->rendererData.fonts[i]);

            SDL_free(state->rendererData.fonts);
        }

        if (state->rendererData.textEngine)
            TTF_DestroyRendererTextEngine(state->rendererData.textEngine);

        FreeArena(&state->frameArena);

        SDL_free(state);
    }

    TTF_Quit();
}