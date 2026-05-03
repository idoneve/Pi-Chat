#pragma once

// #ifndef CLAY_IMPLEMENTATION
// #error "This file include may not come before clay implementation defined"
// #endif // !CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib.h"
#include <stddef.h>
#include <ui_models.h>

// Contains UI rendering and interaction related data
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

// Contains any external data needed for the app (Fonts, Images, Etc)
typedef struct {
    Texture2D profilePicture;
    struct {
        Font* data;
        size_t len;
    } fonts;
} AppResources;

// Represents a message
typedef struct {
    // Message data
    struct {
        char* data;
        size_t len;
    } content;

    // Readable Ip Source
    char* source;
} Message;

// Represents a connection
typedef struct {
    // Messages
    struct {
        Message* data;
        size_t len;
    } messages;

    // Readable name of ip destination
    char* dest;
    bool is_active;
} Connection;

// Contains things necessary to create ui
typedef struct {
    struct {
        Connection* data;
        size_t len;
    } connections;
    struct {
        TabModel* data;
        size_t len;
    } tabs;
} AppModel;

Clay_RenderCommandArray get_layout(const AppResources* resources, const AppModel* model);

void update_app_state(AppState* state);

void update_app_model(AppModel* model);

AppState initialize_app(Font* fonts);

void reinitialize_app(AppState* state);

AppResources load_resources(Font* fonts, size_t font_count);
void unload_resources(AppResources* resources);

void draw_app(Clay_RenderCommandArray render_commands, const AppResources* resources);

void HandleClayErrors(Clay_ErrorData);
