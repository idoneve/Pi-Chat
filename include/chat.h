#ifndef CHAT_H
#define CHAT_H

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
#include <errno.h>

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
        char* data;
        size_t len;
    } content;

    enum { RECEIVE, SEND } type;

    struct {
        char receive_source[INET_ADDRSTRLEN];
        char send_dest[INET_ADDRSTRLEN];
    } type_data;

    // Readable Ip Source
} ClientMessage;

typedef enum { ACTIVITY, MESSAGE, INVALID, DISCONNECT } MessageType;

typedef struct {
    MessageType type;

    union {
        ClientMessage message;
    } type_data;
} Message;

// Shutdown flag for signal handling
extern volatile sig_atomic_t running;

static inline void kill_sig(int sig) {
    (void)sig;
    running = 0;
}

static inline void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = kill_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

// Pack a message with length prefix, returns total bytes to send
static inline ssize_t pack_message(
    const ClientMessage* send_message, char* out_buf, size_t buf_size) {
    if (send_message->content.len + HEADER_SIZE > buf_size)
        return -1; // msg too big

    char type[HEADER_TYPE_SIZE] = { SEND };
    memcpy(out_buf, type, HEADER_TYPE_SIZE);
    memcpy(out_buf + HEADER_TYPE_SIZE, send_message->type_data.send_dest, HEADER_ADDR_SIZE);

    uint32_t net_len = htonl((unsigned int)send_message->content.len);
    memcpy(out_buf + HEADER_SIZE - HEADER_LEN_SIZE, &net_len, HEADER_LEN_SIZE);
    memcpy(out_buf + HEADER_SIZE, send_message->content.data, send_message->content.len);

    return (ssize_t)(send_message->content.len + HEADER_SIZE);
}

static inline Message unpack_message(int fd) {
    uint32_t net_len;
    size_t msg_len;

    Message result;

    ssize_t n;

    char type[HEADER_TYPE_SIZE];
    n = recv(fd, type, HEADER_TYPE_SIZE, 0);
    if (n == 0) {
        return (Message) { .type = DISCONNECT };
    } else if (n < 0) {
        return (Message) { .type = INVALID };
    }

    result.type = (MessageType)*type;

    if (result.type == ACTIVITY) {
        return result;
    }

    char dest_buf[HEADER_ADDR_SIZE];
    n = recv(fd, dest_buf, HEADER_ADDR_SIZE, 0);
    if (n == 0) {
        return (Message) { .type = DISCONNECT };
    } else if (n < 0) {
        return (Message) { .type = INVALID };
    }

    if (n < HEADER_ADDR_SIZE) {
        return (Message) { .type = INVALID };
    }

    // Copy address into receive ClientMessage
    result.type_data.message = (ClientMessage) { .type = RECEIVE };
    memcpy(result.type_data.message.type_data.receive_source, dest_buf, HEADER_ADDR_SIZE);

    // Read 4-byte length header
    n = recv(fd, &net_len, HEADER_LEN_SIZE, 0);
    if (n == 0) {
        return (Message) { .type = DISCONNECT };
    } else if (n < 0) {
        return (Message) { .type = INVALID };
    }

    if (n < HEADER_LEN_SIZE)
        return (Message) { .type = INVALID }; // Incomplete header

    msg_len = ntohl(net_len);
    if (msg_len <= 0 || msg_len >= MAX_MSG_LEN)
        return (Message) { .type = INVALID };

    result.type_data.message.content.len = msg_len;
    char** msg_data = &result.type_data.message.content.data;
    *msg_data = malloc(msg_len + 1);

    // Read the actual message
    ssize_t total = 0;
    while (total < (ssize_t)msg_len) {
        n = recv(fd, *msg_data + total, msg_len - (size_t)total, 0);
        if (n == 0) {
            free(msg_data);
            return (Message) { .type = DISCONNECT };
        } else if (n < 0) {
            free(msg_data);
            return (Message) { .type = INVALID };
        }
        total += n;
    }
    (*msg_data)[msg_len] = '\0';
    

    return result;
}

// Send a length-prefixed message
static inline ssize_t send_message(int fd, const ClientMessage* message) {
    if (message->type != SEND){
        perror("[ERROR] Invalid message type was queued for send");
        return -1;
    }

    char packet[MAX_MSG_LEN + HEADER_SIZE];
    ssize_t total = pack_message(message, packet, sizeof(packet));
    if (total < 0)
        return -1;

    ssize_t sent = 0;
    while (sent < total) {
        ssize_t n = send(fd, packet + sent, (size_t)(total - sent), 0);
        if (n <= 0)
            return n;
        sent += n;
    }
    return sent - HEADER_SIZE; // Payload bytes sent
}

static int create_socket(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Failed to create socket\n");
        exit(1);
    }
    return server_fd;
}

static struct sockaddr_in configure_socket() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // Use ipv4
    addr.sin_port = htons(PORT); // Get port
    return addr;
}

typedef enum {
    FD_ERROR,
    SIGNAL,
    INTERUPT,
} SignalResponse;

static SignalResponse is_signal_ready(int max_fd, fd_set* read_fds) {
    if (select(max_fd + 1, read_fds, NULL, NULL, NULL) < 0) {
        if (errno == EINTR) {
            return INTERUPT; // ctrl+C was pressed
        } else {
            return FD_ERROR;
        }
    }
    return SIGNAL;
}

#endif
