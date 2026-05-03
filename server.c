#include "signal.h"

int start_server(void);
int accept_client(int);

int start_server(void) {
    // Create socket
    printf("\t[Server] Creating socket...\n");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] Failed to create socket\n");
        exit(1);
    }
    printf("\t[Server] Socket created\n");

    // Bind port to socket
    printf("\t[Server] Binding socket...\n");
    int opt = 1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // Use IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // Listens on all network interfaces
    addr.sin_port = htons(PORT); // Get port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Reuse address if needed
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] Failed to bind socket\n");
        exit(1);
    }
    printf("\t[Server] Socket binded\n");

    // Listen to socket
    printf("\t[Server] Starting to listen to socket...\n");
    if (listen(server_fd, MAX_CONNECTIONS + 1) < 0) {
        perror("[ERROR] Failed to listen to socket\n");
        exit(1);
    }
    printf("\t[Server] Listening to socket\n");

    return server_fd;
}

int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        return -1;
    } else {
        return client_fd;
    }
}

int main(void) {
    int active_connections = 0;
    int connections[MAX_CONNECTIONS];
    fd_set read_fds;

    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n\n");

    // Main loop
    while (running) {
        // Create fd reader
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds); // Load server into reader
        int max_fd = server_fd;
        for (int i = 0; i < active_connections; ++i) {
            FD_SET(connections[i], &read_fds);  // Load clients into reader
            if (connections[i] > max_fd) {
                max_fd = connections[i];
            }
        }

        // Only proceed if a watched fd is ready to read (no blocks)
        printf("[Server] Waiting for signal...\n");
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue; // ctrl+C was pressed
            } else {
                perror("[ERROR] Failed to select fd with data\n");
                break;
            }
        }
        printf("\t[Server] A signal is being processed...\n");

        // Accept clients
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept_client(server_fd);
            if (client_fd < 0) {
                perror("[ERROR] Could not accept client\n");
            } else if (active_connections >= MAX_CONNECTIONS) {
                printf("\t[Server] Server is full\n");
                close(client_fd); // Reject client if full
            } else {
                connections[active_connections++] = client_fd;
                printf("\t[Server] Client connection accepted (fd %d)\n", client_fd);
            }
        }

        // Check clients for messages
        for (int i = 0; i < active_connections; ++i) {
            if (FD_ISSET(connections[i], &read_fds)) {
                printf("\t[Server] Client (fd %d) is sending a signal...\n", connections[i]);
                char buf[HEADER_SIZE + MAX_MSG_LEN];
                ssize_t message_size = read(connections[i], buf, sizeof(buf));
                if (message_size <= 0) {
                    // Client disconnected
                    close(connections[i]);
                    printf("\t[Server] Client (fd %d) disconnected\n", connections[i]);
                    connections[i] = connections[--active_connections]; // Copy last fd into dead fd and shrinks active_connections
                } else {
                    // Broadcast message to everyone else
                    for (int j = 0; j < active_connections; ++j) {
                        if (i != j) {
                            if (write(connections[j], buf, (size_t)message_size) < 0) {
                                perror("[ERROR] Could not broadcast message\n");
                                continue;
                            }
                            printf("\t[Server] Broadcasted message from fd %d to fd %d\n", connections[i], connections[j]);
                        }
                    }
                }
                printf("\t[Server] The message of size %zd has been broadcasted\n", message_size);
            }
        }
    }

    // Close the server
    printf("[Server] Shutting down server...\n");
    if (close(server_fd) < 0) {
        perror("[ERROR] The server could not shutdown\n");
        return 1;
    }
    printf("[Server] The server has shut down\n");

    return 0;
}
