// Must be defined in one file, _before_ #include "clay.h"
#define CLAY_IMPLEMENTATION
#include "include/clay.h"
#include "include/renderers/clay_renderer_raylib.c"
#include "include/renderers/raylib.h"
#include <stdio.h>

const Clay_Color COLOR_LIGHT = (Clay_Color) { 224, 215, 210, 255 };
const Clay_Color COLOR_RED = (Clay_Color) { 168, 66, 28, 255 };
const Clay_Color COLOR_ORANGE = (Clay_Color) { 225, 138, 50, 255 };

bool reinitializeClay = false;

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

// Re-useable components are just normal functions
void SidebarItemComponent() {
    CLAY(sidebarItemConfig) { }
}

int start_ui_app() {
    // Note: malloc is only used here as an example, any allocator that provides
    // a pointer to addressable memory of at least totalMemorySize will work
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena
        = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    printf("Starting window\n");

    // Note: screenWidth and screenHeight will need to come from your environment, Clay doesn't
    // handle window related tasks
    Clay_Raylib_Initialize(1024, 768, "pi chat",
        FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    printf("Initializing clay\n");
    int screenWidth = GetScreenWidth(), screenHeight = GetScreenHeight();
    Clay_Initialize(arena, (Clay_Dimensions) { screenWidth, screenHeight },
        (Clay_ErrorHandler) { HandleClayErrors });

    Texture2D file = LoadTexture("resources/profile-picture.png");

    Font fonts[1];
    fonts[0] = LoadFontEx("resources/Roboto-Regular.ttf", 48, 0, 400);

    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    while (!WindowShouldClose()) { // Will be different for each renderer / environment
        printf("Drawing\n");
        screenWidth = GetScreenWidth(), screenHeight = GetScreenHeight();
        Vector2 mousePosition = GetMousePosition();
        Vector2 mouseWheel = GetMouseWheelMoveV();
        double deltaTime = GetFrameTime();

        // Optional: Update internal layout dimensions to support resizing
        Clay_SetLayoutDimensions((Clay_Dimensions) { screenWidth, screenHeight });
        // Optional: Update internal pointer position for handling mouseover / click / touch events
        // - needed for scrolling & debug tools
        Clay_SetPointerState((Clay_Vector2) { mousePosition.x, mousePosition.y },
            IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        // Optional: Update internal pointer position for handling mouseover / click / touch events
        // - needed for scrolling and debug tools
        Clay_UpdateScrollContainers(true, (Clay_Vector2) { mouseWheel.x, mouseWheel.y }, deltaTime);

        printf("Layout\n");

        // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
        Clay_BeginLayout();

        // An example of laying out a UI with a fixed width sidebar and flexible width main content
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
                        .image = { .imageData = &file } }) { }
                    CLAY_TEXT(CLAY_STRING("Clay - UI Library"),
                        CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = { 255, 255, 255, 255 } }));
                }

                // Standard C code like loops etc work inside components
                for (int i = 0; i < 5; i++) {
                    SidebarItemComponent();
                }
            }

            CLAY({ .id = CLAY_ID("MainContent"),
                .layout
                = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } },
                .backgroundColor = COLOR_LIGHT }) {
                CLAY_TEXT(CLAY_STRING("Hello"),
                    CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = COLOR_RED }));
            }
        }

        // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        printf("Render\n");
        BeginDrawing();
        ClearBackground(BLACK);
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }

    UnloadTexture(file);
    printf("Shutting down");
}
