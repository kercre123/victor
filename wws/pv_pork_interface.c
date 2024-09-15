// pv_porcupine_interface.c

#include "pv_porcupine.h"
#include "pv_pork_ipc.h"
#include "picovoice.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdarg.h>
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

// Define pv_porcupine_object structure to hold object_id
struct pv_porcupine_object {
    int object_id;
};

// Store the socket file descriptor
static int sockfd = -1;

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
static int send_data(const void *data, size_t size) {
    if (sockfd == -1) {
        log_message("send_data: Socket is not connected.");
        return -1;
    }
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
static int receive_data(void *data, size_t size) {
    if (sockfd == -1) {
        log_message("receive_data: Socket is not connected.");
        return -1;
    }
    ssize_t total_received = 0;
    char *data_ptr = (char *)data;
    while (total_received < (ssize_t)size) {
        ssize_t received = read(sockfd, data_ptr + total_received, size - total_received);
        if (received < 0) {
            log_message("receive_data: Read error. Error: %s", strerror(errno));
            return -1;
        } else if (received == 0) {
            log_message("receive_data: Connection closed by server.");
            return -1;
        }
        total_received += received;
    }
    log_message("receive_data: Received %zu bytes", size);
    return 0;
}

// Helper function to connect to the IPC server
static int connect_to_server() {
    if (sockfd != -1) {
        // If already connected, return the existing socket descriptor
        log_message("connect_to_server: Already connected with sockfd: %d", sockfd);
        return sockfd;
    }

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_message("connect_to_server: Failed to create socket. Error: %s", strerror(errno));
        return -1;
    }
    log_message("connect_to_server: Created Unix domain socket.");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_message("connect_to_server: Failed to connect to server at %s. Error: %s", SOCKET_PATH, strerror(errno));
        close(sockfd);
        sockfd = -1;
        return -1;
    }
    log_message("connect_to_server: Successfully connected to server at %s", SOCKET_PATH);
    return sockfd;
}

// Implementation of pv_porcupine_init
pv_status_t pv_porcupine_init(
        const char *model_file_path,
        const char *keyword_file_path,
        float sensitivity,
        pv_porcupine_object_t **object) {

    log_message("pv_porcupine_init: Initializing with model_file_path: %s, keyword_file_path: %s, sensitivity: %f", 
                model_file_path, keyword_file_path, sensitivity);

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_init: Failed to connect to server.");
        return PV_STATUS_IO_ERROR;
    }

    // Send MSG_INIT
    uint8_t msg_type = MSG_INIT;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_init: Failed to send MSG_INIT.");
        return PV_STATUS_IO_ERROR;
    }

    // Send model_file_path
    uint32_t len = strlen(model_file_path) + 1;
    uint32_t len_network = htonl(len);
    log_message("pv_porcupine_init: Sending model_file_path length: %u", len);
    if (send_data(&len_network, sizeof(len_network)) < 0 ||
        send_data(model_file_path, len) < 0) {
        log_message("pv_porcupine_init: Failed to send model_file_path.");
        return PV_STATUS_IO_ERROR;
    }

    // Send keyword_file_path
    len = strlen(keyword_file_path) + 1;
    len_network = htonl(len);
    log_message("pv_porcupine_init: Sending keyword_file_path length: %u", len);
    if (send_data(&len_network, sizeof(len_network)) < 0 ||
        send_data(keyword_file_path, len) < 0) {
        log_message("pv_porcupine_init: Failed to send keyword_file_path.");
        return PV_STATUS_IO_ERROR;
    }

    // Send sensitivity
    log_message("pv_porcupine_init: Sending sensitivity: %f", sensitivity);
    if (send_data(&sensitivity, sizeof(sensitivity)) < 0) {
        log_message("pv_porcupine_init: Failed to send sensitivity.");
        return PV_STATUS_IO_ERROR;
    }

    // Receive status code
    pv_status_t status;
    if (receive_data(&status, sizeof(status)) < 0) {
        log_message("pv_porcupine_init: Failed to receive status.");
        return PV_STATUS_IO_ERROR;
    }
    log_message("pv_porcupine_init: Received status: %d", status);

    // If success, receive object ID
    if (status == PV_STATUS_SUCCESS) {
        int object_id;
        if (receive_data(&object_id, sizeof(object_id)) < 0) {
            log_message("pv_porcupine_init: Failed to receive object_id.");
            return PV_STATUS_IO_ERROR;
        }
        log_message("pv_porcupine_init: Received object_id: %d", object_id);

        // Allocate object
        pv_porcupine_object_t *obj = malloc(sizeof(pv_porcupine_object_t));
        if (!obj) {
            log_message("pv_porcupine_init: Failed to allocate memory for pv_porcupine_object_t.");
            return PV_STATUS_OUT_OF_MEMORY;
        }
        obj->object_id = object_id;
        *object = obj;
        log_message("pv_porcupine_init: Successfully initialized object with ID: %d", object_id);
    }

    return status;
}

