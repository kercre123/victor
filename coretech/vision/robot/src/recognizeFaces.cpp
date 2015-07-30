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
#include "anki/vision/robot/lucasKanade.h"

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

namespace Anki
{
  namespace Embedded
  {
    namespace Recognize
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      //Result RecognizeFace(
      //  const Array<u8> &image,
      //  const Rectangle<s32> faceLocation,
      //  cv::CascadeClassifier &eyeClassifier,
      //  const f32 eyeQualityThreshold, //< A value between about 1/2000 to 1/5000 is good, where 0.0 is terrible and 1.0 is wonderful
      //  std::vector<cv::Rect> &detectedEyes,
      //  s32 &leftEyeIndex,
      //  s32 &rightEyeIndex,
      //  f32 &eyeQuality,
      //  s32 &faceId,
      //  f64 &confidence)
      //{
      //  detectedEyes.clear();
      //  leftEyeIndex = -1;
      //  rightEyeIndex = -1;
      //  eyeQuality = 0;
      //  faceId = -1;
      //  confidence = 0;

      //  cv::Mat_<u8> imageCv;
      //  ArrayToCvMat(image, &imageCv);

      //  cv::Rect faceLocationCv(faceLocation.left, faceLocation.top, faceLocation.get_width(), faceLocation.get_height());
      //  cv::Mat faceROI = imageCv(faceLocationCv);

      //  // Preprocess
      //  cv::equalizeHist(faceROI, faceROI);

      //  // Detect eyes
      //  eyeClassifier.detectMultiScale(faceROI, detectedEyes, 1.1, 3, CV_HAAR_SCALE_IMAGE, cv::Size(30,30), cv::Size(faceLocationCv.width/3, faceLocationCv.height/3));

      //  const s32 numEyes = detectedEyes.size();

      //  // Offset the eyes by the face
      //  for(s32 iEye=0; iEye<numEyes; iEye++) {
      //    detectedEyes[iEye].x += faceLocationCv.x;
      //    detectedEyes[iEye].y += faceLocationCv.y;
      //  }

      //  // Are the eyes in a reasonable size and position? Use the metric of Becker2008Evaluation

      //  if(numEyes < 2) {
      //    return RESULT_OK;
      //  }

      //  const f32 eyeQuality_alpha = 20;
      //  const f32 eyeQuality_beta_x = 25;
      //  const f32 eyeQuality_beta_y = 25;
      //  const f32 eyeQuality_gamma = 75;

      //  // Find the best of all detected pairs

      //  for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++) {
      //    for(s32 iRightEye=0; iRightEye<numEyes; iRightEye++) {
      //      if(iLeftEye == iRightEye)
      //        continue;

      //      const f32 faceSize = static_cast<f32>(faceLocationCv.width * faceLocationCv.height);
      //      const f32 leftEyeSize = static_cast<f32>(detectedEyes[iLeftEye].width * detectedEyes[iLeftEye].height);
      //      const f32 rightEyeSize = static_cast<f32>(detectedEyes[iRightEye].width * detectedEyes[iRightEye].height);

      //      const f32 faceY = static_cast<f32>((2*faceLocationCv.y + faceLocationCv.height) / 2);
      //      const f32 leftEyeY = static_cast<f32>((2*detectedEyes[iLeftEye].y + detectedEyes[iLeftEye].height) / 2);
      //      const f32 rightEyeY = static_cast<f32>((2*detectedEyes[iRightEye].y + detectedEyes[iRightEye].height) / 2);

      //      const f32 curValue = EyeQuality(faceLocationCv, detectedEyes[iLeftEye], detectedEyes[iRightEye], eyeQuality_alpha, eyeQuality_beta_x, eyeQuality_beta_y, eyeQuality_gamma);

      //      if(curValue > eyeQuality) {
      //        eyeQuality = curValue;
      //        leftEyeIndex = iLeftEye;
      //        rightEyeIndex = iRightEye;
      //      }
      //    }
      //  } // for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++)

      //  if(eyeQuality < eyeQualityThreshold) {
      //    return RESULT_OK;
      //  }

      //  // Rotate and scale

      //  // Mask out

      //  // Predict the label

      //  return RESULT_OK;
      //} // Result RecognizeFace()

