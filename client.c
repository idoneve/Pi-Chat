#include "signal.h"

int connect_to_server(char *ip_addr) {
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
    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[Error] Failed to connect to server");
        exit(1);
    }
    printf("\t[Client] Connected to socket\n");

    return socket_fd;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("IP address of server must be provided as first argument");
        return 1;
    }

    printf("[Client] Trying to connect to server...\n");
    setup_signal_handler();
    int socket_fd = connect_to_server(argv[1]);
    printf("[Client] Connected to server\n\n");

    // Main loop
    fd_set read_fds;
    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // Watch STDIN
        FD_SET(socket_fd, &read_fds); // Watch socket

        int max_fd = (STDIN_FILENO > socket_fd) ? STDIN_FILENO : socket_fd;
        printf("[Client] Waiting for input...\n");
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue; // ctrl+C was pressed
            } else {
                perror("[ERROR] Failed to select fd with data\n");
                break;
            }
        }
        
        // Signal from STDIN
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[MAX_MSG_LEN];
            fgets(input, sizeof(input), stdin); // Get input
            input[strcspn(input, "\n")] = '\0'; // Add terminator
            if (send_message(socket_fd, input) < 0) { // Send to server
                printf("[Client] Failed to send, server may be down\n");
                break;
            }
        }

        // Signal from server
        if (FD_ISSET(socket_fd, &read_fds)) {
            char message[MAX_MSG_LEN];
            int message_size = unpack_message(socket_fd, message, sizeof(message));
            if (message_size <= 0) {
                printf("[Client] Server has disconnected\n");
                break;
            }
            printf("[Client]: Recieved message from server: \"%s\"\n", message);
        }
    }

    // Disconnect the client
    printf("[Client] Disconnecting from server...\n");
    if (close(socket_fd) < 0) {
        perror("[ERROR] Could not disconnect from server\n");
        return 1;
    }
    printf("[Client] Disconnected\n");

    return 0;
}