// Implementation of pv_porcupine_multiple_keywords_init
pv_status_t pv_porcupine_multiple_keywords_init(
        const char *model_file_path,
        int num_keywords,
        const char * const *keyword_file_paths,
        const float *sensitivities,
        pv_porcupine_object_t **object) {

    log_message("pv_porcupine_multiple_keywords_init: Initializing with model_file_path: %s, num_keywords: %d", 
                model_file_path, num_keywords);

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to connect to server.");
        return PV_STATUS_IO_ERROR;
    }

    // Send MSG_MULTIPLE_KEYWORDS_INIT
    uint8_t msg_type = MSG_MULTIPLE_KEYWORDS_INIT;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to send MSG_MULTIPLE_KEYWORDS_INIT.");
        return PV_STATUS_IO_ERROR;
    }

    // Send model_file_path
    uint32_t len = strlen(model_file_path) + 1;
    uint32_t len_network = htonl(len);
    log_message("pv_porcupine_multiple_keywords_init: Sending model_file_path length: %u", len);
    if (send_data(&len_network, sizeof(len_network)) < 0 ||
        send_data(model_file_path, len) < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to send model_file_path.");
        return PV_STATUS_IO_ERROR;
    }

    // Send number of keywords
    int32_t num_kw_network = htonl(num_keywords);
    log_message("pv_porcupine_multiple_keywords_init: Sending num_keywords: %d", num_keywords);
    if (send_data(&num_kw_network, sizeof(num_kw_network)) < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to send num_keywords.");
        return PV_STATUS_IO_ERROR;
    }

    // Send keyword_file_paths
    for (int i = 0; i < num_keywords; i++) {
        len = strlen(keyword_file_paths[i]) + 1;
        len_network = htonl(len);
        log_message("pv_porcupine_multiple_keywords_init: Sending keyword_file_path[%d] length: %u", i, len);
        if (send_data(&len_network, sizeof(len_network)) < 0 ||
            send_data(keyword_file_paths[i], len) < 0) {
            log_message("pv_porcupine_multiple_keywords_init: Failed to send keyword_file_path[%d].", i);
            return PV_STATUS_IO_ERROR;
        }
    }

    // Send sensitivities
    log_message("pv_porcupine_multiple_keywords_init: Sending sensitivities.");
    if (send_data(sensitivities, sizeof(float) * num_keywords) < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to send sensitivities.");
        return PV_STATUS_IO_ERROR;
    }

    // Receive status code
    pv_status_t status;
    if (receive_data(&status, sizeof(status)) < 0) {
        log_message("pv_porcupine_multiple_keywords_init: Failed to receive status.");
        return PV_STATUS_IO_ERROR;
    }
    log_message("pv_porcupine_multiple_keywords_init: Received status: %d", status);

    // If success, receive object ID
    if (status == PV_STATUS_SUCCESS) {
        int object_id;
        if (receive_data(&object_id, sizeof(object_id)) < 0) {
            log_message("pv_porcupine_multiple_keywords_init: Failed to receive object_id.");
            return PV_STATUS_IO_ERROR;
        }
        log_message("pv_porcupine_multiple_keywords_init: Received object_id: %d", object_id);

        // Allocate object
        pv_porcupine_object_t *obj = malloc(sizeof(pv_porcupine_object_t));
        if (!obj) {
            log_message("pv_porcupine_multiple_keywords_init: Failed to allocate memory for pv_porcupine_object_t.");
            return PV_STATUS_OUT_OF_MEMORY;
        }
        obj->object_id = object_id;
        *object = obj;
        log_message("pv_porcupine_multiple_keywords_init: Successfully initialized object with ID: %d", object_id);
    }

    return status;
}

