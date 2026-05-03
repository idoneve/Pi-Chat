#include "app.h"
#include "style.h"
#include "ui_models.h"
#include <../../chat.h>
#include <arpa/inet.h>
#include <components.h>
#include <netinet/in.h>
#include <stddef.h>

void connection_tab(const TabModel* model) {
    // Supports three digit numbers and ": " in between name and index
    CLAY({ .id = CLAY_IDI("ConnectionTab", model->index),
        .backgroundColor = Clay_Hovered() ? COLOR_HOVER_HIGHLIGHT : COLOR_HOVERLESS_HIGHLIGHT,
        .cornerRadius = CLAY_CORNER_RADIUS(10),
        .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .padding = CLAY_PADDING_ALL(5),
            .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIT() } } }) {
        CLAY_TEXT(((Clay_String) { .chars = model->title,
                      .length = sizeof(model->title),
                      .isStaticallyAllocated = false }),
            CLAY_TEXT_CONFIG({
                .textColor = COLOR_TAB_TEXT,
                .fontSize = 15,
                .textAlignment = CLAY_TEXT_ALIGN_LEFT,
            }));

        const int status_circle_size = 10;

        CLAY({ .id = CLAY_IDI("StatusCircle", model->index),
            .cornerRadius = CLAY_CORNER_RADIUS(.5),
            .layout = { .sizing = { .width = CLAY_SIZING_FIXED(status_circle_size),
                            .height = CLAY_SIZING_FIXED(status_circle_size) } },
            .backgroundColor = model->is_active ? (Clay_Color) { 0, 255, 0, 255 }
                                                : (Clay_Color) { 255, 0, 0, 255 } });
    }
}

void chat_window(const AppModel* model) {
    CLAY({ .id = CLAY_ID("ChatWindow") }) { }
}
