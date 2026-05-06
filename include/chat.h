#ifndef CHAT_H
#define CHAT_H

#include "stdbool.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_MSG_LEN 4096
#define MAX_USERNAME 32
#define BACKLOG 5
#define MAX_CONNECTIONS 2

// Message framing: 4-byte length prefix, then payload
#define HEADER_TYPE_SIZE 1
#define HEADER_ADDR_SIZE INET_ADDRSTRLEN
#define HEADER_LEN_SIZE 4
#define HEADER_SIZE HEADER_LEN_SIZE + HEADER_ADDR_SIZE + HEADER_TYPE_SIZE

// Exit codes
#define EXIT_SOCKET 1
#define EXIT_BIND 2
#define EXIT_LISTEN 3
#define EXIT_ACCEPT 4
#define EXIT_CONNECT 5

// Represents a message
typedef struct {
    // Message data
    struct {
        char data[MAX_MSG_LEN];
        size_t len;
    } content;

    enum { RECEIVE, SEND } type;

    // Holds destination when type is send, holds source when type is receive
    char ip[INET_ADDRSTRLEN];
} ClientMessage;

typedef enum { MESSAGE = 1, INVALID = -1, DISCONNECT = 0} MessageType;

typedef struct {
    MessageType type;

    union {
        ClientMessage message;
    } type_data;
} Message;


typedef struct {
    void* data;
    size_t data_size;
    size_t len;
    size_t cap;
} List;

List init_list(size_t data_size, size_t cap);

void deinit_list(List*);

// Copies data into list by value
void append_list(List*, const void* data);
void* get_list(List, size_t n);

typedef enum {
    FD_ERROR,
    SIGNAL,
    INTERRUPT,
} SignalResponse;

int create_socket(void);

struct sockaddr_in configure_socket(void);

// Send a HEADER prefixed message
ssize_t send_message(int fd, const ClientMessage* message);

// Receive a message
Message receive_message(int fd);


// Shutdown flag for signal handling
extern volatile sig_atomic_t running;

void setup_signal_handler(void);

SignalResponse is_signal_ready(int max_fd, fd_set* read_fds);

#endif
