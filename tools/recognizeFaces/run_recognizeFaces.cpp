#ifndef ROBOT_HARDWARE

#include "recognizeFaces.h"

#include "opencv2/highgui/highgui.hpp"

using namespace Anki;

/*std::vector<std::vector<cv::Mat_<u8> > > GetTrainingImages()
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
*/

int main(int argc, const char *argv[])
{
  const f64 numSecondsToTrain = 5.0;
  
  int cameraId = 0;
  cv::Size2i imageSize(640,480);
  const char * databaseFilename = "faceDatabase.dat";

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
      "run_recognizeFaces 1 640 480\n");
    return -1;
  }

/*  std::vector<std::vector<cv::Mat_<u8> > > trainingImages = GetTrainingImages();
    
  for(s32 iPerson=0; iPerson<trainingImages.size(); iPerson++) {
    for(s32 iImage=0; iImage<trainingImages.size(); iImage++) {
      const Result addFaceResult = AddFaceToDatabase(trainingImages[iPerson][iImage], iPerson);
      
      if(addFaceResult != RESULT_OK) {
        printf("Error: training problem\n");
        return -1;
      }
    }
  } // for(s32 iPerson=0; iPerson<trainingImages.size(); iPerson++)
  */
  
  LoadDatabase(databaseFilename);

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
  
  bool trainingOn = false;
  f64 trainingStartTime;
  char trainingName[16];
  trainingName[1] = '\0';
  
  bool isRunning = true;
  while(isRunning) {
    if(!capture.read(lastImage)) {
      printf("Error: Could not read from camera.\n");
      return -1;
    }
    
    cv::cvtColor(lastImage, lastImageGray, CV_BGR2GRAY);
    
    std::vector<Face> faces;
    
    Result recognizeResult;
    if(trainingOn) {
      recognizeResult = RecognizeFaces(lastImageGray, faces, &trainingName[0]);
    } else {
      recognizeResult = RecognizeFaces(lastImageGray, faces, NULL);
    }

    cv::Mat toShowImage = lastImage.clone();
    
    for(size_t iFace=0; iFace<faces.size(); iFace++) {
      cv::rectangle(
        toShowImage,
        cv::Point(faces[iFace].xc - faces[iFace].w/2, faces[iFace].yc - faces[iFace].w/2),
        cv::Point(faces[iFace].xc + faces[iFace].w/2, faces[iFace].yc + faces[iFace].w/2),
        cv::Scalar(0,255,0),
        5);
      
      char name[1024];
      snprintf(name, 1024, "%d %s", faces[iFace].faceId, faces[iFace].name.data());
      
      cv::putText(toShowImage, name, cv::Point(faces[iFace].xc, faces[iFace].yc), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0));
    }

    if(ABS(Anki::Embedded::GetTimeF64() - trainingStartTime) > numSecondsToTrain) {
      trainingOn = false;
    }

    if(trainingOn) {
      char name[1024];
      snprintf(name, 1024, "Training %s", trainingName);
      cv::putText(toShowImage, name, cv::Point(30,30), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 255));
    }

    cv::imshow("Camera Feed", toShowImage);
    const int pressedKey = cv::waitKey(10);
    
    if((pressedKey & 0xFF) == 'q') {
      isRunning = false;
      continue;
    } else if((pressedKey & 0xFF) >= 'a' && (pressedKey & 0xFF) <= 'z') {
      trainingStartTime = Anki::Embedded::GetTimeF64();
      trainingOn = true;
      trainingName[0] = pressedKey & 0xFF;
    }

  } // while(isRunning)

  SaveDatabase(databaseFilename);

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
