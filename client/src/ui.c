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

    AppModel model = init_app_model();

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
        update_app_model(socket_fd,&model);
        Clay_RenderCommandArray render_commands = get_layout(&resources, &model);
        draw_app(render_commands, &resources);
    }
    printf("[CLIENT] Ending Render Loop. Cleaning up UI data.\n");

    unload_resources(&resources);
    deinit_app_model(&model);
    uninitialize_app();

    printf("[CLIENT] Finished cleaning UI data.\n");

    return 0;
}
