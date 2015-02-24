#ifndef ROBOT_HARDWARE

#include "captureImages.h"

#include "opencv2/highgui/highgui.hpp"

#ifdef _MSC_VER
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
  bool captureOnce = false;

  if(argc == 1) {
    printf("Running with default parameters %d %d (%d,%d) %s %d %d %d %d\n", cameraId, numImages, imageSize.width, imageSize.height, filenamePattern, startCaptureImmediately, showPreview, showCrosshair, captureOnce);
  } else if(argc >= 6 && argc <= 10) {
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

    if(argc >= 10)
      captureOnce =  static_cast<bool>(strtol(argv[9], &pEnd, 10));
  } else {
    printf("Usage:\n"
      "run_captureImages cameraId numImages imageWidth imageHeight filenamePattern <startCaptureImmediately> <showPreview> <showCrosshair> <captureOnce> \n"
      "Examples:\n"
      "run_captureImages 1 100 640 480 \"/Users/pbarnum/tmp/images_%%05d.png\"\n"
      "run_captureImages 1 100 640 480 \"/Users/pbarnum/tmp/images_%%05d.png\" 0 1 1 1s\n");
    return -1;
  }

  std::vector<cv::Mat> capturedImages;
  double captureTime;
  bool wasQuitPressed = false;
  int captureImagesResult;
  int curFileNumber = 0;
  bool cameraSettingsGui = true;
  while(!wasQuitPressed) {
    // captureTime is the time right between right before the first frame to right after the last frame
    captureImagesResult = Anki::CaptureImages(cameraId, numImages, imageSize, capturedImages, captureTime, wasQuitPressed, startCaptureImmediately, showPreview, showCrosshair, cameraSettingsGui);

    printf("Captured %d images at %0.2f FPS. Saving ", static_cast<int>(capturedImages.size()), 1.0 / (captureTime / capturedImages.size()));

    char filename[1024];
    while(curFileNumber < 1000000) {
      snprintf(filename, 1024, filenamePattern, curFileNumber);

      FILE *file = fopen(filename, "r");

      if(file) {
        fclose(file);
      } else {
        break;
      }

      curFileNumber++;
    }

    if(curFileNumber == 1000000) {
      printf("\ncurFileNumber is too large\n");
      return -10;
    }

    printf("starting at %s...\n", filename);

    for(int i=0; i<static_cast<int>(capturedImages.size()); i++) {
      snprintf(filename, 1024, filenamePattern, curFileNumber);
      cv::imwrite(filename, capturedImages[i]);
      curFileNumber++;
    }

    printf("Saving done.\n");

    if(captureOnce) {
      break;
    }

    cameraSettingsGui = false;
  } // while(!wasQuitPressed)

  return captureImagesResult;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
