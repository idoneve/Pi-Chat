#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <style.h>
#include <components.h>
#include <clay.h>
#include <app.h>
#include <clay_renderer_raylib.c>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.h"
#include "raylib.h"
#include "ui_models.h"

extern bool reinitializeClay;

AppState initialize_app(Font*) {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena
        = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Dimensions dimensions = (Clay_Dimensions) {
        .width = (float)GetScreenWidth(),
        .height = (float)GetScreenHeight(),
    };

    Vector2 mousePosition = GetMousePosition();
    Vector2 mouseScroll = GetMouseWheelMoveV();

    Clay_Initialize(arena, dimensions, (Clay_ErrorHandler) { HandleClayErrors, NULL });
    Clay_Raylib_Initialize(1024, 768, "pi chat",
        FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

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

AppResources* load_resources() {
    AppResources* resources = malloc(sizeof(AppResources));

    resources->fonts[REGULAR_24] = LoadFontEx("./resources/Roboto-Regular.ttf", 24, NULL, 400);
    resources->fonts[MONO_16] = LoadFontEx("./resources/RobotoMono-Medium.ttf", 16, NULL, 400);

    printf("[CLIENT] Glyphys: %b\n",
        resources->fonts[REGULAR_24].glyphs != 0 && resources->fonts[REGULAR_24].glyphs != 0);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, resources->fonts);
    SetTextureFilter(resources->fonts[REGULAR_24].texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(resources->fonts[MONO_16].texture, TEXTURE_FILTER_BILINEAR);

    return resources;
}

void unload_resources(AppResources* resources) {
    for (size_t i = 0; i < sizeof(resources->fonts) / sizeof(Font); i++) {
        UnloadFont(resources->fonts[i]);
    }
    free(resources);
}

void draw_app(Clay_RenderCommandArray render_commands, AppResources* resources) {
    BeginDrawing();
    ClearBackground(BLACK);
    Clay_Raylib_Render(render_commands, resources->fonts);
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

    state->screenDimensions = (Clay_Dimensions) { .width = (float)GetScreenWidth(),
        .height = (float)GetScreenHeight() };

    state->mouse.position = (Clay_Vector2) { .x = mousePosition.x, .y = mousePosition.y };
    state->mouse.scroll = (Clay_Vector2) { .x = mouseScroll.x, .y = mouseScroll.y };
    state->deltaTime = GetFrameTime();

    Clay_SetLayoutDimensions(state->screenDimensions);
    Clay_SetPointerState(state->mouse.position, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    Clay_UpdateScrollContainers(true, state->mouse.scroll, (float)state->deltaTime);
}

Clay_RenderCommandArray get_layout(const AppResources*, AppModel* model) {
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

// Returns number of messages received
static Messages listen_for_messages(int socket_fd) {
    Messages result = { .internal = init_list(sizeof(Message), 0) };

    int bytes_avaliable;
    while (ioctl(socket_fd, FIONREAD, &bytes_avaliable) == 0 && bytes_avaliable != 0) {
        Message m = receive_message(socket_fd);
        append_list(&result.internal, &m);
    }

    return result;
}

static Connections init_connections(int server) {
    return (Connections) { .internal = init_list(sizeof(Connection), 0), .server = server };
}

static void deinit_connections(Connections* connections) {
    for (size_t i = 0; i < connections->internal.len; i++) {
        Connection* c = get_list(connections->internal, i);
        for (size_t j = 0; j < c->messages.internal.len; j++) {
            ClientMessage* m = get_list(c->messages.internal, j);
            free(m->content.data);
        }
    }

    deinit_list(&connections->internal);
}

static void add_connection(Connections* connections, Connection connection) {
    append_list(&connections->internal, &connection);
}

// Returns null if selected > connections.len
Connection* get_selected_connection(Connections connections) {
    if (connections.internal.len == 0) {
        return NULL;
    }
    return get_list(connections.internal, connections.selected);
}

static Connection* map_message_to_connection(AppModel* model, ClientMessage* message) {
    Connection* match = NULL;

    // we know the data is a RECEIVE message
    char* message_source = message->ip;
    size_t source_len = sizeof(message->ip);
    for (size_t i = 0; i < model->connections.internal.len; i++) {
        Connection* connection = get_list(model->connections.internal, i);

        if (strncmp(connection->dest, message_source, source_len) == 0) {
            match = connection;
        }
    }

    if (match == NULL) {
        // add new connection
        Connection new_connection = (Connection) { 
            .messages = {
                .internal = init_list(sizeof(ClientMessage), 5),
            }, 
            .is_active = true, .user_input = {.len = 0, .cursor = 0}};
        memcpy(new_connection.dest, message_source, sizeof(new_connection.dest));

        add_connection(&model->connections, new_connection);
        match = get_list(model->connections.internal, model->connections.internal.len - 1);
    }

    return match;
}

static void add_message_to_connection(Connection* connection, ClientMessage* message) {
    append_list(&connection->messages.internal, message);
}

static bool update_connection_activity(AppModel* model, const ActivityMessage* activity) {
    bool matched = false;
    for (size_t i = 0; i < model->connections.internal.len; i++) {
        Connection* connection = get_list(model->connections.internal, i);

        if (strncmp(connection->dest, activity->ip, HEADER_ADDR_SIZE) == 0) {
            connection->is_active = activity->active;
            matched = true;
        }
    }
    return matched;
}

static int update_connections(AppModel* model) {
    Messages incoming;
    while ((incoming = listen_for_messages(model->connections.server)).internal.len != 0) {
        ClientMessage* client_message = NULL;

        for (size_t i = 0; i < incoming.internal.len; i++) {
            Message* message = get_list(incoming.internal, i);

            switch (message->type) {
            case ACTIVITY:
                ActivityMessage* activity = &message->type_data.activity;
                if (!update_connection_activity(model, activity)) {

                    Connection c = {
                        .is_active = activity->active,
                        .messages = init_list(sizeof(Message), 0),
                        .user_input = { },
                    };
                    memcpy(c.dest, activity->ip, HEADER_ADDR_SIZE);

                    add_connection(&model->connections, c);
                }
                break;
            case MESSAGE:
                client_message = &message->type_data.message;
                Connection* connection = map_message_to_connection(model, client_message);
                add_message_to_connection(connection, client_message);
                break;
            case INVALID:
                printf("[ERROR] Client receieved invalid message from server\n");
                continue;
            case DISCONNECT:
                printf("[CLIENT] Server disconnect message receieved\n");
                return -1;
                break;
            }
        }
    }
    return 0;
}

void update_app_model(AppModel* model) {
    if (update_connections(model) < 0) {
        // TODO process server disconnecct
        close(model->connections.server);
        model->connections.server = -1;
        return;
    };

    printf("\t[CLIENT] connections updated\n");
    if (model->connections.internal.len != model->tabs.len) {
        printf("\t[CLIENT] [UI] generating tabs\n");
        if (model->connections.internal.len == 0)
            return;

        if (model->tabs.len > 0)
            free(model->tabs.data);

        model->tabs.len = model->connections.internal.len;
        model->tabs.data = malloc(sizeof(TabModel) * model->tabs.len);

        if (model->tabs.data == NULL) {
            printf("\t[CLIENT] [UI] [ERROR] - Failed to allocate tabs data\n");
            exit(1);
        }
    }

    TabModel* tabs = model->tabs.data;
    for (size_t i = 0; i < model->tabs.len; i++) {
        TabModel* tab = &tabs[i];
        const Connection* connection = get_list(model->connections.internal, i);

        tab->index = i;
        tab->is_active = connection->is_active;

        sprintf(tab->title, "%ld: %s", i, connection->dest);
    }
}

AppModel init_app_model(int fd) {
    return (AppModel) {
        .connections = init_connections(fd),
        .tabs = { .data = NULL, .len = 0 },
    };
}

void deinit_app_model(AppModel* model) {
    for (size_t i = 0; i < model->connections.internal.len; i++) {
        Connection* connection = get_list(model->connections.internal, i);
        deinit_list(&connection->messages.internal);
    }

    deinit_connections(&model->connections);
    free(model->tabs.data);
}
