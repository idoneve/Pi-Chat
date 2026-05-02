#pragma once

#include "chat.h"

volatile sig_atomic_t running = 1;

static void kill_sig(int sig) {
    (void)sig;
    running = 0;
}

void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = kill_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}
