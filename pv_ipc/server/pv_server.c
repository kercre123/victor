#include "pv_ipc.h"
#include "pv_porcupine.h"
#include "picovoice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Maximum number of keywords supported
#define MAX_KEYWORDS 10

// Server's Porcupine Object
static pv_porcupine_object_t *porcupine_obj = NULL;

// Initialize Porcupine with single keyword
pv_status_t handle_init(pv_init_request_t *req) {
    if (porcupine_obj != NULL) {
        return PV_STATUS_INVALID_ARGUMENT; // Already initialized
    }
    return pv_porcupine_init(req->model_file_path, req->keyword_file_path, req->sensitivity, &porcupine_obj);
}

// Initialize Porcupine with multiple keywords
pv_status_t handle_multiple_init(pv_multiple_init_request_t *req) {
    if (porcupine_obj != NULL) {
        return PV_STATUS_INVALID_ARGUMENT; // Already initialized
    }
    const char *keywords[MAX_KEYWORDS];
    float sensitivities[MAX_KEYWORDS];
    if (req->number_keywords > MAX_KEYWORDS) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    for (int i = 0; i < req->number_keywords; ++i) {
        keywords[i] = req->keyword_file_paths[i];
        sensitivities[i] = req->sensitivities[i];
    }
    return pv_porcupine_multiple_keywords_init(req->model_file_path, req->number_keywords, keywords, sensitivities, &porcupine_obj);
}

// Process a single frame
pv_status_t handle_process(pv_process_request_t *req, bool *result) {
    if (porcupine_obj == NULL) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    return pv_porcupine_process(porcupine_obj, req->pcm, result);
}

// Process multiple keywords
pv_status_t handle_multiple_process(pv_multiple_process_request_t *req, int *keyword_index) {
    if (porcupine_obj == NULL) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    return pv_porcupine_multiple_keywords_process(porcupine_obj, req->pcm, keyword_index);
}

// Delete Porcupine object
void handle_delete() {
    if (porcupine_obj != NULL) {
        pv_porcupine_delete(porcupine_obj);
        porcupine_obj = NULL;
    }
}

// Utility: Send all data
int send_all(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const uint8_t *buf = buffer;
    while (total_sent < length) {
        ssize_t sent = send(sockfd, buf + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return -1; // Error
        }
        total_sent += sent;
    }
    return 0;
}

// Utility: Receive all data
int recv_all(int sockfd, void *buffer, size_t length) {
    size_t total_recv = 0;
    uint8_t *buf = buffer;
    while (total_recv < length) {
        ssize_t recvd = recv(sockfd, buf + total_recv, length - total_recv, 0);
        if (recvd <= 0) {
            return -1; // Error or connection closed
        }
        total_recv += recvd;
    }
    return 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    // Create UNIX Domain Socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Remove any existing socket
    unlink(PORCUPINE_SOCKET_PATH);

    // Setup address
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, PORCUPINE_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Porcupine Server is listening at %s\n", PORCUPINE_SOCKET_PATH);

    // Accept a single client
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected.\n");

    // Main loop
    while (1) {
        pv_command_t cmd;
        // Receive command
        if (recv_all(client_fd, &cmd, sizeof(pv_command_t)) == -1) {
            perror("recv command");
            break;
        }

        pv_response_t response;
        memset(&response, 0, sizeof(response));

        switch (cmd) {
            case PV_CMD_INIT: {
                pv_init_request_t init_req;
                if (recv_all(client_fd, &init_req, sizeof(init_req)) == -1) {
                    perror("recv init request");
                    break;
                }
                response.status = handle_init(&init_req);
                break;
            }
            case PV_CMD_MULTIPLE_INIT: {
                pv_multiple_init_request_t mult_init_req;
                if (recv_all(client_fd, &mult_init_req, sizeof(mult_init_req)) == -1) {
                    perror("recv multiple init request");
                    break;
                }
                response.status = handle_multiple_init(&mult_init_req);
                break;
            }
            case PV_CMD_PROCESS: {
                pv_process_request_t proc_req;
                if (recv_all(client_fd, &proc_req, sizeof(proc_req)) == -1) {
                    perror("recv process request");
                    break;
                }
                response.status = handle_process(&proc_req, &response.data.result);
                break;
            }
            case PV_CMD_MULTIPLE_PROCESS: {
                pv_multiple_process_request_t mult_proc_req;
                if (recv_all(client_fd, &mult_proc_req, sizeof(mult_proc_req)) == -1) {
                    perror("recv multiple process request");
                    break;
                }
                response.status = handle_multiple_process(&mult_proc_req, &response.data.keyword_index);
                break;
            }
            case PV_CMD_DELETE: {
                handle_delete();
                response.status = PV_STATUS_SUCCESS;
                break;
            }
            case PV_CMD_VERSION: {
                // No action needed; version is fixed
                response.status = PV_STATUS_SUCCESS;
                break;
            }
            case PV_CMD_FRAME_LENGTH: {
                // No action needed; frame_length is fixed
                response.status = PV_STATUS_SUCCESS;
                break;
            }
            default:
                response.status = PV_STATUS_INVALID_ARGUMENT;
                break;
        }

        // Send response
        if (send_all(client_fd, &response, sizeof(response)) == -1) {
            perror("send response");
            break;
        }

        // Optionally, handle server shutdown commands here
    }

    // Cleanup
    handle_delete();
    close(client_fd);
    close(server_fd);
    unlink(PORCUPINE_SOCKET_PATH);

    return 0;
}
