#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "victor_camera.h"


#define DO_FILE_WRITE 1
#define TIME_TO_RUN 2.0
#define NEVER_QUIT 1


int my_camera_callback(const uint8_t *frame, int width, int height)
{
#if DO_FILE_WRITE
  char file_name[64];
  static int frame_idx = 0;
  const char* name = "main";
  const char* ext = "gray";
  int file_fd;

  snprintf(file_name, sizeof(file_name), "/data/misc/camera/test/%s_%04d.%s", name, frame_idx++, ext);
  file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
  if (file_fd < 0) {
     printf("%s: cannot open file %s \n", __func__, file_name);
  } else {
    write(file_fd,
          frame,
          width * height);
  }

  close(file_fd);
  printf("dump %s", file_name);
#endif  
  return 0;
}




int main()
{
    int rc = 0;

    printf("Camera Capturer\n");

    if (camera_init() != 0)
    {
        printf("Error opening camera\n");
    }
    else {

        printf("Starting\n");
        camera_start(my_camera_callback);

        printf("Sleeping\n");
        do {
           sleep(TIME_TO_RUN);
        } while (NEVER_QUIT);
        printf("Done Sleeping\n");

        camera_stop();


    }
    camera_cleanup();
    printf("Exiting application\n");

    return rc;
}



