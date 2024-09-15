// pv_porcupine_server.c

#include "pv_porcupine.h"
#include "pv_pork_ipc.h"
#include "picovoice.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

// Define the socket path
#ifndef SOCKET_PATH
#define SOCKET_PATH "/tmp/pv_porcupine_socket"
#endif

// Define message types
#define MSG_INIT                      1
#define MSG_MULTIPLE_KEYWORDS_INIT    2
#define MSG_DELETE                    3
#define MSG_PROCESS                   4
#define MSG_MULTIPLE_KEYWORDS_PROCESS 5
#define MSG_VERSION                   6
#define MSG_FRAME_LENGTH              7
#define MSG_SAMPLE_RATE               8

#define MAX_OBJECTS 1 // Only one object since there will only ever be one client

typedef struct {
    int in_use;
    pv_porcupine_object_t *object;
} object_entry_t;

object_entry_t object_table;

// Helper function to log messages with timestamps
static void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf)-1, "%Y-%m-%d %H:%M:%S", t);
    
    // Print timestamp
    printf("[%s] ", timebuf);
    
    // Print the actual log message
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

// Helper function to send data over socket
static int send_data(int sockfd, const void *data, size_t size) {
    ssize_t total_sent = 0;
    const char *data_ptr = (const char *)data;
    while (total_sent < (ssize_t)size) {
        ssize_t sent = write(sockfd, data_ptr + total_sent, size - total_sent);
        if (sent <= 0) {
            log_message("send_data: Failed to send data. Error: %s", strerror(errno));
            return -1;
        }
        total_sent += sent;
    }
    log_message("send_data: Sent %zu bytes", size);
    return 0;
}

// Helper function to receive data from socket
static int receive_data(int sockfd, void *data, size_t size) {
    ssize_t total_received = 0;
    char *data_ptr = (char *)data;
    while (total_received < (ssize_t)size) {
        ssize_t received = read(sockfd, data_ptr + total_received, size - total_received);
        if (received < 0) {
            log_message("receive_data: Read error. Error: %s", strerror(errno));
            return -1;
        } else if (received == 0) {
            log_message("receive_data: Connection closed by peer.");
            return -1;
        }
        total_received += received;
    }
    log_message("receive_data: Received %zu bytes", size);
    return 0;
}

