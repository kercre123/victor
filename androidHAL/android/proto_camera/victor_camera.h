#ifndef VICTOR_CAMERA_H_INCLUDED_
#define VICTOR_CAMERA_H_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif

//camera callback: called with captured `image` of `width` by `height` 8bit greyscale pixels
// (called from separate thread)
typedef int(*camera_cb)(const uint8_t* image, int width, int height); 


//Initializes the camera
int camera_init();

//Starts capturing frames in new thread, sends them to callback `cb`.
int camera_start(camera_cb cb);

//stops capturing frames
int camera_stop();

//de-initializes camera, makes it available to rest of system
int camera_cleanup();



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
