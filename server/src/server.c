#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.h"

volatile sig_atomic_t running = 1;

static void bind_socket(int server_fd) {
    printf("\t[Server] Binding socket...\n");
    struct sockaddr_in addr = configure_socket();
    addr.sin_addr.s_addr = INADDR_ANY; // Listens on all network interfaces

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Reuse address if needed
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] Failed to bind socket\n");
        exit(1);
    }
    printf("\t[Server] Socket binded\n");
}

static void start_listening(int server_fd) {
    printf("\t[Server] Starting to listen to socket...\n");
    if (listen(server_fd, MAX_CONNECTIONS + 1) < 0) {
        perror("[ERROR] Failed to listen to socket\n");
        exit(1);
    }
    printf("\t[Server] Listening to socket\n");
}

static int start_server(void) {
    printf("\t[Server] Creating socket...\n");
    int server_fd = create_socket();
    printf("\t[Server] Socket created\n");

    bind_socket(server_fd);
    start_listening(server_fd);

    return server_fd;
}

static int close_server(int server_fd) {
    printf("[Server] Shutting down server...\n");
    if (close(server_fd) < 0) {
        perror("[ERROR] The server could not shutdown\n");
        return 1;
    }
    printf("[Server] The server has shut down\n");
    return 0;
}

static int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        return -1;
    } else {
        return client_fd;
    }
}

static int load_connections(int server_fd, Connections* connections, fd_set* read_fds) {

    FD_ZERO(read_fds);

    FD_SET(server_fd, read_fds); // Load server into reader
    int max_fd = server_fd;
    for (size_t i = 0; i < connections->len; ++i) {
        if (!connections->data[i].active)
            continue;

        FD_SET(connections->data[i].fd, read_fds); // Load clients into reader
        if (connections->data[i].fd > max_fd) {
            max_fd = connections->data[i].fd;
        }
    }
    return max_fd;
}

// Returns a char* to memory with len INET_ADDRSTRLEN or NULL
static int get_readable_ip(int fd, char* buf, size_t buf_len) {
    if (buf_len != INET_ADDRSTRLEN) {
        printf("[Error] Incorrect buffer length or ip address");
        return -1;
    }

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getpeername(fd, (struct sockaddr*)&addr, &len);
    inet_ntop(AF_INET, &addr.sin_addr, buf, (socklen_t)buf_len);

    return 0;
}

typedef enum {
    ACCEPT_ERROR = -2,
    IP_ERROR = -1,
    NONE = 0,
} AcceptError;

// Returns size of active conenctions or an AcceptError
static AcceptError accept_clients(int server_fd, Connections* connections, fd_set* read_fds) {

    if (!FD_ISSET(server_fd, read_fds))
        return NONE;

    int client_fd = accept_client(server_fd);
    if (client_fd < 0) {
        return ACCEPT_ERROR;
    } else {

        Connection c = (Connection) {
            .fd = client_fd,
            .active = true,
        };

        if (get_readable_ip(client_fd, c.ip, sizeof(c.ip)) < 0) {
            perror("[ERROR] client ip could not be determined");
            return IP_ERROR;
        }

        if (!reactivate_connection(connections, c)) {
            add_connection(connections, c);
        }

        printf("\t[Server] Client connection accepted (fd %d: ip %s)\n", client_fd, c.ip);
        return NONE;
    }
}

static int route_message(
    const Connection* source, const Connections* connections, ClientMessage* message) {

    if (message->type == SEND) {
        printf("[ERROR] Send message given to router");
        return -1;
    }

    // TODO ROUTE MESSAGES
    ssize_t destination = -1;
    for (size_t i = 0; i < connections->len; i++) {
        if (strncmp(message->ip, connections->data[i].ip, INET_ADDRSTRLEN) == 0) {
            destination = (ssize_t)i;
            break;
        }
    }

    if (destination < 0) {
        printf("[ERROR] Unkown destination address provided");
        return (int)destination;
    }

    // set destination to source
    memcpy(message->ip, connections->data[destination].ip, sizeof(message->ip));

    // Broadcast message to routed client
    if (send_message(connections->data[destination].fd, message) < 0) {
        perror("[ERROR] Could not broadcast message\n");
    }

    printf("\t[Server] Broadcasted message from fd %d to fd %d\n", source->fd,
        connections->data[destination].fd);

    return 0;
}

static void check_for_messages(Connections* connections, fd_set* read_fds) {
    for (size_t i = 0; i < connections->len; ++i) {
        if (!connections->data[i].active)
            continue;

        if (!FD_ISSET(connections->data[i].fd, read_fds))
            // Connection has no new messages
            continue;

        printf("\t[Server] Client (fd %d) is sending a signal...\n", connections->data[i].fd);

        char buf[HEADER_SIZE + MAX_MSG_LEN];

        Message message = receive_message(connections->data[i].fd);
        switch (message.type) {
        case INVALID:
            printf("Error: Malinformed Message Received");
            return;
        case DISCONNECT:
            printf("\t[Server] Client (fd %d) has disconnected\n", connections->data[i].fd);
            connections->data[i].active = false;
            close(connections->data[i].fd);
            return;
        case MESSAGE:
            break;
        }

        ClientMessage* client_message = &message.type_data.message;

        if (route_message(&connections->data[i], connections, client_message) < 0) {
            printf("[SERVER] failed to route message");
            continue;
        }

        printf("\t[Server] The message %s of size %zd has been broadcasted\n",
                client_message->content.data,
            client_message->content.len);
    }
}

int main(void) {
    Connections connections = init_connections();

    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n\n");

    // Main loop
    fd_set read_fds;
    while (running) {
        // Create fd reader
        int max_fd = load_connections(server_fd, &connections, &read_fds);

        // Only proceed if a watched fd is ready to read (no blocks)
        printf("[Server] Waiting for signal...\n");
        SignalResponse signal_respnse;
        if ((signal_respnse = is_signal_ready(max_fd, &read_fds)) != SIGNAL) {
            if (signal_respnse == INTERRUPT) {
                continue;
            }
            if (signal_respnse == FD_ERROR) {
                perror("[ERROR] Failed to select fd with data\n");
                break;
            }
        }
        printf("\t[Server] A signal is being processed...\n");

        switch (accept_clients(server_fd, &connections, &read_fds)) {
        case ACCEPT_ERROR:
            perror("[ERROR] Could not accept client\n");
            continue;
        case IP_ERROR:
            continue;
        case NONE:
            break;
        }

        check_for_messages(&connections, &read_fds);
    }

    deinit_connections(&connections);
    return close_server(server_fd);
}
