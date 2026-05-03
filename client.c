#include "signal.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
    if (argc < 2) {
        perror("IP address of server must be provided as first argument");
        return 1;
    }

    printf("[Client] Trying to connect to server...\n");
    setup_signal_handler();
    int socket_fd = connect_to_server(argv[1]);
    printf("[Client] Connected to server\n");

    char message_buf[MAX_MSG_LEN + 1];
    while (running) {
        // Read message from stdin
        printf("\nEnter Message: ");
        if (fgets(message_buf, MAX_MSG_LEN, stdin) == NULL) {
            printf("[ERROR] Message too long: Max Length: %d characters\n", MAX_MSG_LEN - HEADER_SIZE);
            continue;
        }
        printf("[Client] User input recieved\n");

        // Send message to server
        message_buf[strcspn(message_buf, "\n")] = 0; // add terminator at end of line
        printf("[Client] Sending Message %s to %s...\n", message_buf, argv[1]);
        send_message(socket_fd, message_buf);
        printf("[Client] Message Sent\n");

        // Listen for message from server
        // if ((msg_len = recv(socket_fd, message_buf, MAX_MSG_LEN, 0)) < 0) {
        //     printf("[Client] failed to listen to message from server");
        // //     continue;
        // }

        printf("[Client] Listening for server...\n");
        int msg_len;
        if ((msg_len = unpack_message(socket_fd, message_buf, MAX_MSG_LEN)) < 0) {
            perror("[ERROR] Failed to unpack message. Server Connection may have closed\n");
            return 1;
        }
        message_buf[msg_len] = '\0';

        // Unpack and display message

        printf("[Client] Message Received: %s\n", message_buf);

        // Send kill to server
    }

    // start_ui_app(socket_fd);

    return 0;
}
