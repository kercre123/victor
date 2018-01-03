#ifndef VICTOR_CAMERA_H_INCLUDED_
#define VICTOR_CAMERA_H_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif

// Camera callback: called with captured `image` of `width` by `height` 8bit greyscale pixels
// (called from separate thread)
typedef int(*camera_cb)(uint8_t* image, int width, int height);


// Initializes the camera
int camera_init();

// Starts capturing frames in new thread, sends them to callback `cb`.
int camera_start(camera_cb cb);

// Let Camera know that the latest frame is being processed and that the buffer that holds
// it should not be written to
void camera_swap_locks();

// Stops capturing frames
int camera_stop();

// De-initializes camera, makes it available to rest of system
int camera_cleanup();

// Let the camera know we want a frame
void camera_request_frame();

/* Usage Example   
--------------------

    camera_init();
    camera_start(my_camera_callback);

    sleep(2.0); //cap 2 seconds of video

    camera_stop();
    camera_cleanup();
    
 -----------------------
*/


#ifdef __cplusplus
}
#endif


#endif//VICTOR_CAMERA_H_INCLUDED_
