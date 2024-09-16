#ifndef PORCUPINE_IPC_H
#define PORCUPINE_IPC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "picovoice.h"

#define PORCUPINE_SOCKET_PATH "/dev/socket/_anim_pv_wakeword_"

#define PV_PORCUPINE_VERSION "1.3.0"
#define PV_PORCUPINE_FRAME_LENGTH 512

typedef enum {
    PV_CMD_INIT = 1,
    PV_CMD_MULTIPLE_INIT = 2,
    PV_CMD_PROCESS = 3,
    PV_CMD_MULTIPLE_PROCESS = 4,
    PV_CMD_DELETE = 5,
    PV_CMD_VERSION = 6,
    PV_CMD_FRAME_LENGTH = 7
} pv_command_t;

typedef struct {
    char model_file_path[256];
    char keyword_file_path[256];
    float sensitivity;
} pv_init_request_t;

typedef struct {
    int16_t pcm[PV_PORCUPINE_FRAME_LENGTH];
} pv_process_request_t;

typedef struct {
    pv_status_t status;
    union {
        bool result;
        int keyword_index;
    } data;
} pv_response_t;

int send_all(int sockfd, const void *buffer, size_t length);
int recv_all(int sockfd, void *buffer, size_t length);

#endif // PORCUPINE_IPC_H
