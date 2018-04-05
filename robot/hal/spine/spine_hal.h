#ifndef SPINE_HAL_H
#define SPINE_HAL_H

#include <stdint.h>

#include "platform/platform.h"


#ifdef __cplusplus
extern "C" {
#endif


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


#define HAL_SERIAL_POLL_INTERVAL_US 200


//opens UART, does setup
SpineErr hal_init(const char* devicename, long baudrate);

//Spins until valid frame of `type` is recieved, or `timeout_ms` elapses. returns whole frame
const void* hal_get_frame(uint16_t type, int32_t timeout_ms);

  //Spins until any valid frame is recieved, or `timeout_ms` elapses. returns whole frame
const void* hal_get_next_frame(int32_t timeout_ms);

//Spins until valid frame of `type` is recieved. returns whole frame
const void* hal_wait_for_frame(uint16_t type);

//Sends given frame
void hal_send_frame(uint16_t type, const void* data, int len);


enum RobotMode { //todo: mode is a dummy value. If ever needed, this should be in clad file.
    RobotMode_IDLE,
    RobotMode_RUN,
};

void hal_set_mode(int new_mode);

#ifdef __cplusplus
}
#endif

#endif//SPINE_HAL_H
