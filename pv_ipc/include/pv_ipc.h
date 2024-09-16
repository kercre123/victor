#ifndef PORCUPINE_IPC_H
#define PORCUPINE_IPC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "picovoice.h"

#define PORCUPINE_SOCKET_PATH "/dev/socket/_anim_pv_wakeword_"

#define PV_PORCUPINE_VERSION "3.0.0"
#define PV_PORCUPINE_FRAME_LENGTH 512
#define MAX_KEYWORDS 256
#define MAX_PATH_LENGTH 256

typedef enum {
    PV_CMD_INIT = 1,
    PV_CMD_PROCESS = 2,
    PV_CMD_DELETE = 3,
    PV_CMD_VERSION = 4,
    PV_CMD_FRAME_LENGTH = 5
} pv_command_t;

typedef struct {
    char access_key[256];
    char model_path[256];
    int32_t num_keywords;
    char keyword_path[MAX_KEYWORDS][MAX_PATH_LENGTH];
    float sensitivities[MAX_KEYWORDS];
} pv_init_request_t;

typedef struct {
    int16_t pcm[PV_PORCUPINE_FRAME_LENGTH];
} pv_process_request_t;

typedef struct {
    pv_status_t status;
    union {
        int keyword_index;
    } data;
} pv_response_t;

int send_all(int sockfd, const void *buffer, size_t length);
int recv_all(int sockfd, void *buffer, size_t length);

#endif // PORCUPINE_IPC_H
