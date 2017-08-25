

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

    CameraHandle camera  = camera_alloc();
    camera_init(camera);
    camera_start(camera, my_camera_callback);

    sleep(2.0); //cap 2 seconds of video

    camera_stop(camera);
    camera_cleanup(camera);
    free(camera);
    
 -----------------------
*/




