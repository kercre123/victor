#ifndef _RECOGNIZE_FACES_H_
#define _RECOGNIZE_FACES_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"

#include "opencv/cv.h"

#include <vector>

namespace Anki
{
  typedef struct {
    s32 xc, yc, w;
    s32 padding;
    f64 angle;
    s64 faceId; // If less than 0, is unknown
    std::string name;
  } Face;


  // handleArbitraryRotations– extends default in-plane face rotation angle from -15..15 degrees to -30..30 degrees.
  // TRUE: extended in-plane rotation support is enabled at the cost of detection speed (3 times performance hit).
  // FALSE: default fast detection -15..15 degrees.
  //
  // internalResizeWidth – controls the detection speed by setting the size of the image the detection functions will work with. Choose higher value to increase detection quality, or lower value to improve the performance.
  //
  // faceDetectionThreshold should be between 1 and 5. 1 is very sensitive and has lots of false positives. 5 detects only face.
  Result SetRecognitionParameters(const bool handleArbitraryRotations, const int internalResizeWidth, const int faceDetectionThreshold);
  
  // Find and recognize all faces in "image"
  // If knownName is not NULL, the largest detected face will be assigned name "knownName"
  Result RecognizeFaces(const cv::Mat_<u8> &image, std::vector<Face> &faces, const char * knownName = NULL);
  
  // Load or save the face recognition database
  Result LoadDatabase(const char * filename);
  Result SaveDatabase(const char * filename);
} // namespace Anki

#endif // #ifndef _RECOGNIZE_FACES_H_
