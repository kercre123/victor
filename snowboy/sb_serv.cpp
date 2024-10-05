// snowboy_server.c


#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "include/snowboy-detect.h"

#define SOCKET_PATH "/dev/_anim_sb_wakeword_"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: snowboy_server <resource_filename> <model_filename>\n");
        return 1;
    }

    const char* resource_filename = argv[1];
    const char* model_filename = argv[2];

    // Initialize Snowboy detector
    snowboy::SnowboyDetect detector(resource_filename, model_filename);
    detector.SetSensitivity("0.45"); // Adjust as needed
    detector.SetAudioGain(1.0);
    detector.ApplyFrontend(false);

    int server_fd, client_fd;
    struct sockaddr_un address;
    socklen_t addr_len;

    // Remove any existing socket file
    unlink(SOCKET_PATH);

    // Create a Unix domain socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(struct sockaddr_un)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Set permissions for the socket file
    chmod(SOCKET_PATH, 0666);

    // Start listening for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Snowboy server listening on %s\n", SOCKET_PATH);

    while (1) {
        // Accept a client connection
        addr_len = sizeof(struct sockaddr_un);
        if ((client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len)) == -1) {
            perror("accept failed");
            continue;
        }
        printf("Client connected\n");

        // Handle client connection
        while (1) {
            // Read data length (4 bytes)
            uint32_t data_length_net;
            ssize_t n = read(client_fd, &data_length_net, sizeof(data_length_net));
            if (n == 0) {
                // Connection closed
                break;
            } else if (n < 0) {
                perror("read failed");
                break;
            } else if ((size_t)n != sizeof(data_length_net)) {
                fprintf(stderr, "Incomplete read of data length\n");
                break;
            }

            uint32_t data_length = ntohl(data_length_net);
            if (data_length == 0) {
                // Client indicates end of data
                break;
            }

            // Read data buffer
            char* data_buffer = (char*)malloc(data_length);
            if (!data_buffer) {
                fprintf(stderr, "Memory allocation failed\n");
                break;
            }
            size_t bytes_read = 0;
            while (bytes_read < data_length) {
                n = read(client_fd, data_buffer + bytes_read, data_length - bytes_read);
                if (n <= 0) {
                    perror("read failed");
                    free(data_buffer);
                    break;
                }
                bytes_read += n;
            }
            if (bytes_read < data_length) {
                // Error or connection closed
                free(data_buffer);
                break;
            }

            // Process data with SnowboyDetect
            int result = detector.RunDetection(std::string(data_buffer, data_length));
            free(data_buffer);

            // Send back result code (4 bytes)
            uint32_t result_net = htonl(result);
            n = write(client_fd, &result_net, sizeof(result_net));
            if (n <= 0) {
                perror("write failed");
                break;
            } else if ((size_t)n != sizeof(result_net)) {
                fprintf(stderr, "Incomplete write of result code\n");
                break;
            }
        }
        close(client_fd);
        printf("Client disconnected\n");
    }

    close(server_fd);
    unlink(SOCKET_PATH); // Clean up the socket file
    return 0;
}
