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
    addr.sin_family = AF_INET; // Use ipv4
    addr.sin_addr.s_addr = INADDR_ANY; // Listens on all network interfaces
    addr.sin_port = htons(PORT); // Get port
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] Failed to bind socket\n");
        exit(1);
    }
    printf("\t[Server] Socket binded\n");

    // Listen to socket
    printf("\t[Server] Starting to listen to socket...\n");
    if (listen(server_fd, 16) < 0) {
        perror("[ERROR] Failed to listen to socket\n");
        exit(1);
    }
    printf("\t[Server] Listening to socket\n");

    return server_fd;
}

int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("[ERROR] Failed to accept client connection\n");
        return -1;
    }
    return client_fd;
}

int main(void) {
    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n");

    const size_t MAX_CONNECTIONS = 2;
    int connections[MAX_CONNECTIONS];
    int start = 0, active_connections = 0;

    // Main loop
    while (running) {
        int client_fd;

        if (active_connections != MAX_CONNECTIONS && (client_fd = accept_client(server_fd)) >= 0) {
            printf("[Server] Connection Accepted\n");
            connections[(start + active_connections) % MAX_CONNECTIONS] = client_fd;
            active_connections += 1;
            continue;
        }

        if (active_connections == 0) {
            continue;
        }

        // Get Messages
        for (int i = 0; i < active_connections; i++) {
            char message_buf[MAX_MSG_LEN + 1];
            int msg_len;

            int current_connection = (start + i) % MAX_CONNECTIONS;
            if ((msg_len = unpack_message(connections[current_connection], message_buf, MAX_MSG_LEN)) < 0) {
                printf("[SERVER] Failed to unpack message\n");
                running = 0;
            }
            if (msg_len > 0){
                printf("[Sever] Message Recived: %s\n", message_buf);
            }
        }
    }

    printf("[Server] The server has shut down\n");

    return 0;
}