      Result RecognizeFace(
        const Array<u8> &image,
        const Rectangle<s32> faceLocation,
        const FixedLengthList<Array<u8> > &trainingImages,
        const FixedLengthList<Rectangle<s32> > &trainingFaceLocations,
        s32 &faceId,
        f64 &confidence,
        MemoryStack scratch)
      {
        const bool useHistogramPreprocess = false;

        faceId = -1;
        confidence = 0;

        cv::Mat_<u8> imageCv;
        ArrayToCvMat(image, &imageCv);

        // Preprocess
        if(useHistogramPreprocess) {
          // Histogram preprocess
          cv::equalizeHist(imageCv, imageCv);
        } else {
          // DoG preprocess
          cv::Mat_<u8> dog1(imageCv.rows, imageCv.cols);
          cv::Mat_<u8> dog2(imageCv.rows, imageCv.cols);

          cv::GaussianBlur(imageCv, dog1, cv::Size(7,7), 1.0);
          cv::GaussianBlur(imageCv, dog2, cv::Size(13,13), 2.0);

          for(s32 i=0; i<imageCv.rows*imageCv.cols; i++) {
            imageCv.data[i] = (static_cast<s32>(dog1.data[i]) - static_cast<s32>(dog2.data[i]) + 255) >> 1;
          }
        }

        // For each training image, find the best alignment via Affine LK

        const Quadrilateral<f32> templateQuad(
          Point<f32>(static_cast<f32>(faceLocation.left), static_cast<f32>(faceLocation.top)),
          Point<f32>(static_cast<f32>(faceLocation.left), static_cast<f32>(faceLocation.bottom)),
          Point<f32>(static_cast<f32>(faceLocation.right), static_cast<f32>(faceLocation.top)),
          Point<f32>(static_cast<f32>(faceLocation.right), static_cast<f32>(faceLocation.bottom)));

        const s32 numPyramidLevels = 3;

        TemplateTracker::LucasKanadeTracker_Projective templateTracker(
          image,
          templateQuad,
          1.0f,
          numPyramidLevels,
          Transformations::TRANSFORM_PROJECTIVE,
          scratch);

        Array<f32> homography(3, 3, scratch);

        //image.Show("Initial Image", false);

        for(s32 iTraining=0; iTraining<trainingImages.get_size(); iTraining++) {
          PUSH_MEMORY_STACK(scratch);

          Quadrilateral<f32> trainingImageQuad(
            Point<f32>(static_cast<f32>(trainingFaceLocations[iTraining].left), static_cast<f32>(trainingFaceLocations[iTraining].top)),
            Point<f32>(static_cast<f32>(trainingFaceLocations[iTraining].left), static_cast<f32>(trainingFaceLocations[iTraining].bottom)),
            Point<f32>(static_cast<f32>(trainingFaceLocations[iTraining].right), static_cast<f32>(trainingFaceLocations[iTraining].top)),
            Point<f32>(static_cast<f32>(trainingFaceLocations[iTraining].right), static_cast<f32>(trainingFaceLocations[iTraining].bottom)));

          bool numericalFailure;

          Transformations::ComputeHomographyFromQuads(templateQuad, trainingImageQuad, homography, numericalFailure, scratch);

          homography.Print("homography");

          Transformations::PlanarTransformation_f32 intialTransformation(Transformations::TRANSFORM_AFFINE, templateQuad, homography, Point<f32>(0,0), scratch);

          templateTracker.set_transformation(intialTransformation);

          Array<u8> warpedImage(image.get_size(0), image.get_size(1), scratch);

          intialTransformation.Transform(image, warpedImage, scratch);

          //Quadrilateral<f32> warpedQuad = intialTransformation.Transform(const Quadrilateral<f32> &in, scratch);

          Array<u8> trainingImage(trainingImages[iTraining].get_size(0), trainingImages[iTraining].get_size(1), scratch);
          trainingImage.Set(trainingImages[iTraining]);

          cv::Mat_<u8> imageCv;

          ArrayToCvMat(trainingImage, &imageCv);

          // Preprocess
          const bool useHistogramPreprocess = false;
          if(useHistogramPreprocess) {
            // Histogram preprocess
            cv::equalizeHist(imageCv, imageCv);
          } else {
            // DoG preprocess
            cv::Mat_<u8> dog1(imageCv.rows, imageCv.cols);
            cv::Mat_<u8> dog2(imageCv.rows, imageCv.cols);

            cv::GaussianBlur(imageCv, dog1, cv::Size(7,7), 1.0);
            cv::GaussianBlur(imageCv, dog2, cv::Size(13,13), 2.0);

            for(s32 i=0; i<imageCv.rows*imageCv.cols; i++) {
              imageCv.data[i] = (static_cast<s32>(dog1.data[i]) - static_cast<s32>(dog2.data[i]) + 255) >> 1;
            }
          }

          trainingImage.Show("Training", false);
          warpedImage.Show("warpedImage", false);

          const s32 maxIterations = 25;
          const f32 convergenceTolerance = .05f;
          const u8 verify_maxPixelDifference = 20;
          bool verify_converged;
          s32 verify_meanAbsoluteDifference;
          s32 verify_numInBounds;
          s32 verify_numSimilarPixels;

          templateTracker.UpdateTrack(
            trainingImage,
            maxIterations,
            convergenceTolerance,
            verify_maxPixelDifference,
            verify_converged,
            verify_meanAbsoluteDifference,
            verify_numInBounds,
            verify_numSimilarPixels,
            scratch);

          templateTracker.get_transformation().Transform(image, warpedImage, scratch);

          //Quadrilateral<f32> warpedQuad = intialTransformation.Transform(const Quadrilateral<f32> &in, scratch);

          warpedImage.Show("warpedImage2", true);
        }

        cv::Rect faceLocationCv(faceLocation.left, faceLocation.top, faceLocation.get_width(), faceLocation.get_height());
        cv::Mat faceROI = imageCv(faceLocationCv);

        //// Detect eyes
        //eyeClassifier.detectMultiScale(faceROI, detectedEyes, 1.1, 3, CV_HAAR_SCALE_IMAGE, cv::Size(30,30), cv::Size(faceLocationCv.width/3, faceLocationCv.height/3));

        //const s32 numEyes = detectedEyes.size();

        //// Offset the eyes by the face
        //for(s32 iEye=0; iEye<numEyes; iEye++) {
        //  detectedEyes[iEye].x += faceLocationCv.x;
        //  detectedEyes[iEye].y += faceLocationCv.y;
        //}

        //// Are the eyes in a reasonable size and position? Use the metric of Becker2008Evaluation

        //if(numEyes < 2) {
        //  return RESULT_OK;
        //}

        //const f32 eyeQuality_alpha = 20;
        //const f32 eyeQuality_beta_x = 25;
        //const f32 eyeQuality_beta_y = 25;
        //const f32 eyeQuality_gamma = 75;

        //// Find the best of all detected pairs

        //for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++) {
        //  for(s32 iRightEye=0; iRightEye<numEyes; iRightEye++) {
        //    if(iLeftEye == iRightEye)
        //      continue;

        //    const f32 faceSize = static_cast<f32>(faceLocationCv.width * faceLocationCv.height);
        //    const f32 leftEyeSize = static_cast<f32>(detectedEyes[iLeftEye].width * detectedEyes[iLeftEye].height);
        //    const f32 rightEyeSize = static_cast<f32>(detectedEyes[iRightEye].width * detectedEyes[iRightEye].height);

        //    const f32 faceY = static_cast<f32>((2*faceLocationCv.y + faceLocationCv.height) / 2);
        //    const f32 leftEyeY = static_cast<f32>((2*detectedEyes[iLeftEye].y + detectedEyes[iLeftEye].height) / 2);
        //    const f32 rightEyeY = static_cast<f32>((2*detectedEyes[iRightEye].y + detectedEyes[iRightEye].height) / 2);

        //    const f32 curValue = EyeQuality(faceLocationCv, detectedEyes[iLeftEye], detectedEyes[iRightEye], eyeQuality_alpha, eyeQuality_beta_x, eyeQuality_beta_y, eyeQuality_gamma);

        //    if(curValue > eyeQuality) {
        //      eyeQuality = curValue;
        //      leftEyeIndex = iLeftEye;
        //      rightEyeIndex = iRightEye;
        //    }
        //  }
        //} // for(s32 iLeftEye=0; iLeftEye<numEyes; iLeftEye++)

        //if(eyeQuality < eyeQualityThreshold) {
        //  return RESULT_OK;
        //}

        //// Rotate and scale

        //// Mask out

        //// Predict the label

        return RESULT_OK;
      } // Result RecognizeFace()
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
    } // namespace Recognize
  } // namespace Embedded
} // namespace Anki
