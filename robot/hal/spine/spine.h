#ifndef __robot2_hal_spine_h__
#define __robot2_hal_spine_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "platform/platform.h"
#include "schema/messages.h"

#include <stdint.h>
#include <unistd.h>

typedef enum SpineErr_t {
  err_OK,
  err_BAD_CHECKSUM = 0x8001,
  err_INVALID_FRAME_LEN,
  err_UNKNOWN_MSG,
  err_CANT_OPEN_FILE,
  err_TERMIOS_FAIL,
  err_BAD_ARGUMENT,
  err_ALREADY_OPEN,
  err_FILE_READ_ERROR,
} SpineErr;

struct spine_params {
    char devicename[32];
    uint32_t baudrate;
};

struct spine_frame_b2h {
    struct SpineMessageHeader header;
    struct BodyToHead payload;
    struct SpineMessageFooter footer;
};

struct spine_frame_h2b {
    struct SpineMessageHeader header;
    struct HeadToBody payload;
    struct SpineMessageFooter footer;
};

enum RobotMode { //todo: mode is a dummy value. If ever needed, this should be in clad file.
    RobotMode_IDLE,
    RobotMode_RUN,
};

#define SPINE_BUFFER_MAX_LEN 8192

static const size_t SPINE_B2H_FRAME_LEN =
    sizeof(struct SpineMessageHeader) + sizeof(struct BodyToHead) + sizeof(struct SpineMessageFooter);

static const size_t SPINE_CCC_FRAME_LEN =
    sizeof(struct SpineMessageHeader) + sizeof(struct BodyToHead) + sizeof(struct SpineMessageFooter);

struct spine_ctx {
    int fd;
    int errcount;
    uint32_t rx_cursor;
    uint32_t rx_size;
    uint8_t buf_rx[SPINE_BUFFER_MAX_LEN];
    uint8_t buf_tx[SPINE_BUFFER_MAX_LEN];
};
typedef struct spine_ctx* spine_ctx_t;

void spine_init(spine_ctx_t spine);
void spine_destroy(spine_ctx_t spine);

// open UART, configure I/O
SpineErr spine_open(spine_ctx_t spine, struct spine_params params);
int spine_close(spine_ctx_t spine);

// get file descriptor associated with spine I/O
int spine_get_fd(spine_ctx_t spine);

// read all available data from spine
ssize_t spine_read(spine_ctx_t spine);

// send data into spine for processing
ssize_t spine_receive_data(spine_ctx_t spine, const void* bytes, size_t len);

// write all pending output data
ssize_t spine_write_h2b_frame(spine_ctx_t spine, const struct HeadToBody* h2b_payload);

// write message to set lights
// Only useful if not already sending h2b frames which 
// contains light commands. (i.e. calm mode)
ssize_t spine_set_lights(spine_ctx_t spine, const struct LightState* light_state);

// write message to change mode
ssize_t spine_set_mode(spine_ctx_t spine, int new_mode);

// write shutdown message
ssize_t spine_shutdown(spine_ctx_t spine);

// Attempt to parse and return a frame from the internal buffer.
// On success, returns the length of the complete frame and copies data into outbuf.
// On failure, returns 0 if not enough data has been buffered (partial frame) or
// -1 if buffered data is invalid
ssize_t spine_parse_frame(spine_ctx_t spine, void *out_buf, size_t out_buf_len, size_t* out_len);

//
// spine_protocol parsing
//

// parse buffer into SpineMessageHeader
ssize_t spine_parse_header(const void* inbuf, size_t inbuf_len, struct SpineMessageHeader* out_header);

// parse buffer into b2h data frame
ssize_t spine_parse_b2h_frame(const void* inbuf, size_t inbuf_len, struct spine_frame_b2h* out_frame);

// create H2B frame for sending data to body
ssize_t spine_make_h2b_frame(const struct HeadToBody* h2b_payload, struct spine_frame_h2b* out_frame);

int spine_test_setup();
int spine_test_loop_once();

#ifdef __cplusplus
}
#endif

#endif // __robot2_hal_spine_h__
