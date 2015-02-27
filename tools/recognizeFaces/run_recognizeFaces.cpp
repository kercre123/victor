#ifndef ROBOT_HARDWARE

#include "recognizeFaces.h"

#include "opencv2/highgui/highgui.hpp"

using namespace Anki;

std::vector<std::vector<cv::Mat_<u8> > > GetTrainingImages()
{
  // Change this to your directory
  const char *baseDirectory = "/Users/pbarnum/Documents/Anki/products-cozmo-large-files/faceRecognition/ankiPeople/";
  
  const s32 numImages = 6;
  
  const char *imageFilenames[numImages] = {
    "PeterBarnum0.png",
    "PeterBarnum1.png",
    "PeterBarnum2.png",
    "KevinYoon0.png",
    "KevinYoon1.png",
    "KevinYoon2.png",};
  
  const s32 imageIds[numImages] = {
    0,
    0,
    0,
    1,
    1,
    1};
  
  s32 maxId = 0;
  for(s32 i=0; i<numImages; i++) {
    maxId = MAX(maxId, imageIds[i]);
  }
  
  std::vector<std::vector<cv::Mat_<u8> > > images;
  
  images.resize(maxId+1);
  
  for(s32 i=0; i<numImages; i++) {
    char filename[1024];
    snprintf(filename, 1024, "%s/%s", baseDirectory, imageFilenames[i]);
    
    cv::Mat newImage = cv::imread(filename);
    if(newImage.rows == 0) {
      printf("Error: could not load %s\n", filename);
      images.clear();
      return images;
    }
    
    cv::Mat_<u8> newImageGray;
    
    cv::cvtColor(newImage, newImageGray, CV_BGR2GRAY);
    
    images[imageIds[i]].push_back(newImageGray);
  }
  
  return images;
} // GetTrainingImages()

int main(int argc, char ** argv)
{
  int cameraId = 0;
  cv::Size2i imageSize(640,480);

  if(argc == 1) {
    printf("Running with default parameters %d (%d,%d)\n", cameraId, imageSize.width, imageSize.height);
  } else if(argc >= 4 && argc <= 4) {
    char * pEnd;

    cameraId = strtol(argv[1], &pEnd, 10);
    imageSize.width = strtol(argv[2], &pEnd, 10);
    imageSize.height = strtol(argv[3], &pEnd, 10);
  } else {
    printf("Usage:\n"
      "run_recognizeFaces cameraId imageWidth imageHeight\n"
      "Examples:\n"
      "run_captureImages 1 640 480\n");
    return -1;
  }

  std::vector<std::vector<cv::Mat_<u8> > > trainingImages = GetTrainingImages();
    
  for(s32 iPerson=0; iPerson<trainingImages.size(); iPerson++) {
    for(s32 iImage=0; iImage<trainingImages.size(); iImage++) {
      const Result addFaceResult = AddFaceToDatabase(trainingImages[iPerson][iImage], iPerson);
      
      if(addFaceResult != RESULT_OK) {
        printf("Error: training problem\n");
        return -1;
      }
    }
  } // for(s32 iPerson=0; iPerson<trainingImages.size(); iPerson++)

  cv::VideoCapture capture(cameraId);
  
  if (!capture.isOpened()) {
    printf("Error: Capture cannot be opened\n");
    return -1;
  }
  
  capture.set(CV_CAP_PROP_FRAME_WIDTH, imageSize.width);
  capture.set(CV_CAP_PROP_FRAME_HEIGHT, imageSize.height);

  cv::Mat lastImage;
  cv::Mat_<u8> lastImageGray;

  // Capture a few frames to get things started
  for(int i=0; i<5; i++) {
    if(!capture.read(lastImage)) {
      printf("Error: Could not read from camera.\n");
      return -1;
    }
  }
  
  cv::namedWindow("Camera Feed", CV_WINDOW_AUTOSIZE);
  
  bool isRunning = true;
  while(isRunning) {
    if(!capture.read(lastImage)) {
      printf("Error: Could not read from camera.\n");
      return -1;
    }
    
    cv::cvtColor(lastImage, lastImageGray, CV_BGR2GRAY);
    
    std::vector<Face> faces;
    const Result recognizeResult = RecognizeFaces(lastImageGray, faces);

    cv::Mat toShowImage = lastImage.clone();
    
    for(size_t iFace=0; iFace<faces.size(); iFace++) {
      cv::rectangle(
        toShowImage,
        cv::Point(faces[iFace].xc - faces[iFace].w/2, faces[iFace].yc - faces[iFace].w/2),
        cv::Point(faces[iFace].xc + faces[iFace].w/2, faces[iFace].yc + faces[iFace].w/2),
        cv::Scalar(0,255,0),
        5);
    }

    cv::imshow("Camera Feed", toShowImage);
    const int pressedKey = cv::waitKey(10);
    
    if((pressedKey & 0xFF) == 'h') {
      printf("Hold 'q' to quit.\n");
    } else if((pressedKey & 0xFF) == 'q') {
      isRunning = false;
      continue;
    }
  } // while(isRunning)

  //cv::Mat image(imageSize.height, imageSize.width, CV_8UC3);
  //

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
