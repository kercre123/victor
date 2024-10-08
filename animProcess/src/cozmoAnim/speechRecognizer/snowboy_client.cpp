// snowboy_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "snowboy_client.h"

#define SOCKET_PATH "/dev/_anim_sb_wakeword_"

static int sockfd = -1;

int snowboy_init(void) {
    struct sockaddr_un serv_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_un));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKET_PATH, sizeof(serv_addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Connection to Snowboy server failed");
        close(sockfd);
        sockfd = -1;
        return -1;
    }

    return 0;
}

int snowboy_process(const char* data, uint32_t data_length) {
    if (sockfd < 0) {
        fprintf(stderr, "Snowboy client not initialized\n");
        return -1;
    }

    // data length
    uint32_t data_length_net = htonl(data_length);
    ssize_t n = write(sockfd, &data_length_net, sizeof(data_length_net));
    if (n < 0) {
        perror("Write failed");
        return -1;
    } else if ((size_t)n != sizeof(data_length_net)) {
        fprintf(stderr, "Incomplete write of data length\n");
        return -1;
    }

    // data
    size_t bytes_sent = 0;
    while (bytes_sent < data_length) {
        n = write(sockfd, data + bytes_sent, data_length - bytes_sent);
        if (n <= 0) {
            perror("Write failed");
            return -1;
        }
        bytes_sent += n;
    }

    // result code
    uint32_t result_net;
    n = read(sockfd, &result_net, sizeof(result_net));
    if (n <= 0) {
        perror("Read failed");
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
        // Send data length 0 to indicate end of data
        uint32_t data_length_net = htonl(0);
        ssize_t n = write(sockfd, &data_length_net, sizeof(data_length_net));
        if (n < 0) {
            perror("Write failed in snowboy_close");
        } else if ((size_t)n != sizeof(data_length_net)) {
            fprintf(stderr, "Incomplete write in snowboy_close\n");
        }
        close(sockfd);
        sockfd = -1;
    }
}
