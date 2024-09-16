#include "pv_ipc.h"
#include "pv_porcupine.h"
#include "picovoice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Interface Library State
static int client_fd = -1;
static bool is_connected = false;

int pv_sample_rate(void) {
    return 16000;
}

// Helper: Connect to the server
static pv_status_t connect_to_server() {
    if (is_connected) {
        return PV_STATUS_SUCCESS;
    }

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        return PV_STATUS_IO_ERROR;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, PORCUPINE_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(client_fd);
        client_fd = -1;
        return PV_STATUS_IO_ERROR;
    }

    is_connected = true;
    return PV_STATUS_SUCCESS;
}

// Helper: Disconnect from the server
static void disconnect_from_server() {
    if (is_connected) {
        close(client_fd);
        client_fd = -1;
        is_connected = false;
    }
}

// Implementation of send_all and recv_all
static int send_all_lib(int sockfd, const void *buffer, size_t length) {
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

static int recv_all_lib(int sockfd, void *buffer, size_t length) {
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

// Implement API Functions

pv_status_t pv_porcupine_init(
        const char *model_file_path,
        const char *keyword_file_path,
        float sensitivity,
        pv_porcupine_object_t **object) {
    (void)object; // Unused in IPC

    pv_status_t status = connect_to_server();
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    // Prepare and send INIT command
    pv_command_t cmd = PV_CMD_INIT;
    if (send_all_lib(client_fd, &cmd, sizeof(cmd)) == -1) {
        perror("send INIT cmd");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Prepare init request
    pv_init_request_t init_req;
    memset(&init_req, 0, sizeof(init_req));
    strncpy(init_req.model_file_path, model_file_path, sizeof(init_req.model_file_path) - 1);
    strncpy(init_req.keyword_file_path, keyword_file_path, sizeof(init_req.keyword_file_path) - 1);
    init_req.sensitivity = sensitivity;

    if (send_all_lib(client_fd, &init_req, sizeof(init_req)) == -1) {
        perror("send INIT request");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Receive response
    pv_response_t response;
    if (recv_all_lib(client_fd, &response, sizeof(response)) == -1) {
        perror("recv INIT response");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    return response.status;
}

pv_status_t pv_porcupine_multiple_keywords_init(
        const char *model_file_path,
        int number_keywords,
        const char * const *keyword_file_paths,
        const float *sensitivities,
        pv_porcupine_object_t **object) {
    (void)object; // Unused in IPC

    pv_status_t status = connect_to_server();
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    // Prepare and send MULTIPLE_INIT command
    pv_command_t cmd = PV_CMD_MULTIPLE_INIT;
    if (send_all_lib(client_fd, &cmd, sizeof(cmd)) == -1) {
        perror("send MULTIPLE_INIT cmd");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Prepare multiple init request
    pv_multiple_init_request_t mult_init_req;
    memset(&mult_init_req, 0, sizeof(mult_init_req));
    strncpy(mult_init_req.model_file_path, model_file_path, sizeof(mult_init_req.model_file_path) - 1);
    mult_init_req.number_keywords = number_keywords;
    for (int i = 0; i < number_keywords && i < MAX_KEYWORDS; ++i) {
        strncpy(mult_init_req.keyword_file_paths[i], keyword_file_paths[i], sizeof(mult_init_req.keyword_file_paths[i]) - 1);
        mult_init_req.sensitivities[i] = sensitivities[i];
    }

    if (send_all_lib(client_fd, &mult_init_req, sizeof(mult_init_req)) == -1) {
        perror("send MULTIPLE_INIT request");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Receive response
    pv_response_t response;
    if (recv_all_lib(client_fd, &response, sizeof(response)) == -1) {
        perror("recv MULTIPLE_INIT response");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    return response.status;
}

void pv_porcupine_delete(pv_porcupine_object_t *object) {
    (void)object; // Unused in IPC

    if (!is_connected) {
        return;
    }

    // Send DELETE command
    pv_command_t cmd = PV_CMD_DELETE;
    if (send_all_lib(client_fd, &cmd, sizeof(cmd)) == -1) {
        perror("send DELETE cmd");
        disconnect_from_server();
        return;
    }

    // Receive response
    pv_response_t response;
    if (recv_all_lib(client_fd, &response, sizeof(response)) == -1) {
        perror("recv DELETE response");
        disconnect_from_server();
        return;
    }

    // Disconnect after delete
    disconnect_from_server();
}

pv_status_t pv_porcupine_process(pv_porcupine_object_t *object, const int16_t *pcm, bool *result) {
    (void)object; // Unused in IPC

    if (!is_connected) {
        return PV_STATUS_INVALID_ARGUMENT;
    }

    // Send PROCESS command
    pv_command_t cmd = PV_CMD_PROCESS;
    if (send_all_lib(client_fd, &cmd, sizeof(cmd)) == -1) {
        perror("send PROCESS cmd");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Prepare process request
    pv_process_request_t proc_req;
    memcpy(proc_req.pcm, pcm, sizeof(proc_req.pcm));

    if (send_all_lib(client_fd, &proc_req, sizeof(proc_req)) == -1) {
        perror("send PROCESS request");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Receive response
    pv_response_t response;
    if (recv_all_lib(client_fd, &response, sizeof(response)) == -1) {
        perror("recv PROCESS response");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    if (response.status == PV_STATUS_SUCCESS && result != NULL) {
        *result = response.data.result;
    }

    return response.status;
}

pv_status_t pv_porcupine_multiple_keywords_process(
        pv_porcupine_object_t *object,
        const int16_t *pcm,
        int *keyword_index) {
    (void)object; // Unused in IPC

    if (!is_connected) {
        return PV_STATUS_INVALID_ARGUMENT;
    }

    // Send MULTIPLE_PROCESS command
    pv_command_t cmd = PV_CMD_MULTIPLE_PROCESS;
    if (send_all_lib(client_fd, &cmd, sizeof(cmd)) == -1) {
        perror("send MULTIPLE_PROCESS cmd");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Prepare process request
    pv_multiple_process_request_t mult_proc_req;
    memcpy(mult_proc_req.pcm, pcm, sizeof(mult_proc_req.pcm));

    if (send_all_lib(client_fd, &mult_proc_req, sizeof(mult_proc_req)) == -1) {
        perror("send MULTIPLE_PROCESS request");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    // Receive response
    pv_response_t response;
    if (recv_all_lib(client_fd, &response, sizeof(response)) == -1) {
        perror("recv MULTIPLE_PROCESS response");
        disconnect_from_server();
        return PV_STATUS_IO_ERROR;
    }

    if (response.status == PV_STATUS_SUCCESS && keyword_index != NULL) {
        *keyword_index = response.data.keyword_index;
    }

    return response.status;
}

const char *pv_porcupine_version(void) {
    return PV_PORCUPINE_VERSION;
}

int pv_porcupine_frame_length(void) {
    return PV_PORCUPINE_FRAME_LENGTH;
}
