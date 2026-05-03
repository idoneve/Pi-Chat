#include "signal.h"
#include "chat.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>

int start_server() {
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
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // Use IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // Listens on all network interfaces
    addr.sin_port = htons(PORT); // Get port
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
    int connections[MAX_CONNECTIONS];
    int active_connections = 0;
    fd_set read_fds;

    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n");

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

        select(max_fd + 1, &read_fds, NULL, NULL, NULL); // Only proceed if watched fd is ready to read (no blocks)

        // Accept clients
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept_client(server_fd);
            if (client_fd < 0) {
                printf("[ERROR] Could not accept client\n");
            } else if (active_connections >= MAX_CONNECTIONS) {
                printf("[Server] Server is full\n");
                close(client_fd); // Reject client if full
            } else {
                connections[active_connections++] = client_fd;
                printf("[Server] Connection accepted (fd %d)\n", client_fd);
            }
        }

        // Check clients for messages
        for (int i = 0; i < active_connections; ++i) {
            if (FD_ISSET(connections[i], &read_fds)) {
                char buf[HEADER_SIZE + MAX_MSG_LEN];
                int n = read(connections[i], buf, sizeof(buf));

                if (n <= 0) { // Client disconnected

                }
            }
        }
    }

    printf("[Server] The server has shut down\n");

    return 0;
}
