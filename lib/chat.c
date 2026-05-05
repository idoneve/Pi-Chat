#include <chat.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Shutdown flag for signal handling
extern volatile sig_atomic_t running;

int create_socket(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Failed to create socket\n");
        exit(1);
    }
    return server_fd;
}

struct sockaddr_in configure_socket(void) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // Use ipv4
    addr.sin_port = htons(PORT); // Get port
    return addr;
}

SignalResponse is_signal_ready(int max_fd, fd_set* read_fds) {
    if (select(max_fd + 1, read_fds, NULL, NULL, NULL) < 0) {
        if (errno == EINTR) {
            return INTERRUPT; // ctrl+C was pressed
        } else {
            return FD_ERROR;
        }
    }
    return SIGNAL;
}

Connections init_connections(void) {
    return (Connections) {
        .data = malloc(sizeof(Connection) * MAX_CONNECTIONS),
        .len = 0,
        .cap = MAX_CONNECTIONS,
    };
}

void deinit_connections(Connections* connections) {
    free(connections->data);
    connections->data = NULL;
    connections->len = 0;
    connections->cap = 0;
}

void add_connection(Connections* connections, Connection connection) {
    if (connections->len == connections->cap) {
        Connection* old_data = connections->data;

        connections->cap *= 2;
        connections->data = malloc(sizeof(Connection) * connections->cap);
        memcpy(connections->data, old_data, connections->len * sizeof(Connection));
        free(old_data);
    }

    connections->data[connections->len++] = connection;
}

bool reactivate_connection(Connections* connections, Connection incoming) {
    for (size_t i = 0; i < connections->len; i++) {
        Connection* existing = &connections->data[i];

        if (existing->active)
            continue;

        if (strcmp(existing->ip, incoming.ip) == 0) {
            *existing = incoming;

            return true;
        }
    }
    return false;
}

void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = kill_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

void kill_sig(int sig) {
    (void)sig;
    running = 0;
}

// Pack a message with length prefix, returns total bytes to send
static ssize_t pack_message(const ClientMessage* message, char* out_buf, size_t buf_size) {
    if (message->content.len + HEADER_SIZE > buf_size)
        return -1; // msg too big

    char type[HEADER_TYPE_SIZE] = { MESSAGE };
    memcpy(out_buf, type, HEADER_TYPE_SIZE);
    memcpy(out_buf + HEADER_TYPE_SIZE, message->ip, HEADER_ADDR_SIZE);

    uint32_t net_len = htonl((unsigned int)message->content.len);
    memcpy(out_buf + HEADER_SIZE - HEADER_LEN_SIZE, &net_len, HEADER_LEN_SIZE);
    memcpy(out_buf + HEADER_SIZE, message->content.data, message->content.len);

    return (ssize_t)(message->content.len + HEADER_SIZE);
}

// Send a length-prefixed message
ssize_t send_message(int fd, const ClientMessage* message) {
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

// Returns message type if disconnected or invalid or length of message content
static ssize_t unpack_message(int fd, char buffer[HEADER_SIZE + MAX_MSG_LEN]) {
    printf("\t[DEBUG] Beginning unpack\n");
    ssize_t msg_len;

    ssize_t n;

    n = recv(fd, buffer, HEADER_TYPE_SIZE, 0);
    if (n == 0) {
        return DISCONNECT;
    } else if (n < 0) {
        printf("[DEBUG] Invalid type received\n");
        return INVALID;
    }

    printf("[DEBUG] Received regular message\n");

    n = recv(fd, buffer + HEADER_TYPE_SIZE, HEADER_ADDR_SIZE, 0);
    if (n == 0) {
        return DISCONNECT;
    } else if (n < 0) {
        perror("[DEBUG] Invalid address received\n");
        return INVALID;
    }

    if (n < HEADER_ADDR_SIZE) {
        (buffer + HEADER_TYPE_SIZE)[n] = '\0';
        printf("[DEBUG] Invalid address received: %s\n", buffer + HEADER_TYPE_SIZE);
        return INVALID;
    }

    printf("[DEBUG] destination address %s received\n", buffer + HEADER_TYPE_SIZE);

    // Read 4-byte length header
    uint32_t net_len;
    n = recv(fd, &net_len, HEADER_LEN_SIZE, 0);
    if (n == 0) {
        return DISCONNECT;
    } else if (n < 0) {
        return INVALID;
    }

    if (n < HEADER_LEN_SIZE)
        return INVALID; // Incomplete header

    msg_len = ntohl(net_len);
    if (msg_len <= 0 || msg_len >= MAX_MSG_LEN)
        return INVALID;

    printf("[DEBUG] message length %d received\n", msg_len);

    char* msg_data = (buffer + HEADER_SIZE);

    // Read the actual message
    ssize_t total = 0;
    while (total < msg_len) {
        n = recv(fd, msg_data + total, (size_t)(msg_len - total), 0);
        if (n == 0) {
            return DISCONNECT;
        } else if (n < 0) {
            return INVALID;
        }
        total += n;
    }
    msg_data[msg_len] = '\0';

    return msg_len;
}

Message receive_message(int fd) {
    char buffer[MAX_MSG_LEN + HEADER_SIZE];
    ssize_t len;
    switch (len = unpack_message(fd, buffer)) {
    case INVALID:
        return (Message) { .type = INVALID };
    case DISCONNECT:
        return (Message) { .type = DISCONNECT };
    default:
        Message result = { .type = MESSAGE };
        ClientMessage* message = &result.type_data.message;
        message->type = RECEIVE;

        // Copy address into message
        memcpy(message->ip, buffer + HEADER_TYPE_SIZE, HEADER_ADDR_SIZE);
        // Copy message content into message
        message->content.data = malloc((size_t)len);
        memcpy(message->content.data, buffer + HEADER_TYPE_SIZE + HEADER_ADDR_SIZE, (size_t)len);

        return result;
    }
}
