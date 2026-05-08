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

// Returns a char* to memory with len INET_ADDRSTRLEN or NULL
int get_readable_ip(int fd, char* buf, size_t buf_len) {
    if (buf_len != HEADER_ADDR_SIZE) {
        printf("[Error] Incorrect buffer length or ip address");
        return -1;
    }

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getpeername(fd, (struct sockaddr*)&addr, &len);
    inet_ntop(AF_INET, &addr.sin_addr, buf, (socklen_t)buf_len);

    return 0;
}

// Allocates data if cap > 0
List init_list(size_t data_size, size_t cap) {
    return (List) {
        .data = cap == 0 ? NULL : malloc(data_size * cap),
        .data_size = data_size,
        .cap = cap,
        .len = 0,
    };
}

void deinit_list(List* list) { free(list->data); }

void append_list(List* list, const void* data) {
    if (list->len == list->cap) {
        void* old_data = list->data;

        if (list->cap == 0)
            list->cap = 1;
        else
            list->cap *= 2;

        list->data = malloc(list->cap * list->data_size);

        if (old_data != NULL) {
            memcpy(list->data, old_data, list->data_size * list->len);
            free(old_data);
        }
    }

    // cast void* data to a array of bytes and retreive element
    char* element = ((char*)list->data) + (list->len * list->data_size);
    memcpy(element, data, list->data_size);

    list->len += 1;
}

void* get_list(List list, size_t n) { return ((char*)list.data) + n * list.data_size; }

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

// Pack a message with length prefix, returns total bytes to send
static ssize_t pack_message(
    const ClientMessage* message, char out_buf[MESSAGE_HEADER_SIZE + MAX_MSG_LEN]) {
    printf("\t[DEBUG] Packing %s\n", message->content.data);
    if (message->content.len > MAX_MSG_LEN) {
        printf("\t[DEBUG] message too big %s\n", message->content.data);
        return -1; // msg too big
    }

    char type[HEADER_TYPE_SIZE] = { MESSAGE };
    memcpy(out_buf, type, HEADER_TYPE_SIZE);
    memcpy(out_buf + HEADER_TYPE_SIZE, message->ip, HEADER_ADDR_SIZE);

    uint32_t net_len = htonl((unsigned int)message->content.len);
    memcpy(out_buf + MESSAGE_HEADER_SIZE - HEADER_LEN_SIZE, &net_len, HEADER_LEN_SIZE);
    memcpy(out_buf + MESSAGE_HEADER_SIZE, message->content.data, message->content.len);

    return (ssize_t)(message->content.len + MESSAGE_HEADER_SIZE);
}

static void pack_activity(const ActivityMessage* message, char out_buf[ACTIVITY_SIZE]) {

    out_buf[0] = ACTIVITY; // TYPE
    memcpy(out_buf + HEADER_TYPE_SIZE, message->ip, HEADER_ADDR_SIZE); // ADDR
    memcpy(out_buf + HEADER_TYPE_SIZE + HEADER_ADDR_SIZE, &message->active, sizeof(bool)); // Active
}

// Send a length-prefixed message
ssize_t send_message(int fd, const ClientMessage* message) {
    printf("\t[DEBUG] sending message %s\n", message->content.data);
    char packet[MAX_MSG_LEN + MESSAGE_HEADER_SIZE];
    ssize_t total = pack_message(message, packet);
    if (total < 0)
        return -1;

    ssize_t sent = 0;
    while (sent < total) {
        ssize_t n = send(fd, packet + sent, (size_t)(total - sent), 0);
        if (n <= 0)
            return n;
        sent += n;
    }
    return sent - MESSAGE_HEADER_SIZE; // Payload bytes sent
}

bool send_activity(int fd, const ActivityMessage* message) {
    printf("\t[DEBUG] sending activity ip: %s to fd: %d\n", message->ip, fd);
    char packet[ACTIVITY_SIZE];
    pack_activity(message, packet);

    size_t sent = 0;
    while (sent < ACTIVITY_SIZE) {
        ssize_t n = send(fd, packet + sent, ACTIVITY_SIZE - sent, 0);
        if (n <= 0)
            return false;
        sent += (size_t)n;
    }

    return true; // Payload bytes sent
}

static ssize_t recv_all(int fd, char* buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        ssize_t n = recv(fd, buf + total, len - total, 0);

        if (n == 0)
            return 0; // disconnect

        if (n < 0)
            return -1; // error

        total += (size_t)n;
    }

    return (ssize_t)total;
}

// Returns message type if disconnected or invalid or length of message content
static ssize_t unpack_message(int fd, char buffer[MESSAGE_HEADER_SIZE + MAX_MSG_LEN]) {
    printf("\t[DEBUG] Beginning unpack\n");
    ssize_t msg_len;

    ssize_t n;

    n = recv_all(fd, buffer, HEADER_TYPE_SIZE);
    if (n == DISCONNECT) {
        return DISCONNECT;
    } else if (n < 0) {
        printf("[DEBUG] Invalid type received\n");
        return INVALID;
    }

    printf("[DEBUG] Received valid message\n");

    n = recv_all(fd, buffer + HEADER_TYPE_SIZE, HEADER_ADDR_SIZE);
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

    // ACTIVITY MESSAGE RECEIVED
    if (buffer[0] == ACTIVITY) {
        return ACTIVITY;
    }

    printf("[DEBUG] destination address %s received\n", buffer + HEADER_TYPE_SIZE);

    // Read 4-byte length header
    uint32_t net_len;
    n = recv_all(fd, (char*)&net_len, HEADER_LEN_SIZE);
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

    char* msg_data = (buffer + MESSAGE_HEADER_SIZE);

    n = recv_all(fd, msg_data, MAX_MSG_LEN);
    if (n == 0) {
        return DISCONNECT;
    } else if (n < 0) {
        return INVALID;
    }
    msg_data[msg_len] = '\0';

    printf("[DEBUG] message data %s received\n", msg_data);

    return msg_len;
}

Message receive_message(int fd) {
    char buffer[MAX_MSG_LEN + MESSAGE_HEADER_SIZE];
    ssize_t len;
    switch (len = unpack_message(fd, buffer)) {
    case INVALID:
        return (Message) { .type = INVALID };
    case DISCONNECT:
        return (Message) { .type = DISCONNECT };
    default:
        break;
    }

    Message result = {
        .type = buffer[0],
    };

    switch (result.type) {
    case ACTIVITY:
        printf("\t[DEBUG] Unpacked Activity Message\n");
        ActivityMessage activity = {
            .active = *((bool*)(buffer + (ACTIVITY_SIZE - HEADER_STATUS_SIZE))),
        };
        memcpy(activity.ip, buffer + HEADER_TYPE_SIZE, HEADER_ADDR_SIZE);

        Message m = {
            .type = ACTIVITY,
            .type_data = { .activity = activity, },
        };

        return m;
    case MESSAGE:
        printf("\t[DEBUG] Unpacked regular Message\n");
        Message regular_message = { .type = MESSAGE };
        ClientMessage* message = &regular_message.type_data.message;
        message->type = RECEIVE;

        // Copy address into message
        memcpy(message->ip, buffer + HEADER_TYPE_SIZE, HEADER_ADDR_SIZE);
        // Copy message content into message
        message->content.len = (size_t)len;
        memcpy(message->content.data, buffer + MESSAGE_HEADER_SIZE, (size_t)len);

        return regular_message;
    default:
        printf("Unreachable Branch Reached");
        exit(1);
    }
}
