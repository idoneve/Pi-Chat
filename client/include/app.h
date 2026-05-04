#pragma once

// #ifndef CLAY_IMPLEMENTATION
// #error "This file include may not come before clay implementation defined"
// #endif // !CLAY_IMPLEMENTATION
#include "chat.h"
#include "clay.h"
#include "raylib.h"
#include <netinet/in.h>
#include <stddef.h>
#include <sys/select.h>
#include <ui_models.h>

// Contains UI rendering and interaction related data
typedef struct {
    Clay_Arena arena;
    uint64_t memorySize;
    Clay_Dimensions screenDimensions;

    struct {
        Clay_Vector2 position;
        Clay_Vector2 scroll;
    } mouse;

    double deltaTime;
} AppState;

// Contains any external data needed for the app (Fonts, Images, Etc)
typedef struct {
    Font fonts[2];
} AppResources;

// Represents a connection
typedef struct {
    // Messages
    struct {
        ClientMessage* data;
        size_t len;
        size_t cap;
    } messages;

    struct {
        char data[MAX_MSG_LEN + 1];
        size_t len;
        size_t cursor;
    } user_input;

    // Readable name of ip destination
    char dest[INET_ADDRSTRLEN];
    bool is_active;
} Connection;

// Contains things necessary to create ui
typedef struct {
    struct {
        Connection* data;
        size_t len;
        size_t cap;
        size_t selected;
    } connections;
    struct {
        TabModel* data;
        size_t len;
    } tabs;
} AppModel;

Clay_RenderCommandArray get_layout(const AppResources* resources, AppModel* model);

void update_app_state(AppState* state);

void update_app_model(int socket_fd, AppModel* model);

AppState initialize_app(Font* fonts);

void reinitialize_app(AppState* state);

void uninitialize_app(void);

AppResources* load_resources();
void unload_resources(AppResources* resources);

void draw_app(Clay_RenderCommandArray render_commands, AppResources* resources);

void HandleClayErrors(Clay_ErrorData);

AppModel init_app_model(void);

void deinit_app_model(AppModel*);
