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
  } Face;

  // Finds one face in "image", and adds it to the database
  Result AddFaceToDatabase(const cv::Mat_<u8> &image, const u32 faceId);

  // Find and recognize all faces in "image"
  Result RecognizeFaces(const cv::Mat_<u8> &image, std::vector<Face> &faces);
} // namespace Anki

#endif // #ifndef _RECOGNIZE_FACES_H_
