#include "chat.h"

volatile sig_atomic_t running = 1;

static void kill_sig(int sig) {
    (void)sig;
    running = 0;
}

void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = kill_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}

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
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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

int main(void) {
    printf("[Server] Starting up server...\n");
    setup_signal_handler();
    int server_fd = start_server();
    printf("[Server] Server has started\n");

    // Main loop
    while (running) {
        // Forward messages from one client to another
    }
    printf("[Server] The server has shut down\n");

    return 0;
}
