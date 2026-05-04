#include "app.h"
#include "raylib.h"
#include "style.h"
#include "ui_models.h"
#include <chat.h>
#include <arpa/inet.h>
#include <components.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static void chat_message(size_t index, const ClientMessage* message) {
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

static size_t append_to_buffer(char new_char, char* data, size_t len, size_t cap) {
    if (len == cap)
        return len;

    data[len++] = new_char;
    return len;
}

static size_t remove_from_buffer(char* data, size_t len, size_t cap) {
    if (len == 0)
        return 0;

    if (len < cap) {
        data[len] = '\0';
    }

    return --len;
}

static size_t empty_buffer(char* data, size_t len) {
    memset(data, '\0', len);
    return 0;
}

static void handle_text_input(Connection* selected) {
    char* user_input = selected->user_input.data;
    size_t* input_len = &selected->user_input.len;
    size_t* cursor = &selected->user_input.cursor;

    const size_t input_cap = sizeof(selected->user_input.data);
    int key;
    if ((key = GetCharPressed()) != 0) {

        *input_len = append_to_buffer(key, user_input, *input_len, input_cap);

    } else if ((key = GetKeyPressed()) != 0) {
        switch (key) {
        case KEY_BACKSPACE:
            // TODO handle cursor postion
            *input_len = remove_from_buffer(user_input, *input_len, input_cap);
            break;

        case KEY_ENTER:
            // TODO - send user data
            *input_len = empty_buffer(user_input, *input_len);
            break;

        case KEY_LEFT:
            *cursor -= 1;
            break;

        case KEY_RIGHT:
            *cursor -= 1;
            break;

        case KEY_UP:
            *cursor = 0;
            break;

        case KEY_DOWN:
            *cursor = *input_len;
            break;

        default:
            break;
        }
    }
    return;
}

static void message_entry(AppModel* model) {

    CLAY({
        .id = CLAY_ID("UserInput"),
        .layout = { 
            .padding = CLAY_PADDING_ALL(PADDING_CHAT_USER_INPUT),
            .sizing = { 
                .width = CLAY_SIZING_GROW(), 
                .height = CLAY_SIZING_FIT(50,200) 
            },
        },
        .border = {.width = CLAY_BORDER_OUTSIDE(5), .color = COLOR_LIGHT},
        .clip = {.vertical = true, .horizontal = false, .childOffset = Clay_GetScrollOffset()}
    }) {
        if (model->connections.len != 0) {
            Connection* selected = &model->connections.data[model->connections.selected];

            handle_text_input(selected);

            // TODO handle cursor rendering
            CLAY_TEXT(((Clay_String) {
                          .chars = selected->user_input.data,
                          .length = selected->user_input.len,
                          .isStaticallyAllocated = false,
                      }),
                CLAY_TEXT_CONFIG({
                    .textAlignment = CLAY_TEXT_ALIGN_LEFT,
                    .textColor = COLOR_CHAT_USER_INPUT_TEXT,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .fontId = ID_CHAT_USER_INPUT_FONT,
                    .fontSize = SIZE_CHAT_USER_INPUT_FONT,
                    .letterSpacing = SPACING_CHAT_USER_INPUT_FONT,
                }));
        }
    }
}

static void submit_button(AppModel* model) {
    CLAY({ .id = CLAY_ID("SubmitButtonContainer"),
        .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .width = CLAY_SIZING_FIT(), .height = CLAY_SIZING_GROW() } } }) {
        CLAY({
        .id = CLAY_ID("SubmitButton"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIT(),
                .height = CLAY_SIZING_FIT(),
            },
            .padding = CLAY_PADDING_ALL(PADDING_CHAT_SUBMIT_BUTTON),
            .childAlignment  = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        },
        .backgroundColor = Clay_Hovered() ? COLOR_CHAT_SUBMIT_BUTTON_HOVERED: COLOR_CHAT_SUBMIT_BUTTON_BACKGROUND,
        .cornerRadius = CLAY_CORNER_RADIUS(CORNER_RADIUS_CHAT_SUBMIT_BUTTON),
    }){

            if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                && model->connections.len != 0) {
                Connection* selected = &model->connections.data[model->connections.selected];

                // TODO send message
                selected->user_input.len = 0;
            }

            CLAY_TEXT(CLAY_STRING("Send"),
                CLAY_TEXT_CONFIG({
                    .textColor = COLOR_CHAT_SUBMIT_BUTTON,
                    .fontSize = SIZE_CHAT_SUBMIT_BUTTON_FONT,
                    .fontId = ID_CHAT_SUBMIT_BUTTON_FONT,
                    .letterSpacing = SPACING_CHAT_SUBMIT_BUTTON_FONT,
                }));
        }
    }
}

void chat_window(AppModel* model) {
    CLAY({
        .id = CLAY_ID("ChatWindow"),
        .backgroundColor = COLOR_CHAT_BACKGROUND,
        .layout = { 
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM, 
        }, 
    }) {
        CLAY({ .id = CLAY_ID("MessageWindow"),
            .clip
            = { .horizontal = false, .vertical = true, .childOffset = Clay_GetScrollOffset() },
            .layout = { .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0) },
                .childGap = GAP_CHAT_MESSAGE_OUTER,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = { .y = CLAY_ALIGN_Y_TOP } } }) {

            if (model->connections.len != 0) {
                Connection* active_connection
                    = &model->connections.data[model->connections.selected];

                for (size_t i = 0; i < active_connection->messages.len; i++) {
                    chat_message(i, &active_connection->messages.data[i]);
                }
            }
        }

        // TODO fix Alignment
        CLAY({ .id = CLAY_ID("BottomChatBar"),
            .layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childGap = GAP_CHAT_BOTTOM_BAR,
                .padding = CLAY_PADDING_ALL(PADDING_CHAT_BOTTOM_BAR),
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT() } },
            .backgroundColor = COLOR_WHITE }) {

            message_entry(model);
            submit_button(model);
        }
    }
}

static void connection_tab(size_t* selected, const TabModel* model) {
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
