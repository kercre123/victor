#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "victor_camera.h"


#define TIME_TO_RUN 2.0
#define NEVER_QUIT 1

static int  s_dump_images = 0;
static char s_output_path[256] = "/data/misc/camera/test";

int my_camera_callback(uint8_t *frame, int width, int height)
{
  if (s_dump_images) {
    char file_name[64];
    static int frame_idx = 0;
    const char* name = "main";
    const char* ext = "gray";
    int file_fd;

    snprintf(file_name, sizeof(file_name), "%s/%s_%04d.%s",
                                           s_output_path, name, frame_idx++, ext);
    file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
    if (file_fd < 0) {
      printf("%s: cannot open file %s \n", __func__, file_name);
    } else {
      write(file_fd, frame, width * height);
    }

    close(file_fd);
    printf("dump %s", file_name);
  }
  return 0;
}

void
usage( char * name )
{
  printf("usage: %s\n", name);
  printf("   --output,-o    output directory for image dumps\n");
  printf("   --dump,-d      dump processed image received from camera system\n");
  printf("   --help,-h      this helpful message\n");
}

int main(int argc, char* argv[])
{
  char* outdir = NULL;
  int c;
  int optidx = 0;

  struct option longopt[] = {
    {"output",1,NULL,'o'},
    {"dump",0,NULL,'h'},
    {"help",0,NULL,'h'},
    {0,0,0,0}
  };

  while ((c = getopt_long(argc,argv,"o:dh",longopt,&optidx)) != -1) {
      switch ( c )
      {
        case 'o':
          outdir = strdup( optarg );
          break;
        case 'd':
          s_dump_images = 1;
          break;
        case 'h':
          usage(argv[0]);
          return 0;
          break;
        default:
          printf("bad arg\n");
          usage(argv[0]);
          return 1;
      }
  }

  if (outdir) {
    memset(s_output_path, 0, sizeof(s_output_path));
    strncpy(s_output_path, outdir, sizeof(s_output_path));
  }

  int rc = 0;

  printf("Camera Capturer\n");

  if (camera_init() != 0)
  {
    printf("Error opening camera\n");
  } else {
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



