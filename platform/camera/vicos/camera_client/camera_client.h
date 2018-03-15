/**
 * File: camera_client.h
 *
 * Author: Brian Chapados
 * Created: 01/24/2018
 *
 * Description:
 *               API for remote IPC connection to anki camera system daemon
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __platform_hal_camera_client_h__
#define __platform_hal_camera_client_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
  ANKI_CAMERA_STATUS_OFFLINE,
  ANKI_CAMERA_STATUS_IDLE,
  ANKI_CAMERA_STATUS_STARTING,
  ANKI_CAMERA_STATUS_RUNNING,
} anki_camera_status_t;

// BEGIN: shared types
// These types are shared with the server component in the OS.
// Eventually these structs will be available via a header in our
// custom toolchain, or we will move the camera system back into the
// engine instead of using a separate process.

//
// IPC Message Protocol
//
typedef enum {
  ANKI_CAMERA_MSG_C2S_HEARTBEAT,
  ANKI_CAMERA_MSG_C2S_CLIENT_REGISTER,
  ANKI_CAMERA_MSG_C2S_CLIENT_UNREGISTER,
  ANKI_CAMERA_MSG_C2S_START,
  ANKI_CAMERA_MSG_C2S_STOP,
  ANKI_CAMERA_MSG_C2S_PARAMS,
  ANKI_CAMERA_MSG_S2C_STATUS,
  ANKI_CAMERA_MSG_S2C_BUFFER,
  ANKI_CAMERA_MSG_S2C_HEARTBEAT,
} anki_camera_msg_id_t;

#define ANKI_CAMERA_MSG_PAYLOAD_LEN 128

struct anki_camera_msg {
  anki_camera_msg_id_t msg_id;
  uint32_t version;
  uint32_t client_id;
  int fd;
  uint8_t payload[ANKI_CAMERA_MSG_PAYLOAD_LEN];
};

typedef struct {
  uint64_t timestamp;
  uint32_t frame_id;
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_row;
  uint8_t  bits_per_pixel;
  uint8_t  format;
  uint8_t  _reserved[2];
  uint32_t _pad_to_64[8];
  uint8_t  data[0];
} anki_camera_frame_t;

typedef struct {
  uint16_t exposure_ms;
  float gain;
} anki_camera_exposure_t;

typedef struct {
  float r_gain;
  float g_gain;
  float b_gain;
} anki_camera_awb_t;

typedef enum {
  ANKI_CAMERA_MSG_C2S_PARAMS_ID_EXP,
  ANKI_CAMERA_MSG_C2S_PARAMS_ID_AWB,
} anki_camera_params_id_t;

typedef struct {
  anki_camera_params_id_t id;
  uint8_t data[sizeof(((struct anki_camera_msg*)0)->payload) - sizeof(anki_camera_params_id_t)];
} anki_camera_msg_params_payload_t;

// END: shared types


struct anki_camera_handle {
  int client_handle;
  uint32_t current_frame_id;
};

// Initializes the camera & starts thread for communicating with daemon
int camera_init(struct anki_camera_handle** camera);

// Starts capturing frames
// Captured frames are buffered internally and can be accessed by calling
// `camera_frame_acquire`.
int camera_start(struct anki_camera_handle* camera);

// Stops capturing frames
int camera_stop(struct anki_camera_handle* camera);

// De-initializes camera, makes it available to rest of system
int camera_release(struct anki_camera_handle* camera);

// Acquire (lock) the most recent available frame for reading
int camera_frame_acquire(struct anki_camera_handle* camera, anki_camera_frame_t** out_frame);

// Release (unlock) frame to camera system
int camera_frame_release(struct anki_camera_handle* camera, uint32_t frame_id);

int camera_set_exposure(struct anki_camera_handle* camera, uint16_t exposure_ms, float gain);

int camera_set_awb(struct anki_camera_handle* camera, float r_gain, float g_gain, float b_gain);

// Get current status of camera system
anki_camera_status_t camera_status(struct anki_camera_handle* camera);

#ifdef __cplusplus
}
#endif

#endif // __platform_hal_camera_client_h__
