#ifndef PORCUPINE_IPC_H
#define PORCUPINE_IPC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "picovoice.h"  // Include picovoice.h to get pv_status_t definition

// Define UNIX Domain Socket Path
#define PORCUPINE_SOCKET_PATH "/tmp/porcupine_socket"

// Define Protocol Constants
#define PV_PORCUPINE_VERSION "1.3.0"
#define PV_PORCUPINE_FRAME_LENGTH 512
#define MAX_KEYWORDS 10  // Move this here to make it available in both files

// Define Message Types
typedef enum {
    PV_CMD_INIT = 1,
    PV_CMD_MULTIPLE_INIT = 2,
    PV_CMD_PROCESS = 3,
    PV_CMD_MULTIPLE_PROCESS = 4,
    PV_CMD_DELETE = 5,
    PV_CMD_VERSION = 6,
    PV_CMD_FRAME_LENGTH = 7
} pv_command_t;

// Define Initialization Request
typedef struct {
    char model_file_path[256];
    char keyword_file_path[256];
    float sensitivity;
} pv_init_request_t;

// Define Multiple Initialization Request
typedef struct {
    char model_file_path[256];
    int number_keywords;
    char keyword_file_paths[MAX_KEYWORDS][256]; // Use MAX_KEYWORDS here
    float sensitivities[MAX_KEYWORDS];
} pv_multiple_init_request_t;

// Define Process Request
typedef struct {
    int16_t pcm[PV_PORCUPINE_FRAME_LENGTH];
} pv_process_request_t;

// Define Process Multiple Keywords Request
typedef struct {
    int16_t pcm[PV_PORCUPINE_FRAME_LENGTH];
} pv_multiple_process_request_t;

// Define Response Structure
typedef struct {
    pv_status_t status;
    union {
        bool result;        // For PROCESS
        int keyword_index;  // For MULTIPLE_PROCESS
    } data;
} pv_response_t;

// Utility Functions (to be implemented)
int send_all(int sockfd, const void *buffer, size_t length);
int recv_all(int sockfd, void *buffer, size_t length);

#endif // PORCUPINE_IPC_H
