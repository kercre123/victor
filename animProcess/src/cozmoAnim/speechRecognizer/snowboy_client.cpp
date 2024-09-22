#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "snowboy_client.h"

#define PORT 12345

static int sockfd = -1;

int snowboy_init(void) {
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
        perror("Socket creation error");
        return -1;
    }
  
    memset(&serv_addr, 0, sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)  {
        perror("Invalid address / Address not supported");
        close(sockfd);
        sockfd = -1;
        return -1;
    }
  
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  {
        perror("Connection Failed");
        close(sockfd);
        sockfd = -1;
        return -1;
    }
    return 0;
}

int snowboy_process(const char* data, uint32_t data_length) {
    if (sockfd < 0) {
        fprintf(stderr, "Socket not initialized\n");
        return -1;
    }

    uint32_t data_length_net = htonl(data_length);
    ssize_t n = write(sockfd, &data_length_net, sizeof(data_length_net));
    if (n < 0) {
        perror("write");
        return -1;
    } else if ((size_t)n != sizeof(data_length_net)) {
        fprintf(stderr, "Incomplete write of data length\n");
        return -1;
    }

    size_t bytes_sent = 0;
    while (bytes_sent < data_length) {
        n = write(sockfd, data + bytes_sent, data_length - bytes_sent);
        if (n <= 0) {
            perror("write");
            return -1;
        }
        bytes_sent += n;
    }

    uint32_t result_net;
    n = read(sockfd, &result_net, sizeof(result_net));
    if (n <= 0) {
        perror("read");
        return -1;
    } else if ((size_t)n != sizeof(result_net)) {
        fprintf(stderr, "Incomplete read of result code\n");
        return -1;
    }
    int result = ntohl(result_net);
    return result;
}

void snowboy_close(void) {
    if (sockfd >= 0) {
        uint32_t data_length_net = htonl(0);
        ssize_t n = write(sockfd, &data_length_net, sizeof(data_length_net));
        if (n < 0) {
            perror("write");
        } else if ((size_t)n != sizeof(data_length_net)) {
            fprintf(stderr, "Incomplete write in snowboy_close\n");
        }
        close(sockfd);
        sockfd = -1;
    }
}
