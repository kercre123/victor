#ifndef ROBOT_HARDWARE

#include "captureImages.h"

#include "opencv2/highgui/highgui.hpp"

#ifndef snprintf
#define snprintf sprintf_s
#endif

int main(int argc, char ** argv)
{
  int cameraId = 1;
  int numImages = 100;
  cv::Size2i imageSize(640,480);

  const char * filenamePattern = "/Users/pbarnum/tmp/images_%05d.png";

  bool startCaptureImmediately = false;
  bool showPreview = true;
  bool showCrosshair = true;

  if(argc == 1) {
    printf("Running with default parameters %d %d (%d,%d) %s %d %d %d\n", cameraId, numImages, imageSize.width, imageSize.height, filenamePattern, startCaptureImmediately, showPreview, showCrosshair);
  } else if(argc >= 6 && argc <= 9) {
    char * pEnd;

    cameraId = strtol(argv[1], &pEnd, 10);
    numImages = strtol(argv[2], &pEnd, 10);
    imageSize.width = strtol(argv[3], &pEnd, 10);
    imageSize.height = strtol(argv[4], &pEnd, 10);
    filenamePattern = argv[5];

    if(argc >= 7)
      startCaptureImmediately = static_cast<bool>(strtol(argv[6], &pEnd, 10));

    if(argc >= 8)
      showPreview = static_cast<bool>(strtol(argv[7], &pEnd, 10));

    if(argc >= 9)
      showCrosshair = static_cast<bool>(strtol(argv[8], &pEnd, 10));
  } else {
    printf("Usage:\n"
      "run_captureImages cameraId numImages imageWidth imageHeight filenamePattern <startCaptureImmediately> <showPreview> <showCrosshair>\n"
      "Examples:\n"
      "run_captureImages 1 100 640 480 \"/Users/pbarnum/tmp/images_%%05d.png\"\n"
      "run_captureImages 1 100 640 480 \"/Users/pbarnum/tmp/images_%%05d.png\" 0 1 1\n");
    return -1;
  }

  std::vector<cv::Mat> capturedImages;
  double captureTime;

  // captureTime is the time right between right before the first frame to right after the last frame
  const int result = Anki::CaptureImages(cameraId, numImages, imageSize, capturedImages, captureTime, startCaptureImmediately, showPreview, showCrosshair);

  printf("Captured %d images at %0.2f FPS. Saving...\n", static_cast<int>(capturedImages.size()), 1.0 / (captureTime / capturedImages.size()));

  char filename[1024];
  for(int i=0; i<static_cast<int>(capturedImages.size()); i++) {
    snprintf(filename, 1024, filenamePattern, i);
    cv::imwrite(filename, capturedImages[i]);
  }

  printf("Saving done.\n");

  return result;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
