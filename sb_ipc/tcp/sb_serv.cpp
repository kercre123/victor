#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "include/snowboy-detect.h"

#define PORT 12345

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "snowboy_server <resource_filename> <model_filename>" << std::endl;
        return 1;
    }

    std::string resource_filename = argv[1];
    std::string model_filename = argv[2];

    snowboy::SnowboyDetect detector(resource_filename, model_filename);
    detector.SetSensitivity("0.73");
    detector.SetAudioGain(1.0);
    detector.ApplyFrontend(false);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))  {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
  
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)  {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)  {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Snowboy server listening on port " << PORT << std::endl;
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                           (socklen_t*)&addrlen))<0)  {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        std::cout << "Client connected" << std::endl;
        while (true) {
            // data length (4 bytes)
            uint32_t data_length_net;
            int n = read(new_socket, &data_length_net, sizeof(data_length_net));
            if (n == 0) {
                // connection closed
                break;
            } else if (n < 0) {
                perror("read");
                break;
            }
            uint32_t data_length = ntohl(data_length_net);
            if (data_length == 0) {
                // eof
                break;
            }
            std::string data_buffer(data_length, 0);
            size_t bytes_read = 0;
            while (bytes_read < data_length) {
                n = read(new_socket, &data_buffer[bytes_read], data_length - bytes_read);
                if (n <= 0) {
                    perror("read");
                    break;
                }
                bytes_read += n;
            }
            if (bytes_read < data_length) {
                break;
            }

            int result = detector.RunDetection(data_buffer);
            // result code (4 bytes)
            uint32_t result_net = htonl(result);
            n = write(new_socket, &result_net, sizeof(result_net));
            if (n <= 0) {
                perror("write");
                break;
            }
        }
        close(new_socket);
        std::cout << "Client disconnected" << std::endl;
    }
    close(server_fd);
    return 0;
}