// Implementation of pv_porcupine_process
pv_status_t pv_porcupine_process(pv_porcupine_object_t *object, const int16_t *pcm, bool *result) {
    log_message("pv_porcupine_process: Processing PCM data for object_id: %d", object->object_id);

    if (!object || !pcm || !result) {
        log_message("pv_porcupine_process: Invalid arguments.");
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_process: Failed to connect to server.");
        return PV_STATUS_IO_ERROR;
    }

    uint8_t msg_type = MSG_PROCESS;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_process: Failed to send MSG_PROCESS.");
        return PV_STATUS_IO_ERROR;
    }

    // Send object ID
    int object_id = object->object_id;
    log_message("pv_porcupine_process: Sending object_id: %d", object_id);
    if (send_data(&object_id, sizeof(object_id)) < 0) {
        log_message("pv_porcupine_process: Failed to send object_id.");
        return PV_STATUS_IO_ERROR;
    }

    // Send PCM data
    int frame_length = pv_porcupine_frame_length();
    log_message("pv_porcupine_process: Sending PCM data of length: %d", frame_length);
    if (send_data(pcm, sizeof(int16_t) * frame_length) < 0) {
        log_message("pv_porcupine_process: Failed to send PCM data.");
        return PV_STATUS_IO_ERROR;
    }

    // Receive status code
    pv_status_t status;
    if (receive_data(&status, sizeof(status)) < 0) {
        log_message("pv_porcupine_process: Failed to receive status.");
        return PV_STATUS_IO_ERROR;
    }
    log_message("pv_porcupine_process: Received status: %d", status);

    // Receive result
    if (status == PV_STATUS_SUCCESS) {
        uint8_t res;
        if (receive_data(&res, sizeof(res)) < 0) {
            log_message("pv_porcupine_process: Failed to receive result.");
            return PV_STATUS_IO_ERROR;
        }
        *result = res ? true : false;
        log_message("pv_porcupine_process: Received result: %d", res);
    }

    return status;
}

// Implementation of pv_porcupine_multiple_keywords_process
pv_status_t pv_porcupine_multiple_keywords_process(
        pv_porcupine_object_t *object,
        const int16_t *pcm,
        int *keyword_index) {

    log_message("pv_porcupine_multiple_keywords_process: Processing PCM data for object_id: %d", object->object_id);

    if (!object || !pcm || !keyword_index) {
        log_message("pv_porcupine_multiple_keywords_process: Invalid arguments.");
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_multiple_keywords_process: Failed to connect to server.");
        return PV_STATUS_IO_ERROR;
    }

    uint8_t msg_type = MSG_MULTIPLE_KEYWORDS_PROCESS;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_multiple_keywords_process: Failed to send MSG_MULTIPLE_KEYWORDS_PROCESS.");
        return PV_STATUS_IO_ERROR;
    }

    // Send object ID
    int object_id = object->object_id;
    log_message("pv_porcupine_multiple_keywords_process: Sending object_id: %d", object_id);
    if (send_data(&object_id, sizeof(object_id)) < 0) {
        log_message("pv_porcupine_multiple_keywords_process: Failed to send object_id.");
        return PV_STATUS_IO_ERROR;
    }

    // Send PCM data
    int frame_length = pv_porcupine_frame_length();
    log_message("pv_porcupine_multiple_keywords_process: Sending PCM data of length: %d", frame_length);
    if (send_data(pcm, sizeof(int16_t) * frame_length) < 0) {
        log_message("pv_porcupine_multiple_keywords_process: Failed to send PCM data.");
        return PV_STATUS_IO_ERROR;
    }

    // Receive status code
    pv_status_t status;
    if (receive_data(&status, sizeof(status)) < 0) {
        log_message("pv_porcupine_multiple_keywords_process: Failed to receive status.");
        return PV_STATUS_IO_ERROR;
    }
    log_message("pv_porcupine_multiple_keywords_process: Received status: %d", status);

    // Receive keyword index
    if (status == PV_STATUS_SUCCESS) {
        int32_t index_network;
        if (receive_data(&index_network, sizeof(index_network)) < 0) {
            log_message("pv_porcupine_multiple_keywords_process: Failed to receive keyword_index.");
            return PV_STATUS_IO_ERROR;
        }
        *keyword_index = ntohl(index_network);
        log_message("pv_porcupine_multiple_keywords_process: Received keyword_index: %d", *keyword_index);
    }

    return status;
}

