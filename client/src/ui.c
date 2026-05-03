// Must be defined in one file, _before_ #include "clay.h"
#include "raylib.h"
#include <emmintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#define CLAY_IMPLEMENTATION
#include <ui.h>
#include <clay.h>
#include "clay_renderer_raylib.c"
#include <stdio.h>

const Clay_Color COLOR_LIGHT = (Clay_Color) { 224, 215, 210, 255 };
const Clay_Color COLOR_RED = (Clay_Color) { 168, 66, 28, 255 };
const Clay_Color COLOR_ORANGE = (Clay_Color) { 225, 138, 50, 255 };

bool reinitializeClay = false;

typedef struct {
    Clay_Arena arena;
    uint32_t memorySize;
    Clay_Dimensions screenDimensions;

    struct {
        Clay_Vector2 position;
        Clay_Vector2 scroll;
    } mouse;

    double deltaTime;
} AppState;

typedef struct {
    Texture2D profilePicture;
} AppResources;

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    } else if (errorData.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}

// Layout config is just a struct that can be declared statically, or inline
Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration) { .layout
    = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) } },
    .backgroundColor = COLOR_ORANGE };

AppState initialize_app(Font* fonts, size_t font_len) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena
        = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Raylib_Initialize(
        1024, 768, "pi chat", FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    Clay_Dimensions dimensions = (Clay_Dimensions) {
        .width = GetScreenWidth(),
        .height = GetScreenHeight(),
    };

    Vector2 mousePosition = GetMousePosition();
    Vector2 mouseScroll = GetMouseWheelMoveV();

    Clay_Initialize(arena, dimensions, (Clay_ErrorHandler) { HandleClayErrors });

    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    return (AppState) {
        .arena = arena, .memorySize = totalMemorySize,
        .mouse = {
            .position = (Clay_Vector2) { .x = mousePosition.x, .y = mousePosition.y },
            .scroll = (Clay_Vector2) { .x = mouseScroll.x, .y = mouseScroll.y }, 
        },
        .screenDimensions = dimensions,
        .deltaTime = 0,
    };
}

void reinitialize_app(AppState* state) {
    Clay_SetMaxElementCount(8192);
    uint32_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena
        = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(arena, (Clay_Dimensions) { (float)GetScreenWidth(), (float)GetScreenHeight() },
        (Clay_ErrorHandler) { HandleClayErrors, 0 });

    state->arena = arena;
    state->memorySize = totalMemorySize;
}

void update_app_state(AppState* state) {
    Vector2 mousePosition = GetMousePosition();
    Vector2 mouseScroll = GetMouseWheelMoveV();

    state->screenDimensions
        = (Clay_Dimensions) { .width = GetScreenWidth(), .height = GetScreenHeight() };

    state->mouse.position = (Clay_Vector2) { .x = mousePosition.x, .y = mousePosition.y };
    state->mouse.scroll = (Clay_Vector2) { .x = mouseScroll.x, .y = mouseScroll.y };
    state->deltaTime = GetFrameTime();

    Clay_SetLayoutDimensions(state->screenDimensions);
    Clay_SetPointerState(state->mouse.position, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    Clay_UpdateScrollContainers(true, state->mouse.scroll, state->deltaTime);
}

Clay_RenderCommandArray get_layout(const AppState* state, const AppResources* resources) {
    Clay_BeginLayout();

    CLAY({ .id = CLAY_ID("OuterContainer"),
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16 },
        .backgroundColor = { 250, 250, 255, 255 } }) {
        CLAY({ .id = CLAY_ID("SideBar"),
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16 },
            .backgroundColor = COLOR_LIGHT }) {

            CLAY({ .id = CLAY_ID("ProfilePictureOuter"),
                .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(16),
                    .childGap = 16,
                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                .backgroundColor = COLOR_RED }) {

                CLAY({ .id = CLAY_ID("ProfilePicture"),
                    .layout = { .sizing
                        = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) } },
                    .image = { .imageData = (Texture2D*)(&resources->profilePicture) } }) { }

                CLAY_TEXT(CLAY_STRING("Clay - UI Library"),
                    CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = { 255, 255, 255, 255 } }));
            }

            // Standard C code like loops etc work inside components
            for (int i = 0; i < 5; i++) { }
        }

        CLAY({ .id = CLAY_ID("MainContent"),
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } },
            .backgroundColor = COLOR_LIGHT }) {
            CLAY_TEXT(
                CLAY_STRING("Hello"), CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = COLOR_RED }));
        }
    }

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout();
}

AppResources load_resources() {
    return (AppResources) { .profilePicture = LoadTexture("resources/profile-picture.png") };
}

int start_ui_app(int socket_fd) {
    const size_t FONTS_LEN = 1;
    Font fonts[FONTS_LEN];
    fonts[0] = LoadFontEx("resources/Roboto-Regular.ttf", 48, 0, 400);

    AppState state = initialize_app(fonts, FONTS_LEN);
    AppResources resources = load_resources();

    while (!WindowShouldClose()) {
        if (reinitializeClay) {
            reinitialize_app(&state);
            reinitializeClay = false;
        }

        update_app_state(&state);
        Clay_RenderCommandArray renderCommands = get_layout(&state, &resources);

        printf("Render\n");
        BeginDrawing();
        ClearBackground(BLACK);
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }

    UnloadTexture(resources.profilePicture);

    return 0;
}
