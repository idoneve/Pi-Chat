#include <emmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <style.h>
#include <components.h>
#include <clay.h>
#include <app.h>
#include <clay_renderer_raylib.c>
#include <sys/select.h>
#include "chat.h"
#include "raylib.h"
#include "ui_models.h"

extern bool reinitializeClay;

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

void uninitialize_app(void) { Clay_Raylib_Close(); }

AppResources load_resources(Font* fonts, size_t font_count) {
    if (fonts == NULL) {
        printf("[ERROR] Failed to allocate fonts\n");
        exit(1);
    }

    return (AppResources) {
        .fonts = { .data = fonts, .len = font_count },
    };
}

void unload_resources(AppResources* resources) {
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

size_t listen_for_messages(int socket_fd, Message* buf, size_t buf_len) {
    // TODO - Poll server for incoming events - do not block if no events ready
    // TODO - Parse message header for connection information.
    //   if message is activity request return activity message
}

Connection* map_message_to_connection(AppModel* model, ClientMessage* message) {
    Connection* match = NULL;
    for (size_t i = 0; i < model->connections.len; i++) {

        if (strcmp(model->connections.data[i].dest, message->source)) {
            match = &model->connections.data[i];
        }
    }

    if (match == NULL) {
        if (model->connections.len == model->connections.cap) {
            // reallocate connection buffer and copy data
            Connection* old_data = model->connections.data;

            model->connections.cap = model->connections.len * 2;
            model->connections.data = malloc(sizeof(Connection) * model->connections.cap);

            memcpy(model->connections.data, old_data, model->connections.len);
            free(old_data);
        }

        Connection* end = &model->connections.data[model->connections.len];

        // add new connection
        *end = (Connection) { .messages = {
                                  .data = malloc(sizeof(ClientMessage) * 5),
                                  .len = 0,
                                  .cap = 5,
                              } };

        // copy source char[16] into connection dest char[16]
        memcpy(end->dest, message->source, sizeof(message->source));

        model->connections.len += 1;

        match = &model->connections.data[model->connections.len - 1];
    }

    return match;
}

void add_message_to_connection(Connection* connection, ClientMessage* message) {
    if (connection->messages.len == connection->messages.cap) {
        // Resize message buffer
        ClientMessage* old_data = connection->messages.data;

        connection->messages.cap = connection->messages.len * 2;
        connection->messages.data = malloc(sizeof(ClientMessage) * connection->messages.cap);
        memcpy(connection->messages.data, old_data, connection->messages.len);
        free(old_data);
    }

    // add new message
    connection->messages.data[connection->messages.len] = *message;
    connection->messages.len++;
}

void update_connections(int socket_fd, AppModel* model) {
    Message incoming[5];
    size_t recieved;
    while ((recieved = listen_for_messages(socket_fd, incoming, 5)) != 0) {
        ClientMessage* client_message = NULL;

        for (size_t i = 0; i < recieved; i++) {
            Message message = incoming[i];

            switch (message.type) {
            case ACTIVITY:
                // Send response to server
                break;
            case MESSAGE:
                client_message = &message.data.message;
                Connection* connection = map_message_to_connection(model, client_message);
                add_message_to_connection(connection, client_message);
                break;
            }
        }
    }
}

void update_app_model(int socket_fd, AppModel* model) {
    update_connections(socket_fd, model);
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

AppModel init_app_model(void) {
    return (AppModel) { 
            .connections = {
                .data = malloc(sizeof(Connection)),
                .len = 0,
                .cap = 1,
            },
           .tabs = {.data = NULL, .len = 0},
    };
}

void deinit_app_model(AppModel* model) {
    for (size_t i = 0; i < model->connections.len; i++) {
        Connection* connection = &model->connections.data[i];
        for (size_t j = 0; j < connection->messages.len; j++) {
            ClientMessage* message = &connection->messages.data[j];
            free(message->content.data);
        }
        free(connection->messages.data);
        free(connection->user_input.data);
    }

    free(model->connections.data);
    free(model->tabs.data);
}