// Implementation of pv_porcupine_version
const char *pv_porcupine_version(void) {
    log_message("pv_porcupine_version: Requesting version from server.");

    static char version[64] = {0};

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_version: Failed to connect to server.");
        return NULL;
    }

    uint8_t msg_type = MSG_VERSION;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_version: Failed to send MSG_VERSION.");
        return NULL;
    }

    uint32_t len_network;
    if (receive_data(&len_network, sizeof(len_network)) < 0) {
        log_message("pv_porcupine_version: Failed to receive version length.");
        return NULL;
    }
    uint32_t len = ntohl(len_network);
    log_message("pv_porcupine_version: Received version length: %u", len);

    if (len >= sizeof(version)) {
        log_message("pv_porcupine_version: Version string too long.");
        return NULL;
    }
    if (receive_data(version, len) < 0) {
        log_message("pv_porcupine_version: Failed to receive version string.");
        return NULL;
    }

    log_message("pv_porcupine_version: Received version string: %s", version);
    return version;
}

// Implementation of pv_porcupine_frame_length
int pv_porcupine_frame_length(void) {
    log_message("pv_porcupine_frame_length: Requesting frame length from server.");

    if (connect_to_server() < 0) {
        log_message("pv_porcupine_frame_length: Failed to connect to server.");
        return -1;
    }

    uint8_t msg_type = MSG_FRAME_LENGTH;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_porcupine_frame_length: Failed to send MSG_FRAME_LENGTH.");
        return -1;
    }

    int frame_length;
    if (receive_data(&frame_length, sizeof(frame_length)) < 0) {
        log_message("pv_porcupine_frame_length: Failed to receive frame_length.");
        return -1;
    }

    log_message("pv_porcupine_frame_length: Received frame_length: %d", frame_length);
    return frame_length;
}

// Implementation of pv_sample_rate (from picovoice.h)
int pv_sample_rate(void) {
    log_message("pv_sample_rate: Requesting sample rate from server.");

    if (connect_to_server() < 0) {
        log_message("pv_sample_rate: Failed to connect to server.");
        return -1;
    }

    uint8_t msg_type = MSG_SAMPLE_RATE;
    if (send_data(&msg_type, sizeof(msg_type)) < 0) {
        log_message("pv_sample_rate: Failed to send MSG_SAMPLE_RATE.");
        return -1;
    }

    int sample_rate;
    if (receive_data(&sample_rate, sizeof(sample_rate)) < 0) {
        log_message("pv_sample_rate: Failed to receive sample_rate.");
        return -1;
    }

    log_message("pv_sample_rate: Received sample_rate: %d", sample_rate);
    return sample_rate;
}

// Implementation of pv_porcupine_delete
void pv_porcupine_delete(pv_porcupine_object_t *object) {
    log_message("pv_porcupine_delete: Deleting object_id: %d", object->object_id);

    if (!object) {
        log_message("pv_porcupine_delete: Received NULL object.");
        return;
    }

    if (sockfd != -1) {
        uint8_t msg_type = MSG_DELETE;
        if (send_data(&msg_type, sizeof(msg_type)) < 0) {
            log_message("pv_porcupine_delete: Failed to send MSG_DELETE.");
            // Proceed to close the socket even if sending fails
        }

        int object_id = object->object_id;
        if (send_data(&object_id, sizeof(object_id)) < 0) {
            log_message("pv_porcupine_delete: Failed to send object_id: %d.", object_id);
            // Proceed to close the socket even if sending fails
        }
        log_message("pv_porcupine_delete: Sent MSG_DELETE for object_id: %d.", object_id);
    }

    // Close the socket and reset sockfd
    if (sockfd != -1) {
        close(sockfd);
        log_message("pv_porcupine_delete: Closed socket with sockfd: %d.", sockfd);
        sockfd = -1;
    }

    free(object);
    log_message("pv_porcupine_delete: Freed pv_porcupine_object_t memory.");
}
