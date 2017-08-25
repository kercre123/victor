#ifndef __fugly_camera__victor_camera_h__
#define __fugly_camera__victor_camera_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cameraobj_t* CameraHandle;


//camera callback: called with captured `image` of `width` by `height` 8bit greyscale pixels
// (called from separate thread)
typedef int(*camera_cb)(const uint8_t* image, int width, int height); 


//Allocates a camera object for you.  call `free(handle)` when shutting down
__attribute__((visibility("hidden")))
CameraHandle camera_alloc(void);

//Initializes the camera
__attribute__((visibility("hidden")))
int camera_init(CameraHandle camera);

//Starts capturing frames in new thread, sends them to callback `cb`.
__attribute__((visibility("hidden")))
int camera_start(CameraHandle camera, camera_cb cb);

//stops capturing frames
__attribute__((visibility("hidden")))
int camera_stop(CameraHandle camera);

//de-initializes camera, makes it available to rest of system
__attribute__((visibility("hidden")))
int camera_cleanup(CameraHandle camera);

__attribute__((visibility("hidden")))
int camera_set_exposure(CameraHandle camera, int exp);

__attribute__((visibility("hidden")))
int camera_set_fps(CameraHandle camera, int fps);

#ifdef __cplusplus
}
#endif

#endif // __fugly_camera__victor_camera_h__

/* Usage Example   
--------------------

    CameraHandle camera  = camera_alloc();
    camera_init(camera);
    camera_start(camera, my_camera_callback);

    sleep(2.0); //cap 2 seconds of video

    camera_stop(camera);
    camera_cleanup(camera);
    free(camera);
    
 -----------------------
*/

