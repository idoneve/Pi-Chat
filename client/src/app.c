#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <style.h>
#include <components.h>
#include <clay.h>
#include <app.h>
#include <clay_renderer_raylib.c>
#include "raylib.h"
#include "ui_models.h"

extern bool reinitializeClay;

// Layout config is just a struct that can be declared statically, or inline
Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration) { .layout
    = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) } },
    .backgroundColor = COLOR_ORANGE };

AppState initialize_app(Font* fonts) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena
        = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Raylib_Initialize(1024, 768, "pi chat",
        FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    Clay_Dimensions dimensions = (Clay_Dimensions) {
        .width = GetScreenWidth(),
        .height = GetScreenHeight(),
    };

    Vector2 mousePosition = GetMousePosition();
    Vector2 mouseScroll = GetMouseWheelMoveV();

    Clay_Initialize(arena, dimensions, (Clay_ErrorHandler) { HandleClayErrors, NULL });

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

void uninitialize_app() { Clay_Raylib_Close(); }

AppResources load_resources(Font* fonts, size_t font_count) {
    if (fonts == NULL) {
        printf("[ERROR] Failed to allocate fonts\n");
        exit(1);
    }

    return (AppResources) {
        .profilePicture = LoadTexture("resources/profile-picture.png"),
        .fonts = { .data = fonts, .len = font_count },
    };
}

void unload_resources(AppResources* resources) {
    UnloadTexture(resources->profilePicture);
    for (size_t i = 0; i < resources->fonts.len; i++) {
        UnloadFont(resources->fonts.data[i]);
    }
}

void draw_app(Clay_RenderCommandArray render_commands, const AppResources* resources) {
    BeginDrawing();
    ClearBackground(BLACK);
    Clay_Raylib_Render(render_commands, resources->fonts.data);
    EndDrawing();
}

void reinitialize_app(AppState* state) {
    Clay_SetMaxElementCount(8192);
    state->memorySize = Clay_MinMemorySize();
    state->arena
        = Clay_CreateArenaWithCapacityAndMemory(state->memorySize, malloc(state->memorySize));
    Clay_Initialize(state->arena,
        (Clay_Dimensions) { (float)GetScreenWidth(), (float)GetScreenHeight() },
        (Clay_ErrorHandler) { HandleClayErrors, 0 });
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

Clay_RenderCommandArray get_layout(const AppResources* resources, AppModel* model) {
    Clay_BeginLayout();

    CLAY({ .id = CLAY_ID("OuterContainer"),
        .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16 },
        .backgroundColor = { 250, 250, 255, 255 } }) {

        side_bar(model);

        chat_window(model);
    }

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    return Clay_EndLayout();
}

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

void update_app_model(AppModel* model) {
    if (model->connections.len != model->tabs.len) {
        if (model->connections.len == 0)
            return;

        if (model->tabs.len > 0)
            free(model->tabs.data);

        model->tabs.len = model->connections.len;
        model->tabs.data = malloc(sizeof(TabModel) * model->tabs.len);

        if (model->tabs.data == NULL) {
            printf("[CLIENT] [UI] [ERROR] - Failed to allocate tabs data");
            exit(1);
        }
    }

    TabModel* tabs = model->tabs.data;
    for (size_t i = 0; i < model->tabs.len; i++) {
        TabModel* tab = &tabs[i];
        const Connection* connection = &model->connections.data[i];

        tab->index = i;
        tab->is_active = connection->is_active;

        sprintf(tab->title, "%ld: %s", i, connection->dest);
    }
}
