#include "chat.h"

int main(int argc, char* argv[]) {
    printf("[Server] starting up server...\n");

    // create socket
    printf("[Server] creating socket...\n");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] failed to create socket\n");
        exit(1);
    }
    printf("[Server] socket created\n");
    
    // bind port to socket
    printf("[Server] binding socket...\n");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // use ipv4
    addr.sin_addr.s_addr = INADDR_ANY; // listens on all network interfaces
    addr.sin_port = htons(PORT); // get port
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] failed to bind socket\n");
        exit(1);
    }
    printf("[Server] socket binded\n");

    // listen to socket
    printf("[Server] starting to listen to socket...\n");
    if (listen(server_fd, 16) < 0) {
        perror("[ERROR] failed to listen to socket\n");
        exit(1);
    }
    printf("[Server] listening to socket\n");
    printf("[Server] server has started\n");

    // main loop
    while (running) {
        // Message from A -> B

        // Message from B -> A

        // Kill signal
    }
    printf("[Server] has shut down\n");

    return 0;
}
