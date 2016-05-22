#ifndef ROBOT_HARDWARE

#include "recognizeFaces.h"

#include "opencv2/highgui/highgui.hpp"

#include <string.h>

using namespace Anki;

int main(int argc, const char *argv[])
{
  const cv::Size2i toSizeSize(480,270);

  const char * inputFilenamePattern = "/Users/pbarnum/Documents/datasets/cozmoPlaytests/Cozmo - Bianca.MP4";
  const char * outputFilenamePattern = "/tmp/%05d.png";
  s32 firstFileIndex = 8066;
  s32 lastFileIndex = 8100;
  cv::Size2i imageSize(960,540);
  const char * databaseFilename = NULL;
  s32 faceDetectionThreshold = 5;
  bool handleArbitraryRotations = false;

  if(argc >= 9 && argc <= 9) {
    char * pEnd;

    inputFilenamePattern = argv[1];
    outputFilenamePattern = argv[2];
    
    firstFileIndex = strtol(argv[3], &pEnd, 10);
    lastFileIndex = strtol(argv[4], &pEnd, 10);
    
    imageSize.width = strtol(argv[5], &pEnd, 10);
    imageSize.height = strtol(argv[6], &pEnd, 10);
    
    faceDetectionThreshold = strtol(argv[7], &pEnd, 10);
    
    handleArbitraryRotations = static_cast<bool>(strtol(argv[8], &pEnd, 10));
  } else {
    printf("Usage:\n"
      "run_recognizeFacesOffline inputFilenamePattern outputFilenamePattern firstFileIndex lastFileIndex imageWidth imageHeight faceDetectionThreshold handleArbitraryRotations\n"
      "Examples:\n"
      "run_recognizeFacesOffline /Volumes/PeterMac/cozmoPlaytests/Cozmo_Bianca/Cozmo_Bianca_%%SB05d.png /Users/pbarnum/Documents/tmp/cozmoPlaytestsResults/Cozmo_Bianca/Cozmo_Bianca_%%05d.png 1 500 960 540 5 1\n"
      "run_recognizeFacesOffline \"/Users/pbarnum/Documents/datasets/cozmoPlaytests/Cozmo - Bianca.MP4\" /Volumes/PeterMac/cozmoPlaytestsResults/Cozmo_Bianca/Cozmo_Bianca_%%05d.png 1 10000000 960 540 5 0\n");
    
      return -1;
  }
  
  printf("Running with %s %s %d %d (%d,%d) %d %d\n", inputFilenamePattern, outputFilenamePattern, firstFileIndex, lastFileIndex, imageSize.width, imageSize.height, faceDetectionThreshold, handleArbitraryRotations);
  
  bool isVideo = true;
  const char * inputSuffix = inputFilenamePattern + (strlen(inputFilenamePattern)-3);
  if(!strcmp("png", inputSuffix) ||
      !strcmp("pgm", inputSuffix) ||
      !strcmp("jpg", inputSuffix) ||
      !strcmp("PNG", inputSuffix) ||
      !strcmp("PGM", inputSuffix) ||
      !strcmp("JPG", inputSuffix)) {
  
    isVideo = false;
    printf("Input is not a video\n");
  } else {
    printf("Input is a video\n");
  }
  
  LoadDatabase(NULL);

  SetRecognitionParameters(handleArbitraryRotations, imageSize.width, faceDetectionThreshold);

  cv::namedWindow("Image Feed", CV_WINDOW_AUTOSIZE);

  cv::VideoCapture cap;
  
  if(isVideo) {
    cap = cv::VideoCapture(inputFilenamePattern);
  
    if( !cap.isOpened() ) {
      printf("Could not open video %s\n", inputFilenamePattern);
      return -1;
    }
    
    if(!cap.set(CV_CAP_PROP_POS_FRAMES, firstFileIndex)){
      printf("Could not move to frame %d\n", firstFileIndex);
      return -1;
    }
  }
  
  for(s32 iImage=firstFileIndex; iImage<=lastFileIndex; iImage++) {
    cv::Mat lastImageRaw;
    cv::Mat lastImage;
    cv::Mat_<u8> lastImageGray;
  
    char filename[1024];
    snprintf(filename, 1024, inputFilenamePattern, iImage);
    
    printf("%d) ", iImage);
    
    if(isVideo) {
      if(!cap.read(lastImageRaw)) {
        printf("Error: Could not read\n");
        return -1;
      }
    } else {
      printf("%s ", filename);
      
      lastImageRaw = cv::imread(filename);
    }
    
    if(lastImageRaw.cols == 0) {
      printf("Error: Could not read from filename %s\n", filename);
      return -1;
    }
    
    // Resize the entire image
    //cv::resize(lastImageRaw, lastImage, imageSize, 0, 0, cv::INTER_LINEAR);
    
    // Grab the center, then resize
    cv::Mat lastImageClipped = lastImageRaw(cv::Range(0, lastImageRaw.rows/2), cv::Range(lastImageRaw.cols/4, (3*lastImageRaw.cols)/4)).clone();
    cv::resize(lastImageClipped, lastImage, imageSize, 0, 0, cv::INTER_LINEAR);
    
    //printf("%d,%d\n", lastImage.cols, lastImage.rows);
    
    cv::cvtColor(lastImage, lastImageGray, CV_BGR2GRAY);
    
    std::vector<Face> faces;
    
    const f64 t0 = Anki::Embedded::GetTimeF64();
    
    const Result recognizeResult = RecognizeFaces(lastImageGray, faces, NULL);
    
    const f64 t1 = Anki::Embedded::GetTimeF64();
    
    cv::Mat toShowImage = lastImage.clone();
    
    if(faces.size() == 0) {
      printf("%0.3fs\n", t1-t0);
    } else {
      printf("%0.3fs Detected %d faces\n", t1-t0, faces.size());
    }
    
    for(size_t iFace=0; iFace<faces.size(); iFace++) {
      cv::rectangle(
        toShowImage,
        cv::Point(faces[iFace].xc - faces[iFace].w/2, faces[iFace].yc - faces[iFace].w/2),
        cv::Point(faces[iFace].xc + faces[iFace].w/2, faces[iFace].yc + faces[iFace].w/2),
        cv::Scalar(0,255,0),
        5);
      
      char name[1024];
      snprintf(name, 1024, "%d %s (%dpix)", faces[iFace].faceId, faces[iFace].name.data(), faces[iFace].w);
      
      cv::putText(toShowImage, name, cv::Point(faces[iFace].xc, faces[iFace].yc), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 0));
    }

    cv::imshow("Image Feed", toShowImage);
    
    snprintf(filename, 1024, outputFilenamePattern, iImage);
    cv::Mat toShowImageSmall;
    cv::resize(toShowImage, toShowImageSmall, toSizeSize, 0, 0, cv::INTER_LINEAR);
    cv::imwrite(filename, toShowImageSmall);
    
    const int pressedKey = cv::waitKey(10);
    
    if((pressedKey & 0xFF) == 'q') {
      break;
    }
  } // for(s32 iImage=firstFileIndex; iImage<=lastFileIndex; iImage++)

  SaveDatabase(databaseFilename);

  return 0;
} // int main()
#endif // #ifndef ROBOT_HARDWARE
