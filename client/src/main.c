#include "chat.h"
#include "ui.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int connect_to_server(char* ip_addr) {
    // Create socket
    printf("\t[Client] Creating socket...\n");
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("[ERROR] Failed to create socket\n");
        exit(1);
    }
    printf("\t[Client] Socket created\n");

    // Configure socket
    printf("\t[Client] Configuring socket...\n");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // Use ipv4
    addr.sin_port = htons(PORT); // Get port
    inet_pton(AF_INET, ip_addr, &addr.sin_addr);
    printf("\t[Client] Socket configured\n");

    // Connect to socket
    printf("\t[Client] Connecting to socket...\n");
    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[Error] Failed to connect to server");
        exit(1);
    }
    printf("\t[Client] Connected to socket\n");

    return socket_fd;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Ip address of server must be provided as first argument");
        return 1;
    }

    printf("[Client] Trying to connect to server...\n");
    setup_signal_handler();
    // int socket_fd = connect_to_server(argv[1]);
    printf("[Client] Connected to server\n");

    printf("[Client] Starting GUI\n");
    start_ui_app(0);

    return 0;
}
