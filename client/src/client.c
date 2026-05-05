#include "client.h"
#include <complex.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <ui.h>
#include <chat.h>

volatile sig_atomic_t running = 1;

static void connect_to_socket(int socket_fd, struct sockaddr_in* addr, socklen_t addr_size) {
    printf("\t[Client] Connecting to socket...\n");
    if (connect(socket_fd, (struct sockaddr*)addr, addr_size) < 0) {
        perror("[Error] Failed to connect to server");
        exit(1);
    }
    printf("\t[Client] Connected to socket\n");
}

static int connect_to_server(char* ip_addr) {
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
    return max_fd;
}

static int start_cli(int socket_fd) {
    // Main loop
    fd_set read_fds;
    while (running) {
        int max_fd = reload_fds(socket_fd, &read_fds);

        printf("[CLIENT] Waiting for input.\n");

        bool destination_received = false;
        printf("[CLIENT] Enter Destination Address\n->");
        fflush(stdout);

        SignalResponse signal_response;
        if ((signal_response = is_signal_ready(max_fd, &read_fds)) != SIGNAL) {
            if (signal_response == INTERRUPT) {
                continue;
            }
            if (signal_response == FD_ERROR) {
                perror("[ERROR] Failed to select fd with data\n");
                break;
            }
        }

        // Signal from STDIN
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            destination_received = true;

            ClientMessage message = {
                .type = SEND,
            };

            if (fgets(message.ip, sizeof(message.ip), stdin) == NULL) { // Get input
                perror("[ERROR] Something went wrong reading input\n");
                break;
            }
            message.ip[strcspn(message.ip, "\n")] = '\0'; // Add terminator
            printf("[CLIENT] Input received address %s\n", message.ip);

            struct in_addr dummy;
            if (inet_pton(AF_INET, message.ip, &dummy) != 1) {
                printf("[ERROR] Input was not a valid address\n");
                continue; // not a valid IPv4 address
            }

            char input[MAX_MSG_LEN];
            printf("[CLIENT] Enter Message\n->");
            fflush(stdout);
            if (fgets(input, sizeof(input), stdin) == NULL) { // Get input
                perror("[ERROR] Something went wrong reading input\n");
                break;
            }
            input[strcspn(input, "\n")] = '\0'; // Add terminator
                                                //
            printf("[CLIENT] Input received messsage %s\n", message.ip);

            if (strlen(input) == 0)
                continue; // ignore empty reads, go back to select

            message.content.data = input;
            message.content.len = strnlen(input, MAX_MSG_LEN);

            if (send_message(socket_fd, &message) < 0) { // Send to server
                printf("[Client] Failed to send, server may be down\n");
                break;
            }
        }

        // Signal from server
        if (!FD_ISSET(socket_fd, &read_fds))
            continue;

        if (!destination_received)
            printf("\n\t[CLIENT] Server messsage received first\n");

        Message m = unpack_message(socket_fd);

        if (m.type == DISCONNECT) {
            printf("[Client] Server has disconnected\n");
            break;
        }

        switch (m.type) {
        case INVALID:
            printf("[ERROR] Invalid message received from server\n");
            continue;
        case ACTIVITY:
            continue;
        case MESSAGE:
            if (m.type_data.message.type == SEND) {
                printf("[ERROR] SEND message type received from server. Ignoring\n");
                free(m.type_data.message.content.data);
                continue;
            }
            break;
        default:
            break;
        }

        printf(
            "[Client]: Recieved message from server: \"%s\"\n", m.type_data.message.content.data);

        // Discard message after done
        free(m.type_data.message.content.data);
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
