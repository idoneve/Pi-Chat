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

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int fd;
    bool active;
} ServerConnection;

typedef struct {
    List internal;
} ServerConnections;

static ServerConnections init_connections(void) {
    return (ServerConnections) { .internal = init_list(sizeof(ServerConnection), MAX_CONNECTIONS) };
}

static void deinit_connections(ServerConnections* connections) {
    deinit_list(&connections->internal);
}

static void add_connection(ServerConnections* connections, ServerConnection connection) {
    append_list(&connections->internal, &connection);
}

static bool reactivate_connection(ServerConnections* connections, ServerConnection incoming) {
    for (size_t i = 0; i < connections->internal.len; i++) {
        ServerConnection* existing = get_list(connections->internal, i);

        if (existing->active)
            continue;

        if (strcmp(existing->ip, incoming.ip) == 0) {
            *existing = incoming;

            return true;
        }
    }
    return false;
}

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

static int load_connections(int server_fd, ServerConnections* connections, fd_set* read_fds) {

    FD_ZERO(read_fds);

    FD_SET(server_fd, read_fds); // Load server into reader
    int max_fd = server_fd;
    for (size_t i = 0; i < connections->internal.len; ++i) {
        ServerConnection* connection = get_list(connections->internal, i);
        if (!connection->active)
            continue;

        FD_SET(connection->fd, read_fds); // Load clients into reader
        if (connection->fd > max_fd) {
            max_fd = connection->fd;
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
static AcceptError accept_clients(int server_fd, ServerConnections* connections, fd_set* read_fds) {

    if (!FD_ISSET(server_fd, read_fds))
        return NONE;

    int client_fd = accept_client(server_fd);
    if (client_fd < 0) {
        return ACCEPT_ERROR;
    } else {

        ServerConnection c = (ServerConnection) {
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
    const ServerConnection* source, const ServerConnections* connections, ClientMessage* message) {

    if (message->type == SEND) {
        printf("[ERROR] Send message given to router");
        return -1;
    }

    ServerConnection* destination = NULL;
    for (size_t i = 0; i < connections->internal.len; i++) {
        ServerConnection* connection = get_list(connections->internal, i);
        if (strncmp(message->ip, connection->ip, INET_ADDRSTRLEN) == 0) {
            destination = connection;
            break;
        }
    }

    if (destination == NULL) {
        printf("[ERROR] Unkown destination address provided");
        return -1;
    }

    // set destination to source
    memcpy(message->ip, source->ip, sizeof(message->ip));

    // Broadcast message to routed client
    if (send_message(destination->fd, message) < 0) {
        perror("[ERROR] Could not broadcast message\n");
        return -1;
    }

    printf("\t[Server] Broadcasted message from fd %d to fd %d\n", source->fd, destination->fd);

    return 0;
}

static void check_for_messages(ServerConnections* connections, fd_set* read_fds) {
    for (size_t i = 0; i < connections->internal.len; ++i) {
        ServerConnection* connection = get_list(connections->internal, i);
        if (!connection->active)
            continue;

        if (!FD_ISSET(connection->fd, read_fds))
            // ServerConnection has no new messages
            continue;

        printf("\t[Server] Client (fd %d) is sending a signal...\n", connection->fd);

        char buf[HEADER_SIZE + MAX_MSG_LEN];

        Message message = receive_message(connection->fd);
        switch (message.type) {
        case INVALID:
            printf("Error: Malinformed Message Received");
            return;
        case DISCONNECT:
            printf("\t[Server] Client (fd %d) has disconnected\n", connection->fd);
            connection->active = false;
            close(connection->fd);
            return;
        case MESSAGE:
            break;
        }

        ClientMessage* client_message = &message.type_data.message;

        if (route_message(connection, connections, client_message) < 0) {
            printf("[SERVER] failed to route message");
            continue;
        }

        printf("\t[Server] The message %s of size %zd has been broadcasted\n",
                client_message->content.data,
            client_message->content.len);
    }
}

int main(void) {
    ServerConnections connections = init_connections();

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
