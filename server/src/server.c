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
} Connection;

static void bind_socket(int server_fd) {
    printf("\t[Server] Binding socket...\n");
    struct sockaddr_in addr = configure_socket();

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

static int load_connections(int server_fd, Connection* connections, size_t active_connections,
    fd_set* read_fds, fd_set* write_fds) {

    FD_ZERO(write_fds);
    FD_ZERO(read_fds);

    FD_SET(server_fd, write_fds);
    FD_SET(server_fd, read_fds); // Load server into reader
    int max_fd = server_fd;
    for (size_t i = 0; i < active_connections; ++i) {
        FD_SET(connections[i].fd, write_fds);
        FD_SET(connections[i].fd, read_fds); // Load clients into reader
        if (connections[i].fd > max_fd) {
            max_fd = connections[i].fd;
        }
    }
    return max_fd;
}

typedef enum {
    FULL_ERROR = -1,
    ACCEPT_ERROR = -2,
    FD_UNSET_ERROR = -3,
} AcceptError;

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

static ssize_t accept_clients(
    int server_fd, Connection* connections, size_t active_connections, fd_set* read_fds) {

    if (!FD_ISSET(server_fd, read_fds))
        return FD_UNSET_ERROR;

    int client_fd = accept_client(server_fd);
    if (client_fd < 0) {
        return ACCEPT_ERROR;
    } else if (active_connections >= MAX_CONNECTIONS) {
        close(client_fd);
        return FULL_ERROR;
    } else {
        char ip[INET_ADDRSTRLEN];
        get_readable_ip(client_fd, ip, sizeof(ip));

        connections[active_connections] = (Connection) {
            .fd = client_fd,
        };
        memcpy(connections[active_connections].ip, ip, INET_ADDRSTRLEN);

        active_connections += 1;
        printf("\t[Server] Client connection accepted (fd %d)\n", client_fd);
        return (ssize_t)active_connections;
    }
}

static int route_message(size_t current_connection, Connection* connections, size_t connections_len,
    const ClientMessage* message) {

    // TODO ROUTE MESSAGES
    int destination_fd = -1;
    for (size_t i = 0; i < connections_len; i++) {
        if (strncmp(message->type_data.send_dest, connections[i].ip, INET_ADDRSTRLEN) == 0) {
            destination_fd = connections[i].fd;
        }
    }

    if (destination_fd < 0) {
        printf("[ERROR] Unkown destination address provided");
        return destination_fd;
    }

    // Broadcast message to everyone else
    for (size_t j = 0; j < connections_len; ++j) {
        if (current_connection != j) {
            if (send(connections[j].fd, message->content.data, message->content.len, 0) < 0) {
                perror("[ERROR] Could not broadcast message\n");
                // TODO send inactive response to client
                continue;
            }
            printf("\t[Server] Broadcasted message from fd %d to fd %d\n",
                connections[current_connection], connections[j]);
        }
    }

    return 0;
}

static void check_for_messages(
    Connection* connections, size_t active_connections, fd_set* read_fds) {
    for (size_t i = 0; i < active_connections; ++i) {

        if (FD_ISSET(connections[i].fd, read_fds))
            continue;

        printf("\t[Server] Client (fd %d) is sending a signal...\n", connections[i]);

        char buf[HEADER_SIZE + MAX_MSG_LEN];

        Message message = unpack_message(connections[i].fd);
        switch (message.type) {
        case INVALID:
            printf("Error: Malinformed Message Received");
            return;
        case ACTIVITY:
            // MARK CONNECTION AS ACTIVE
            return;
        case DISCONNECT:
            printf("\t[Server] Client (fd %d) has disconnected\n", connections[i]);
            connections[i] = connections[--active_connections];
            return;
        case MESSAGE:
            break;
        }

        ClientMessage* client_message = &message.type_data.message;
        if (client_message->type != SEND) {
            printf("[SERVER] unexpected message type received");
            return;
        }

        route_message(i, connections, active_connections, client_message);
        printf("\t[Server] The message of size %zd has been broadcasted\n",
            client_message->content.len);
    }
}

int main(void) {
    size_t active_connections = 0;
    Connection connections[MAX_CONNECTIONS];

    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n\n");

    // Main loop
    fd_set read_fds;
    fd_set write_fds;
    while (running) {
        // Create fd reader
        int max_fd
            = load_connections(server_fd, connections, active_connections, &read_fds, &write_fds);

        // Only proceed if a watched fd is ready to read (no blocks)
        printf("[Server] Waiting for signal...\n");
        SignalResponse signal_respnse;
        if ((signal_respnse = is_signal_ready(max_fd, &read_fds)) != SIGNAL) {
            if (signal_respnse == INTERUPT) {
                continue;
            }
            if (signal_respnse == FD_ERROR) {
                perror("[ERROR] Failed to select fd with data\n");
                break;
            }
        }
        printf("\t[Server] A signal is being processed...\n");

        ssize_t accept_response;
        if ((accept_response
                = accept_clients(server_fd, connections, active_connections, &read_fds))
            < 0) {
            switch (accept_response) {
            case ACCEPT_ERROR:
                perror("[ERROR] Could not accept client\n");
                break;
            case FULL_ERROR:
                printf("\t[Server] Server is full\n");
                break;

            default:
                break;
            }
        }
        active_connections = (size_t)accept_response;

        check_for_messages(connections, active_connections, &read_fds);
    }

    return close_server(server_fd);
}
