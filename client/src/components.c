#include "app.h"
#include "raylib.h"
#include "style.h"
#include "ui_models.h"
#include <../../chat.h>
#include <arpa/inet.h>
#include <components.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>

void chat_message(size_t index, const Message* message) {
    CLAY({
            .id = CLAY_IDI("ChatMessage", index),
            .layout = { 
                .padding = CLAY_PADDING_ALL(PADDING_CHAT_MESSAGE_SURROUND),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment
                    = { .x = (message->source == NULL ? CLAY_ALIGN_X_RIGHT : CLAY_ALIGN_X_LEFT), },
                .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIT() },
                .childGap = GAP_CHAT_MESSAGE_INNER 
        }}) {

        // Sender Name
        char* sender_id = message->source == NULL ? "Me" : message->source;
        CLAY_TEXT(((Clay_String) {
                      .chars = sender_id,
                      .length = strlen(sender_id),
                      .isStaticallyAllocated = message->source == NULL,
                  }),
            CLAY_TEXT_CONFIG({ .textAlignment
                = message->source == NULL ? CLAY_TEXT_ALIGN_RIGHT : CLAY_TEXT_ALIGN_LEFT,
                .letterSpacing = SPACING_CHAT_FONT,
                .textColor = COLOR_CHAT_TEXT,
                .fontId = ID_CHAT_FONT,
                .fontSize = SIZE_CHAT_FONT }));

        CLAY({ .id = CLAY_IDI("ChatMessageBubble", index),
            .layout = { .padding = CLAY_PADDING_ALL(PADDING_CHAT_MESSAGE_TEXT),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { .width = CLAY_SIZING_PERCENT(.33), .height = CLAY_SIZING_FIT() }, 
            },
            .backgroundColor = COLOR_CHAT_MESSAGE_BACKGROUND,
            .cornerRadius = CLAY_CORNER_RADIUS(CORNER_RADIUS_CHAT_MESSAGE) }) {

            CLAY_TEXT(((Clay_String) {
                          .chars = message->content.data,
                          .length = message->content.len,
                          .isStaticallyAllocated = false,
                      }),
                CLAY_TEXT_CONFIG({
                    .textColor = COLOR_CHAT_TEXT,
                    .fontSize = SIZE_CHAT_FONT,
                    .fontId = ID_CHAT_FONT,
                    .letterSpacing = SPACING_CHAT_FONT,
                }));
        }
    }
}

void chat_window(AppModel* model) {
    Connection* active_connection = &model->connections.data[model->connections.selected];

    CLAY({
        .id = CLAY_ID("ChatWindow"),
        .backgroundColor = COLOR_CHAT_BACKGROUND,
        .clip = { 
            .horizontal = false, 
            .vertical = true,
            .childOffset = Clay_GetScrollOffset() 
        },
        .layout = { 
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM, 
            .childGap = GAP_CHAT_MESSAGE_OUTER,
        },
    }) {
        for (size_t i = 0; i < active_connection->messages.len; i++) {
            chat_message(i, &active_connection->messages.data[i]);
        }
    }
}

void connection_tab(size_t* selected, const TabModel* model) {
    // Supports three digit numbers and ": " in between name and index

    Clay_Color background
        = (model->index == *selected) ? COLOR_SIDEBAR_TAB_ACTIVE : COLOR_SIDEBAR_TAB_INACTIVE;

    CLAY({ .id = CLAY_IDI("ConnectionTab", model->index),
        .backgroundColor = Clay_Hovered() ? COLOR_SIDEBAR_TAB_HOVERED : background,
        .cornerRadius = CLAY_CORNER_RADIUS(10),
        .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .padding = CLAY_PADDING_ALL(PADDING_SIDEBAR_TAB),
            .sizing = { .width = CLAY_SIZING_PERCENT(1.0), .height = CLAY_SIZING_FIT() } } }) {

        if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            (*selected) = model->index;
        }

        CLAY_TEXT(((Clay_String) { .chars = model->title,
                      .length = strnlen(model->title, INET_ADDRSTRLEN),
                      .isStaticallyAllocated = false }),
            CLAY_TEXT_CONFIG({
                .textColor = COLOR_SIDEBAR_TAB_TEXT,
                .fontSize = SIZE_TAB_FONT,
                .fontId = ID_TAB_FONT,
                .letterSpacing = SPACING_TAB_FONT,
            }));

        Clay_Color status_background = model->is_active ? COLOR_SIDEBAR_TAB_ACTIVE_CONNECTION
                                                        : COLOR_SIDEBAR_TAB_INACTIVE_CONNECTION;
        CLAY({
            .id = CLAY_IDI("StatusCircle", model->index),
            .cornerRadius = CLAY_CORNER_RADIUS(.5),
            .layout = { .sizing = { .width = CLAY_SIZING_FIXED(SIZE_SIDEBAR_TAB_STATUS),
                            .height = CLAY_SIZING_FIXED(SIZE_SIDEBAR_TAB_STATUS) } },
            .backgroundColor = status_background,
        });
    }
}

void side_bar(AppModel* model) {
    CLAY({ .id = CLAY_ID("SideBar"),
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_PERCENT(0.25), .height = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(PADDING_SIDEBAR_OUTER),
            .childGap = GAP_SIDEBAR_INNER },
        .backgroundColor = COLOR_SIDEBAR_BACKGROUND,

        // Enable Scrolling
        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() } }) {

        CLAY({ .id = CLAY_ID("Title"),
            .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(PADDING_SIDEBAR_TITLE),
                .childGap = GAP_SIDEBAR_TITLE,
                .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
            .backgroundColor = COLOR_SIDEBAR_TITLE_BACKGROUND }) {

            CLAY_TEXT(CLAY_STRING("Pi-Chat"),
                CLAY_TEXT_CONFIG({ .fontSize = SIZE_SIDEBAR_TITLE_FONT,
                    .letterSpacing = SPACING_SIDEBAR_TITLE_FONT,
                    .textColor = COLOR_SIDEBAR_TITLE_TEXT }));
        }

        // Standard C code like loops etc work inside components
        for (size_t i = 0; i < model->tabs.len; i++) {
            connection_tab(&model->connections.selected, &model->tabs.data[i]);
        }
    }
}
