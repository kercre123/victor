#ifndef PV_PORCUPINE_IPC_H
#define PV_PORCUPINE_IPC_H

#include <stdint.h>

#define SOCKET_PATH "/dev/socket/_anim_wakeword_socket_0"

typedef enum {
    MSG_INIT,
    MSG_MULTIPLE_KEYWORDS_INIT,
    MSG_DELETE,
    MSG_PROCESS,
    MSG_MULTIPLE_KEYWORDS_PROCESS,
    MSG_VERSION,
    MSG_FRAME_LENGTH,
    MSG_SAMPLE_RATE
} message_type_t;

#endif // PV_PORCUPINE_IPC_H