// Function to handle the client connection
void handle_client(int client_fd) {
    uint8_t msg_type;
    while (1) {
        log_message("handle_client: Waiting to receive message type...");
        int recv_status = receive_data(client_fd, &msg_type, sizeof(msg_type));
        if (recv_status < 0) {
            log_message("handle_client: Failed to receive message type. Closing connection.");
            break;
        }
        log_message("handle_client: Received message type: %u", msg_type);
        
        switch (msg_type) {
            case MSG_INIT: {
                log_message("handle_client: Handling MSG_INIT");
                // Receive model_file_path
                uint32_t len_network;
                if (receive_data(client_fd, &len_network, sizeof(len_network)) < 0) {
                    log_message("handle_client: Failed to receive model_file_path length.");
                    break;
                }
                uint32_t len = ntohl(len_network);
                log_message("handle_client: model_file_path length: %u", len);
                char *model_file_path = malloc(len);
                if (!model_file_path) {
                    log_message("handle_client: Failed to allocate memory for model_file_path.");
                    break;
                }
                if (receive_data(client_fd, model_file_path, len) < 0) {
                    log_message("handle_client: Failed to receive model_file_path data.");
                    free(model_file_path);
                    break;
                }
                log_message("handle_client: Received model_file_path: %s", model_file_path);

                // Receive keyword_file_path
                if (receive_data(client_fd, &len_network, sizeof(len_network)) < 0) {
                    log_message("handle_client: Failed to receive keyword_file_path length.");
                    free(model_file_path);
                    break;
                }
                len = ntohl(len_network);
                log_message("handle_client: keyword_file_path length: %u", len);
                char *keyword_file_path = malloc(len);
                if (!keyword_file_path) {
                    log_message("handle_client: Failed to allocate memory for keyword_file_path.");
                    free(model_file_path);
                    break;
                }
                if (receive_data(client_fd, keyword_file_path, len) < 0) {
                    log_message("handle_client: Failed to receive keyword_file_path data.");
                    free(model_file_path);
                    free(keyword_file_path);
                    break;
                }
                log_message("handle_client: Received keyword_file_path: %s", keyword_file_path);

                // Receive sensitivity
                float sensitivity;
                if (receive_data(client_fd, &sensitivity, sizeof(sensitivity)) < 0) {
                    log_message("handle_client: Failed to receive sensitivity.");
                    free(model_file_path);
                    free(keyword_file_path);
                    break;
                }
                log_message("handle_client: Received sensitivity: %f", sensitivity);

                // Call pv_porcupine_init
                pv_porcupine_object_t *object = NULL;
                pv_status_t status = pv_porcupine_init(
                    model_file_path, keyword_file_path, sensitivity, &object);

                free(model_file_path);
                free(keyword_file_path);

                log_message("handle_client: pv_porcupine_init returned status: %d", status);

                // Send status
                if (send_data(client_fd, &status, sizeof(status)) < 0) {
                    log_message("handle_client: Failed to send status.");
                    break;
                }

                if (status == PV_STATUS_SUCCESS) {
                    // Since there's only one object, object ID is 1
                    object_table.in_use = 1;
                    object_table.object = object;
                    int object_id = 1;

                    // Send object ID
                    if (send_data(client_fd, &object_id, sizeof(object_id)) < 0) {
                        log_message("handle_client: Failed to send object_id.");
                        break;
                    }
                    log_message("handle_client: Sent object_id: %d", object_id);
                }
                break;
            }

            case MSG_MULTIPLE_KEYWORDS_INIT: {
                log_message("handle_client: Handling MSG_MULTIPLE_KEYWORDS_INIT");
                // Receive model_file_path
                uint32_t len_network;
                if (receive_data(client_fd, &len_network, sizeof(len_network)) < 0) {
                    log_message("handle_client: Failed to receive model_file_path length.");
                    break;
                }
                uint32_t len = ntohl(len_network);
                log_message("handle_client: model_file_path length: %u", len);
                char *model_file_path = malloc(len);
                if (!model_file_path) {
                    log_message("handle_client: Failed to allocate memory for model_file_path.");
                    break;
                }
                if (receive_data(client_fd, model_file_path, len) < 0) {
                    log_message("handle_client: Failed to receive model_file_path data.");
                    free(model_file_path);
                    break;
                }
                log_message("handle_client: Received model_file_path: %s", model_file_path);

                // Receive number of keywords
                int32_t num_kw_network;
                if (receive_data(client_fd, &num_kw_network, sizeof(num_kw_network)) < 0) {
                    log_message("handle_client: Failed to receive num_keywords.");
                    free(model_file_path);
                    break;
                }
                int32_t num_kw = ntohl(num_kw_network);
                log_message("handle_client: Number of keywords: %d", num_kw);

                // Receive keyword_file_paths
                char **keyword_file_paths = malloc(num_kw * sizeof(char *));
                if (!keyword_file_paths) {
                    log_message("handle_client: Failed to allocate memory for keyword_file_paths.");
                    free(model_file_path);
                    break;
                }
                for (int i = 0; i < num_kw; i++) {
                    if (receive_data(client_fd, &len_network, sizeof(len_network)) < 0) {
                        log_message("handle_client: Failed to receive keyword_file_path[%d] length.", i);
                        free(model_file_path);
                        for (int j = 0; j < i; j++) free(keyword_file_paths[j]);
                        free(keyword_file_paths);
                        break;
                    }
                    len = ntohl(len_network);
                    log_message("handle_client: keyword_file_path[%d] length: %u", i, len);
                    keyword_file_paths[i] = malloc(len);
                    if (!keyword_file_paths[i]) {
                        log_message("handle_client: Failed to allocate memory for keyword_file_path[%d].", i);
                        free(model_file_path);
                        for (int j = 0; j < i; j++) free(keyword_file_paths[j]);
                        free(keyword_file_paths);
                        break;
                    }
                    if (receive_data(client_fd, keyword_file_paths[i], len) < 0) {
                        log_message("handle_client: Failed to receive keyword_file_path[%d] data.", i);
                        free(model_file_path);
                        for (int j = 0; j <= i; j++) free(keyword_file_paths[j]);
                        free(keyword_file_paths);
                        break;
                    }
                    log_message("handle_client: Received keyword_file_path[%d]: %s", i, keyword_file_paths[i]);
                }

                // Receive sensitivities
                float *sensitivities = malloc(num_kw * sizeof(float));
                if (!sensitivities) {
                    log_message("handle_client: Failed to allocate memory for sensitivities.");
                    free(model_file_path);
                    for (int i = 0; i < num_kw; i++) free(keyword_file_paths[i]);
                    free(keyword_file_paths);
                    break;
                }
                if (receive_data(client_fd, sensitivities, sizeof(float) * num_kw) < 0) {
                    log_message("handle_client: Failed to receive sensitivities.");
                    free(model_file_path);
                    for (int i = 0; i < num_kw; i++) free(keyword_file_paths[i]);
                    free(keyword_file_paths);
                    free(sensitivities);
                    break;
                }
                log_message("handle_client: Received sensitivities.");

                // Call pv_porcupine_multiple_keywords_init
                pv_porcupine_object_t *object = NULL;
                pv_status_t status = pv_porcupine_multiple_keywords_init(
                    model_file_path, num_kw, (const char * const *)keyword_file_paths, sensitivities, &object);

                free(model_file_path);
                for (int i = 0; i < num_kw; i++) free(keyword_file_paths[i]);
                free(keyword_file_paths);
                free(sensitivities);

                log_message("handle_client: pv_porcupine_multiple_keywords_init returned status: %d", status);

                // Send status
                if (send_data(client_fd, &status, sizeof(status)) < 0) {
                    log_message("handle_client: Failed to send status.");
                    break;
                }

                if (status == PV_STATUS_SUCCESS) {
                    // Since there's only one object, object ID is 1
                    object_table.in_use = 1;
                    object_table.object = object;
                    int object_id = 1;

                    // Send object ID
                    if (send_data(client_fd, &object_id, sizeof(object_id)) < 0) {
                        log_message("handle_client: Failed to send object_id.");
                        break;
                    }
                    log_message("handle_client: Sent object_id: %d", object_id);
                }
                break;
            }

            case MSG_DELETE: {
                log_message("handle_client: Handling MSG_DELETE");
                int object_id;
                if (receive_data(client_fd, &object_id, sizeof(object_id)) < 0) {
                    log_message("handle_client: Failed to receive object_id for delete.");
                    break;
                }
                log_message("handle_client: Received object_id to delete: %d", object_id);
                if (object_table.in_use && object_id == 1) {
                    pv_porcupine_delete(object_table.object);
                    object_table.in_use = 0;
                    object_table.object = NULL;
                    log_message("handle_client: Deleted object_id: %d", object_id);
                } else {
                    log_message("handle_client: Invalid object_id: %d", object_id);
                }
                break;
            }

            case MSG_PROCESS: {
                log_message("handle_client: Handling MSG_PROCESS");
                int object_id;
                if (receive_data(client_fd, &object_id, sizeof(object_id)) < 0) {
                    log_message("handle_client: Failed to receive object_id for process.");
                    break;
                }
                log_message("handle_client: Received object_id for process: %d", object_id);
                if (!(object_table.in_use && object_id == 1)) {
                    pv_status_t status = PV_STATUS_INVALID_ARGUMENT;
                    log_message("handle_client: Invalid object_id: %d", object_id);
                    if (send_data(client_fd, &status, sizeof(status)) < 0) {
                        log_message("handle_client: Failed to send PV_STATUS_INVALID_ARGUMENT.");
                        break;
                    }
                    break;
                }
                pv_porcupine_object_t *object = object_table.object;

                int frame_length = pv_porcupine_frame_length();
                log_message("handle_client: frame_length: %d", frame_length);
                int16_t *pcm = malloc(sizeof(int16_t) * frame_length);
                if (!pcm) {
                    pv_status_t status = PV_STATUS_OUT_OF_MEMORY;
                    log_message("handle_client: Failed to allocate memory for PCM data.");
                    if (send_data(client_fd, &status, sizeof(status)) < 0) {
                        log_message("handle_client: Failed to send PV_STATUS_OUT_OF_MEMORY.");
                        break;
                    }
                    break;
                }
                if (receive_data(client_fd, pcm, sizeof(int16_t) * frame_length) < 0) {
                    log_message("handle_client: Failed to receive PCM data.");
                    free(pcm);
                    break;
                }
                log_message("handle_client: Received PCM data for processing.");

                bool result;
                pv_status_t status = pv_porcupine_process(object, pcm, &result);
                free(pcm);

                log_message("handle_client: pv_porcupine_process returned status: %d, result: %d", status, result);

                if (send_data(client_fd, &status, sizeof(status)) < 0) {
                    log_message("handle_client: Failed to send process status.");
                    break;
                }
                if (status == PV_STATUS_SUCCESS) {
                    uint8_t res = result ? 1 : 0;
                    if (send_data(client_fd, &res, sizeof(res)) < 0) {
                        log_message("handle_client: Failed to send process result.");
                        break;
                    }
                    log_message("handle_client: Sent process result: %d", res);
                }
                break;
            }

            case MSG_MULTIPLE_KEYWORDS_PROCESS: {
                log_message("handle_client: Handling MSG_MULTIPLE_KEYWORDS_PROCESS");
                int object_id;
                if (receive_data(client_fd, &object_id, sizeof(object_id)) < 0) {
                    log_message("handle_client: Failed to receive object_id for multiple keywords process.");
                    break;
                }
                log_message("handle_client: Received object_id for multiple keywords process: %d", object_id);
                if (!(object_table.in_use && object_id == 1)) {
                    pv_status_t status = PV_STATUS_INVALID_ARGUMENT;
                    log_message("handle_client: Invalid object_id: %d", object_id);
                    if (send_data(client_fd, &status, sizeof(status)) < 0) {
                        log_message("handle_client: Failed to send PV_STATUS_INVALID_ARGUMENT.");
                        break;
                    }
                    break;
                }
                pv_porcupine_object_t *object = object_table.object;

                int frame_length = pv_porcupine_frame_length();
                log_message("handle_client: frame_length: %d", frame_length);
                int16_t *pcm = malloc(sizeof(int16_t) * frame_length);
                if (!pcm) {
                    pv_status_t status = PV_STATUS_OUT_OF_MEMORY;
                    log_message("handle_client: Failed to allocate memory for PCM data.");
                    if (send_data(client_fd, &status, sizeof(status)) < 0) {
                        log_message("handle_client: Failed to send PV_STATUS_OUT_OF_MEMORY.");
                        break;
                    }
                    break;
                }
                if (receive_data(client_fd, pcm, sizeof(int16_t) * frame_length) < 0) {
                    log_message("handle_client: Failed to receive PCM data.");
                    free(pcm);
                    break;
                }
                log_message("handle_client: Received PCM data for multiple keywords processing.");

                int keyword_index;
                pv_status_t status = pv_porcupine_multiple_keywords_process(object, pcm, &keyword_index);
                free(pcm);

                log_message("handle_client: pv_porcupine_multiple_keywords_process returned status: %d, keyword_index: %d", status, keyword_index);

                if (send_data(client_fd, &status, sizeof(status)) < 0) {
                    log_message("handle_client: Failed to send process status.");
                    break;
                }
                if (status == PV_STATUS_SUCCESS) {
                    int32_t index_network = htonl(keyword_index);
                    if (send_data(client_fd, &index_network, sizeof(index_network)) < 0) {
                        log_message("handle_client: Failed to send keyword_index.");
                        break;
                    }
                    log_message("handle_client: Sent keyword_index: %d", keyword_index);
                }
                break;
            }

            case MSG_VERSION: {
                log_message("handle_client: Handling MSG_VERSION");
                const char *version = pv_porcupine_version();
                if (!version) {
                    log_message("handle_client: pv_porcupine_version returned NULL.");
                    uint32_t len_network = htonl(0);
                    send_data(client_fd, &len_network, sizeof(len_network));
                    break;
                }
                uint32_t len = strlen(version) + 1;
                uint32_t len_network = htonl(len);
                log_message("handle_client: Sending version length: %u", len);
                if (send_data(client_fd, &len_network, sizeof(len_network)) < 0) {
                    log_message("handle_client: Failed to send version length.");
                    break;
                }
                if (send_data(client_fd, version, len) < 0) {
                    log_message("handle_client: Failed to send version string.");
                    break;
                }
                log_message("handle_client: Sent version string: %s", version);
                break;
            }

            case MSG_FRAME_LENGTH: {
                log_message("handle_client: Handling MSG_FRAME_LENGTH");
                int frame_length = pv_porcupine_frame_length();
                log_message("handle_client: frame_length: %d", frame_length);
                if (send_data(client_fd, &frame_length, sizeof(frame_length)) < 0) {
                    log_message("handle_client: Failed to send frame_length.");
                    break;
                }
                log_message("handle_client: Sent frame_length: %d", frame_length);
                break;
            }

            case MSG_SAMPLE_RATE: {
                log_message("handle_client: Handling MSG_SAMPLE_RATE");
                int sample_rate = pv_sample_rate();
                log_message("handle_client: sample_rate: %d", sample_rate);
                if (send_data(client_fd, &sample_rate, sizeof(sample_rate)) < 0) {
                    log_message("handle_client: Failed to send sample_rate.");
                    break;
                }
                log_message("handle_client: Sent sample_rate: %d", sample_rate);
                break;
            }

            default:
                log_message("handle_client: Unknown message type: %u. Closing connection.", msg_type);
                close(client_fd);
                return;
        }
    }

    log_message("handle_client: Closing client connection.");
    close(client_fd);
}

int main() {
    // Initialize object table
    memset(&object_table, 0, sizeof(object_table));

    // Create Unix domain socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    log_message("Server: Created Unix domain socket.");

    // Remove existing socket file
    unlink(SOCKET_PATH);
    log_message("Server: Unlinked existing socket file (if any).");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        log_message("Server: Failed to bind socket.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    log_message("Server: Bound to socket path: %s", SOCKET_PATH);

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        log_message("Server: Failed to listen on socket.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    log_message("Server: Listening on socket.");

    printf("IPC server listening on %s\n", SOCKET_PATH);

    // Accept the single client
    log_message("Server: Waiting to accept a client...");
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        log_message("Server: Failed to accept client connection.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    log_message("Server: Accepted client connection. Client FD: %d", client_fd);

    handle_client(client_fd);

    close(server_fd);
    unlink(SOCKET_PATH);
    log_message("Server: Closed server socket and unlinked socket file.");
    return 0;
}
