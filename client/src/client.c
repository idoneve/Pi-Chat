#include "client.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <ui.h>
#include <chat.h>

volatile sig_atomic_t running = 1;

static void connect_to_socket(int socket_fd, const struct sockaddr_in* addr, socklen_t addr_size) {
    printf("\t[Client] Connecting to socket...\n");
    if (connect(socket_fd, (struct sockaddr*)&addr, addr_size) < 0) {
        perror("[Error] Failed to connect to server");
        exit(1);
    }
    printf("\t[Client] Connected to socket\n");
}

int connect_to_server(char* ip_addr) {
    int socket_fd = create_socket();

    printf("\t[Client] Configuring socket...\n");
    struct sockaddr_in addr = configure_socket();
    inet_pton(AF_INET, ip_addr, &addr.sin_addr);
    printf("\t[Client] Socket configured\n");

    connect_to_socket(socket_fd, &addr, sizeof(addr));

    return socket_fd;
}

static int reload_fds(int socket_fd, fd_set* read_fds) {
    FD_ZERO(read_fds);
    FD_SET(STDIN_FILENO, read_fds); // Watch STDIN
    FD_SET(socket_fd, read_fds); // Watch socket
                                 //
    int max_fd = (STDIN_FILENO > socket_fd) ? STDIN_FILENO : socket_fd;
    printf("[Client] Waiting for input...\n");
    return max_fd;
}

static int start_cli(int socket_fd) {
    // Main loop
    fd_set read_fds;
    while (running) {
        int max_fd = reload_fds(socket_fd, &read_fds);
        is_signal_ready(max_fd, &read_fds);

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

        // Signal from STDIN
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[MAX_MSG_LEN];
            if (fgets(input, sizeof(input), stdin) == NULL) { // Get input
                perror("[ERROR] Something went wrong reading input\n");
                break;
            }
            input[strcspn(input, "\n")] = '\0'; // Add terminator
            if (send_message(socket_fd, input) < 0) { // Send to server
                printf("[Client] Failed to send, server may be down\n");
                break;
            }
        }

        // Signal from server
        if (FD_ISSET(socket_fd, &read_fds)) {
            char message[MAX_MSG_LEN];
            ssize_t message_size = unpack_message(socket_fd, message, sizeof(message));
            if (message_size <= 0) {
                printf("[Client] Server has disconnected\n");
                break;
            }
            printf("[Client]: Recieved message from server: \"%s\"\n", message);
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        perror("IP address of server must be provided as first argument");
        return 1;
    }

    printf("[Client] Trying to connect to server...\n");
    setup_signal_handler();
    int socket_fd = connect_to_server(argv[1]);
    printf("[Client] Connected to server\n\n");

    bool use_cli = false;
    if (argc >= 3) {
        if (strcmp(argv[2], "-cli") == 0) {
            use_cli = true;
        } else {
            printf("[CLIENT] [ERROR] - Unrecognized argument provided %s\n", argv[2]);
        }
    }

    int exit;
    if (!use_cli) {
        exit = start_ui_app(socket_fd);
    } else {
        exit = start_cli(socket_fd);
    }

    // Disconnect the client
    printf("[Client] Disconnecting from server...\n");
    if (close(socket_fd) < 0) {
        perror("[ERROR] Could not disconnect from server\n");
        return 1;
    }
    printf("[Client] Disconnected\n");

    return exit;
}
