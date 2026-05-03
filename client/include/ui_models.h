#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // Includes spacing for 2 digits + : + " " + Readable Address + null terminator
    int index;
    char title[5 + INET_ADDRSTRLEN];
    bool is_active;
} TabModel;
