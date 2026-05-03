#ifndef CHAT_H
#define CHAT_H

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

#define PORT            8080
#define MAX_MSG_LEN     4096
#define MAX_USERNAME    32
#define BACKLOG         5
#define MAX_CONNECTIONS 2

// Message framing: 4-byte length prefix, then payload
#define HEADER_SIZE     4

// Exit codes
#define EXIT_SOCKET     1
#define EXIT_BIND       2
#define EXIT_LISTEN     3
#define EXIT_ACCEPT     4
#define EXIT_CONNECT    5

// Shutdown flag for signal handling
extern volatile sig_atomic_t running;

void setup_signal_handler(void);

// Pack a message with length prefix, returns total bytes to send
static inline ssize_t pack_message(const char *msg, char *out_buf, size_t buf_size) {
    size_t msg_len = strlen(msg);
    if (msg_len + HEADER_SIZE > buf_size) return -1; // msg too big

    uint32_t net_len = htonl((unsigned int)msg_len);
    memcpy(out_buf, &net_len, HEADER_SIZE);
    memcpy(out_buf + HEADER_SIZE, msg, msg_len);

    return (ssize_t)(msg_len + HEADER_SIZE);
}

// Unpack a length-prefixed message. Returns message length or -1 on error
static inline ssize_t unpack_message(int fd, char *out_msg, size_t max_len) {
    uint32_t net_len;
    size_t msg_len;

    // Read 4-byte length header
    ssize_t n = recv(fd, &net_len, HEADER_SIZE, 0);
    if (n <= 0) return n;  // 0 = disconnect, -1 = error
    if (n < HEADER_SIZE) return -1;  // Incomplete header

    msg_len = ntohl(net_len);
    if (msg_len <= 0 || msg_len >= max_len) return -1;

    // Read the actual message
    ssize_t total = 0;
    while (total < (ssize_t)msg_len) {
        n = recv(fd, out_msg + total, msg_len - (size_t)total, 0);
        if (n <= 0) return n;
        total += n;
    }
    out_msg[msg_len] = '\0';

    return (ssize_t)msg_len;
}

// Send a length-prefixed message
static inline ssize_t send_message(int fd, const char *msg) {
    char packet[MAX_MSG_LEN + HEADER_SIZE];
    ssize_t total = pack_message(msg, packet, sizeof(packet));
    if (total < 0) return -1;

    ssize_t sent = 0;
    while (sent < total) {
        ssize_t n = send(fd, packet + sent, (size_t)(total - sent), 0);
        if (n <= 0) return n;
        sent += n;
    }
    return sent - HEADER_SIZE;  // Payload bytes sent
}

#endif
