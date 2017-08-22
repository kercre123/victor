
typedef struct cameraobj_t* CameraHandle;


//camera callback: called with captured `image` of `width` by `height` 8bit greyscale pixels
// (called from separate thread)
typedef int(*camera_cb)(const uint8_t* image, int width, int height); 


//Allocates a camera object for you.  call `free(handle)` when shutting down
CameraHandle camera_alloc(void);

//Initializes the camera
int camera_init(CameraHandle camera);

//Starts capturing frames in new thread, sends them to callback `cb`.
int camera_start(CameraHandle camera, camera_cb cb);

//stops capturing frames
int camera_stop(CameraHandle camera);

//de-initializes camera, makes it available to rest of system
int camera_cleanup(CameraHandle camera);



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




