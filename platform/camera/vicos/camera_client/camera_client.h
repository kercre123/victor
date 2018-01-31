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

typedef struct {
  uint64_t timestamp;
  uint32_t frame_id;
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_row;
  uint8_t  bits_per_pixel;
  uint8_t  format;
  uint8_t  _reserved[2];
  uint8_t  data[0];
} anki_camera_frame_t;

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

// Get current status of camera system
anki_camera_status_t camera_status(struct anki_camera_handle* camera);

#ifdef __cplusplus
}
#endif

#endif // __platform_hal_camera_client_h__
