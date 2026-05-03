#define CLAY_IMPLEMENTATION
#include <stdio.h>
#include <app.h>
#include <clay.h>
#include <components.h>
#include <ui.h>
#include "style.h"
#include "raylib.h"

bool reinitializeClay = false;

int start_ui_app(int socket_fd) {
    Font fonts[2];
    fonts[REGULAR_24] = LoadFontEx("./resources/Roboto-Regular.ttf", 48, NULL, 0);
    fonts[MONO_16] = LoadFontEx("./resources/RobotoMono-Medium.ttf", 32, NULL, 0);

    printf("[CLIENT] Initializing App\n");
    AppState state = initialize_app(fonts);
    printf("[CLIENT] Loading Resources\n");
    AppResources resources = load_resources(fonts, 2);

    Message messages1[5] = {
        (Message) { .source = NULL, .content = { "Test message", 12 } },
        (Message) { .source = "192.168.0.255", .content = { .data = "Does this", .len = 9 } },
        (Message) { .source = "192.168.0.255", .content = { "Work", 4 } },
        (Message) { .source = NULL, .content = { "I think", 7 } },
        (Message) { .source = NULL, .content = { "but who freakin knows", 21 } },
    };

    // ---  DEBUG DATA ---
    Connection connections[2] = {
        (Connection) { .messages = {
            .data = messages1,
            .len = 5,
        }, .dest = "192.168.0.255", .is_active = true },
        (Connection) { .messages = {}, .dest = "192.168.0.250", .is_active = false },
    };

    AppModel model = { .connections = { .data = connections, .len = 2 }, .tabs = { } };

    bool enable_debug_mode = false;
    // -------------------

    printf("[CLIENT] Beginning Render Loop\n");
    while (!WindowShouldClose()) {
        if (reinitializeClay) {
            reinitialize_app(&state);
            reinitializeClay = false;
        }

        if (IsKeyPressed(KEY_D) && IsKeyDown(KEY_LEFT_CONTROL)) {
            enable_debug_mode = !enable_debug_mode;
            Clay_SetDebugModeEnabled(enable_debug_mode);
        }

        update_app_state(&state);
        update_app_model(&model);
        Clay_RenderCommandArray render_commands = get_layout(&resources, &model);
        draw_app(render_commands, &resources);
    }

    unload_resources(&resources);

    return 0;
}
