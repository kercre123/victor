/**
File: recognizeFaces.cpp
Author: Peter Barnum
Created: 2014-10-28

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/recognize.h"

#include "anki/common/robot/hostIntrinsics_m4.h"

#include "anki/vision/robot/histogram.h"

#define ACCELERATION_NONE 0
#define ACCELERATION_ARM_M4 1
#define ACCELERATION_ARM_A7 2

#if defined(__ARM_ARCH_7A__)
#define ACCELERATION_TYPE ACCELERATION_ARM_A7
#else
#define ACCELERATION_TYPE ACCELERATION_ARM_M4
#endif

#if ACCELERATION_TYPE == ACCELERATION_NONE
#warning not using ARM acceleration
#endif

#if ACCELERATION_TYPE == ACCELERATION_ARM_A7
#include <arm_neon.h>
#endif

using namespace Anki;
using namespace Anki::Embedded;

#if ANKICORETECH_EMBEDDED_USE_OPENCV

// Use the metric of Becker2008Evaluation
static f32 EyeQuality_Si(const cv::Rect &face, const cv::Rect &eye, const f32 alpha, const f32 beta_x, const f32 beta_y)
{
  // Face and eye area
  const f32 faceSize = static_cast<f32>(face.width * face.height);
  const f32 eyeSize = static_cast<f32>(eye.width * eye.height);

  // Face center x,y
  const f32 faceX = static_cast<f32>((2*face.x + face.width) / 2);
  const f32 faceY = static_cast<f32>((2*face.y + face.height) / 2);

  const f32 faceRadius = static_cast<f32>(MAX(face.width, face.height) / 2);

  // Eye center x,y
  const f32 eyeX = static_cast<f32>((2*eye.x + eye.width) / 2);
  const f32 eyeY = static_cast<f32>((2*eye.y + eye.height) / 2);

  const f32 part1 = ((2*faceSize) / eyeSize) - alpha;
  const f32 part2 = (ABS(faceX - eyeX) / (0.01f * faceRadius)) - beta_x;
  const f32 part3 = (ABS(faceY - eyeY) / (0.01f * faceRadius)) - beta_y;

  const f32 total = part1*part1 + part2*part2 + part3*part3;

  return total;
} // static f32 EyeQuality_Si(const cv::Rect &face, const cv::Rect &eye, const f32 alpha, const f32 beta_x, const f32 beta_y)

static f32 EyeQuality_Sb(const cv::Rect &face, const cv::Rect &leftEye, const cv::Rect &rightEye, const f32 gamma)
{
  const f32 faceSize = static_cast<f32>(face.width * face.height);
  const f32 leftEyeSize = static_cast<f32>(leftEye.width * leftEye.height);
  const f32 rightEyeSize = static_cast<f32>(rightEye.width * rightEye.height);

  const f32 faceRadius = static_cast<f32>(MAX(face.width, face.height) / 2);

  const f32 leftEyeX = static_cast<f32>((2*leftEye.x + leftEye.width) / 2);
  const f32 leftEyeY = static_cast<f32>((2*leftEye.y + leftEye.height) / 2);
  const f32 rightEyeX = static_cast<f32>((2*rightEye.x + rightEye.width) / 2);
  const f32 rightEyeY = static_cast<f32>((2*rightEye.y + rightEye.height) / 2);

  const f32 eyeDistance = sqrtf((leftEyeX-rightEyeX)*(leftEyeX-rightEyeX) + (leftEyeY-rightEyeY)*(leftEyeY-rightEyeY));

  const f32 part1 = (faceSize / leftEyeSize) - (faceSize / rightEyeSize);
  const f32 part2 = (eyeDistance / (0.01f * faceRadius)) - gamma;

  const f32 total = part1*part1 + part2*part2;;

  return total;
} // static f32 EyeQuality_Sb(const cv::Rect &face, const cv::Rect &leftEye, const cv::Rect &rightEye, const f32 gamma)

// Use the metric of Becker2008Evaluation
static f32 EyeQuality(const cv::Rect &face, const cv::Rect &leftEye, const cv::Rect &rightEye, const f32 alpha, const f32 beta_x, const f32 beta_y, const f32 gamma)
{
  const f32 faceSize = static_cast<f32>(face.width * face.height);
  const f32 leftEyeSize = static_cast<f32>(leftEye.width * leftEye.height);
  const f32 rightEyeSize = static_cast<f32>(rightEye.width * rightEye.height);

  const f32 faceY = static_cast<f32>((2*face.y + face.height) / 2);
  const f32 leftEyeY = static_cast<f32>((2*leftEye.y + leftEye.height) / 2);
  const f32 rightEyeY = static_cast<f32>((2*rightEye.y + rightEye.height) / 2);

  // Extra heuristics

  // Left eye must be left of the right eye
  if((leftEye.x+leftEye.width) > rightEye.x) {
    return 0;
  }

  // Eyes must be above the center
  if(faceY < leftEyeY || faceY < rightEyeY) {
    return 0;
  }

  // The eyes and face shouldn't be too small
  if(leftEyeSize < 100 || rightEyeSize < 100 || faceSize < 100 || (faceSize < leftEyeSize*4) || (faceSize < rightEyeSize*4)) {
    return 0;
  }

  const f32 total =
    EyeQuality_Si(face, leftEye, alpha, beta_x, beta_y) +
    EyeQuality_Si(face, rightEye, alpha, beta_x, beta_y) +
    EyeQuality_Sb(face, leftEye, rightEye, gamma);

  return 1.0f / total;
} // static f32 EyeQuality(const cv::Rect &face, const cv::Rect &leftEye, const cv::Rect &rightEye, const f32 alpha, const f32 beta_x, const f32 beta_y, const f32 gamma)

//def Distance(p1,p2):
//  dx = p2[0] - p1[0]
//  dy = p2[1] - p1[1]
//  return math.sqrt(dx*dx+dy*dy)

//def ScaleRotateTranslate(image, angle, center = None, new_center = None, scale = None, resample=Image.BICUBIC):
//Result ScaleRotateTranslate(const cv::Mat_<u8> &image, const f32 angle, const f32 scaleX, const f32 scaleY, const cv::Point &center, cv::Mat_<u8> &updatedImage)
//{
//  const f32 x = center.x;
//  const f32 y = center.y;
//
//  const f32 cosine = cosf(angle);
//  const f32 sine = sinf(angle);
//
//  const f32 a = cosine/scaleX;
//  const f32 b = sine/scaleX;
//  const f32 c = x - x*a - y*b;
//  const f32 d = -sine/scaleY;
//  const f32 e = cosine/scaleY;
//  const f32 f = y - x*d - y*e;
//
//  cv::Mat_<f32> transformation(1,6);
//
//  transformation.at<f32>(0,0) = a;
//  transformation.at<f32>(0,1) = b;
//  transformation.at<f32>(0,2) = c;
//  transformation.at<f32>(0,3) = d;
//  transformation.at<f32>(0,4) = e;
//  transformation.at<f32>(0,5) = f;
//
//  updatedImage = cv::Mat_<u8>(image.size());
//
//  //  return image.transform(image.size, Image.AFFINE, (a,b,c,d,e,f), resample=resample)
//  cv::transform(image, updatedImage, transformation);
//
//  return RESULT_OK;
//}

//def CropFace(image, eye_left=(0,0), eye_right=(0,0), offset_pct=(0.2,0.2), dest_sz = (70,70)):
//Result CropFace(image, eye_left=(0,0), eye_right=(0,0), offset_pct=(0.2,0.2), dest_sz = (70,70)):
//  # calculate offsets in original image
//  offset_h = math.floor(float(offset_pct[0])*dest_sz[0])
//  offset_v = math.floor(float(offset_pct[1])*dest_sz[1])
//  # get the direction
//  eye_direction = (eye_right[0] - eye_left[0], eye_right[1] - eye_left[1])
//  # calc rotation angle in radians
//  rotation = -math.atan2(float(eye_direction[1]),float(eye_direction[0]))
//  # distance between them
//  dist = Distance(eye_left, eye_right)
//  # calculate the reference eye-width
//  reference = dest_sz[0] - 2.0*offset_h
//  # scale factor
//  scale = float(dist)/float(reference)
//  # rotate original around the left eye
//  image = ScaleRotateTranslate(image, center=eye_left, angle=rotation)
//  # crop the rotated image
//  crop_xy = (eye_left[0] - scale*offset_h, eye_left[1] - scale*offset_v)
//  crop_size = (dest_sz[0]*scale, dest_sz[1]*scale)
//  image = image.crop((int(crop_xy[0]), int(crop_xy[1]), int(crop_xy[0]+crop_size[0]), int(crop_xy[1]+crop_size[1])))
//  # resize it
//  image = image.resize(dest_sz, Image.ANTIALIAS)
//  return image

#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

namespace Anki
{
  namespace Embedded
  {
    namespace Recognize
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      Result RecognizeFace(
        const Array<u8> &image,
        const Rectangle<s32> faceLocation,
        cv::CascadeClassifier &eyeClassifier,
        const f32 eyeQualityThreshold, //< A value between about 1/2000 to 1/5000 is good, where 0.0 is terrible and 1.0 is wonderful
        std::vector<cv::Rect> &detectedEyes,
        s32 &leftEyeIndex,
        s32 &rightEyeIndex,
        f32 &eyeQuality,
        s32 &faceId,
        f64 &confidence)
      {
        detectedEyes.clear();
        leftEyeIndex = -1;
        rightEyeIndex = -1;
        eyeQuality = 0;
        faceId = -1;
        confidence = 0;

        cv::Mat_<u8> imageCv;
        ArrayToCvMat(image, &imageCv);

        cv::Rect faceLocationCv(faceLocation.left, faceLocation.top, faceLocation.get_width(), faceLocation.get_height());
        cv::Mat faceROI = imageCv(faceLocationCv);

        // Preprocess
        cv::equalizeHist(faceROI, faceROI);

        // Detect eyes
        eyeClassifier.detectMultiScale(faceROI, detectedEyes, 1.1, 3, CV_HAAR_SCALE_IMAGE, cv::Size(30,30), cv::Size(faceLocationCv.width/3, faceLocationCv.height/3));

        const s32 numEyes = detectedEyes.size();

        // Offset the eyes by the face
        for(s32 iEye=0; iEye<numEyes; iEye++) {
          detectedEyes[iEye].x += faceLocationCv.x;
          detectedEyes[iEye].y += faceLocationCv.y;
        }

        // Are the eyes in a reasonable size and position? Use the metric of Becker2008Evaluation

        if(numEyes < 2) {
          return RESULT_OK;
        }

        const f32 eyeQuality_alpha = 20;
        const f32 eyeQuality_beta_x = 25;
        const f32 eyeQuality_beta_y = 25;
        const f32 eyeQuality_gamma = 75;

        // Find the best of all detected pairs

        for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++) {
          for(s32 iRightEye=0; iRightEye<numEyes; iRightEye++) {
            if(iLeftEye == iRightEye)
              continue;

            const f32 faceSize = static_cast<f32>(faceLocationCv.width * faceLocationCv.height);
            const f32 leftEyeSize = static_cast<f32>(detectedEyes[iLeftEye].width * detectedEyes[iLeftEye].height);
            const f32 rightEyeSize = static_cast<f32>(detectedEyes[iRightEye].width * detectedEyes[iRightEye].height);

            const f32 faceY = static_cast<f32>((2*faceLocationCv.y + faceLocationCv.height) / 2);
            const f32 leftEyeY = static_cast<f32>((2*detectedEyes[iLeftEye].y + detectedEyes[iLeftEye].height) / 2);
            const f32 rightEyeY = static_cast<f32>((2*detectedEyes[iRightEye].y + detectedEyes[iRightEye].height) / 2);

            const f32 curValue = EyeQuality(faceLocationCv, detectedEyes[iLeftEye], detectedEyes[iRightEye], eyeQuality_alpha, eyeQuality_beta_x, eyeQuality_beta_y, eyeQuality_gamma);

            if(curValue > eyeQuality) {
              eyeQuality = curValue;
              leftEyeIndex = iLeftEye;
              rightEyeIndex = iRightEye;
            }
          }
        } // for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++)

        if(eyeQuality < eyeQualityThreshold) {
          return RESULT_OK;
        }

        // Rotate and scale

        // Mask out

        // Predict the label

        return RESULT_OK;
      } // Result RecognizeFace(const Array<u8> &image, const Rectangle<s32> faceLocation, const cv::FaceRecognizer &model, s32 &faceId, f64 &confidence)
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
    } // namespace Recognize
  } // namespace Embedded
} // namespace Anki
