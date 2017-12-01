/**
 * File: GroundPlaneClassifier.cpp
 *
 * Author: Lorenzo Riano
 * Created: 11/29/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "groundPlaneClassifier.h"

#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/logisticRegression.h" // TODO this is temporary only for calculateError
#include "basestation/utils/data/dataPlatform.h"
#include "engine/cozmoContext.h"
#include "engine/groundPlaneROI.h"
#include "util/fileUtils/fileUtils.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#define DEBUG_DISPLAY_IMAGES true

namespace Anki {
namespace Cozmo {

GroundPlaneClassifier::GroundPlaneClassifier(const Json::Value& config, const CozmoContext *context)
  : _context(context)
{

  DEV_ASSERT(context != nullptr, "GroundPlaneClassifier.ContextCantBeNULL");

  // TODO the classifier should be loaded from a file, not created here
  {
    _classifier.reset(new DTDrivingSurfaceClassifier(config, context));
    const std::string path = _context->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                         "vision/groundclassification");
//    const std::string path = "/Users/lorenzori/tmp/images_training/simulated_images";
    PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.pathToData", "the path to data is: %s",
                    path.c_str());

    const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
    const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

    bool result = _classifier->TrainFromFiles(positivePath.c_str(), negativePath.c_str());

    DEV_ASSERT(result, "GroundPlaneClassifier.TrainingFromFiles");

    // Error on whole training set
    {
      cv::Mat trainingSamples, trainingLabels;
      _classifier->GetTrainingData(trainingSamples, trainingLabels);

      // building a std::vector<Vision::PixelRGB>
      std::vector<Anki::Vision::PixelRGB> pixels;
      {
        pixels.reserve(trainingSamples.rows);
        const cv::Vec3f* data = trainingSamples.ptr<cv::Vec3f>(0);
        for (int i = 0; i < trainingSamples.rows; ++i) {
          const cv::Vec3f& vec = data[i];
          pixels.emplace_back(vec[0], vec[1], vec[2]);
        }
      }

      // calculate error
      std::vector<uchar> responses = _classifier->PredictClass(pixels);
      cv::Mat responsesMat(responses);

      const float error = Anki::calculateError(responsesMat, trainingLabels);
      PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.Train.ErrorLevel", "Error after training is: %f", error);
      DEV_ASSERT(error < 0.1, "GroundPLaneClassifier.TrainingErrorTooHigh");
    }

  }
}

Result GroundPlaneClassifier::Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                                     DebugImageList <Vision::ImageRGB>& debugImageRGBs,
                                     std::list<Poly2f>& outPolygons)
{

  // nothing to do here if there's no ground plane visible
  if (! poseData.groundPlaneVisible) {
    PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.Update.GroundPlane", "Ground plane is not visible");
    return RESULT_OK;
  }

  // TODO Probably STEP 1 and 2 can be combined in a single pass

  // STEP 1: Obtain the overhead ground plane image
  GroundPlaneROI groundPlaneROI;
  const Matrix_3x3f& H = poseData.groundPlaneHomography;
  const Vision::ImageRGB groundPlaneImage = groundPlaneROI.GetOverheadImage(image, H);

  if (DEBUG_DISPLAY_IMAGES) {
    debugImageRGBs.emplace_back("OverheadImage", groundPlaneImage);
  }

  // STEP 2: Classify the overhead image
  Vision::Image classifiedMask(groundPlaneImage.GetNumRows(), groundPlaneImage.GetNumCols(), u8(0));
  _classifier->classifyImage(groundPlaneImage, classifiedMask);

  // STEP 3: To find the contours only in the right area, invert the mask while applying the overhead mask
  //         This way obstacles become white and everything else is black
  // TODO this thing is not working when the head is up and a bit of black shows at the bottom of the plane!!!
  cv::bitwise_not(classifiedMask.get_CvMat_(),
                  classifiedMask.get_CvMat_(),
                  groundPlaneROI.GetOverheadMask().get_CvMat_());

  Vision::ImageRGB colorMask;
  if (DEBUG_DISPLAY_IMAGES) {
    // save a copy of the original mask
    colorMask.SetFromGray(classifiedMask);
  }

  // STEP 4: Get the bounding box around the objects
  std::vector<std::vector<cv::Point>> contours;
  // TODO experiment with CHAIN_APPROX_TC89_L1 or CHAIN_APPROX_TC89_KCOS
  cv::findContours(classifiedMask.get_CvMat_(), contours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

  PRINT_CH_DEBUG("VisionSystem", "GroundPlaneClassifier.Update.FindContours", "%d contours found", int(contours.size()));

  // STEP 5: Create the polygons with the right metric information
  // In the overhead image (and mask) 1 pixel == 1 mm. Thanks Andrew!!
  for (auto& contour : contours) {
    Poly2f polygon;
    polygon.reserve(contour.size());
    for (auto& point : contour) {

      // ground_x = point.y, ground.y = point.x ??
      const f32 ground_x = static_cast<f32>(point.y) + groundPlaneROI.GetDist();
      const f32 ground_y = static_cast<f32>(point.x) - 0.5f*groundPlaneROI.GetWidthFar(); // Zero is at the center;

      polygon.emplace_back(Point2f(ground_x, ground_y));
    }
    outPolygons.push_back(polygon);
  }

  if (DEBUG_DISPLAY_IMAGES) {
    cv::Scalar color(255, 0, 0);
    cv::drawContours(colorMask.get_CvMat_(),
                     contours,
                     -1,
                     color,
                     1);
    debugImageRGBs.emplace_back("ColoredMask", colorMask);
  }

  if (DEBUG_DISPLAY_IMAGES) {
    // Draw Ground plane on the image and display it
    Vision::ImageRGB toDisplay;
    image.CopyTo(toDisplay);
    Quad2f quad;
    if (poseData.groundPlaneVisible) {
      groundPlaneROI.GetImageQuad(H,
                                  toDisplay.GetNumCols(),
                                  toDisplay.GetNumRows(),
                                  quad);

      toDisplay.DrawQuad(quad, NamedColors::WHITE, 3);
      debugImageRGBs.emplace_back("GroundQuadImage", toDisplay);
    }
  }

  return RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
