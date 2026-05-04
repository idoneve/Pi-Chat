#include <netinet/in.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.h"

volatile sig_atomic_t running = 1;


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
    int server_fd = create_socket();

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

static int load_connections(
    int server_fd, int* connections, int active_connections, fd_set* read_fds, fd_set* write_fds) {

    FD_ZERO(write_fds);
    FD_ZERO(read_fds);

    FD_SET(server_fd, write_fds);
    FD_SET(server_fd, read_fds); // Load server into reader
    int max_fd = server_fd;
    for (int i = 0; i < active_connections; ++i) {
        FD_SET(connections[i], write_fds);
        FD_SET(connections[i], read_fds); // Load clients into reader
        if (connections[i] > max_fd) {
            max_fd = connections[i];
        }
    }
    return max_fd;
}

typedef enum {
    FULL_ERROR = -1,
    ACCEPT_ERROR = -2,
    FD_UNSET_ERROR = -3,
} AcceptError;

static int accept_clients(
    int server_fd, int* connections, int active_connections, fd_set* read_fds) {

    if (!FD_ISSET(server_fd, read_fds))
        return FD_UNSET_ERROR;

    int client_fd = accept_client(server_fd);
    if (client_fd < 0) {
        return ACCEPT_ERROR;
    } else if (active_connections >= MAX_CONNECTIONS) {
        close(client_fd);
        return FULL_ERROR;
    } else {
        connections[active_connections++] = client_fd;
        printf("\t[Server] Client connection accepted (fd %d)\n", client_fd);
        return active_connections;
    }
}

static void route_message(
    int current_connection, int* connections, int connections_len, char* msg, size_t msg_len) {

    // TODO ROUTE MESSAGES

    // Broadcast message to everyone else
    for (int j = 0; j < connections_len; ++j) {
        if (current_connection != j) {
            if (send(connections[j], msg, (size_t)msg_len, 0) < 0) {
                perror("[ERROR] Could not broadcast message\n");
                // TODO send inactive response to client
                continue;
            }
            printf("\t[Server] Broadcasted message from fd %d to fd %d\n",
                connections[current_connection], connections[j]);
        }
    }
}

static void check_for_messages(int* connections, int active_connections, fd_set* read_fds) {
    for (int i = 0; i < active_connections; ++i) {

        if (FD_ISSET(connections[i], read_fds))
            continue;

        printf("\t[Server] Client (fd %d) is sending a signal...\n", connections[i]);

        char buf[HEADER_SIZE + MAX_MSG_LEN];

        ssize_t message_size = recv(connections[i], buf, sizeof(buf), 0);

        if (message_size <= 0) {
            close(connections[i]);
            printf("\t[Server] Client (fd %d) disconnected\n", connections[i]);

            // swap-remove dead connection and decrement active_connections
            connections[i] = connections[--active_connections];
        } else {
            route_message(i, connections, active_connections, buf, (size_t)message_size);
        }
        printf("\t[Server] The message of size %zd has been broadcasted\n", message_size);
    }
}


int main(void) {
    int active_connections = 0;
    int connections[MAX_CONNECTIONS];

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

        int accept_response;
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
        active_connections = accept_response;

        check_for_messages(connections, active_connections, &read_fds);
    }

    return close_server(server_fd);
}
