/**
 * File: DevProxSensorVisualizer.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2/20/18
 *
 * Description: Vision System Component that uses prox sensor data to generate drivable and non drivable pixels
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/lineSegment2d.h"
#include "coretech/vision/engine/imageCache.h"
#include "engine/groundPlaneROI.h"
#include "engine/vision/visionPoseData.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "coretech/vision/engine/image_impl.h"
#include "engine/vision/onlineGrowingForestClassifier.h"

#include "proxSensorImageAnalyzer.h"

#include <vector>


#define DEBUG_VISUALIZATION true

namespace Anki {
namespace Cozmo {

namespace {
# define CONSOLE_GROUP_NAME "Vision.ProxSensorImageAnalyzer"
CONSOLE_VAR(f32,  kProxSensorImageAnalyzer_MaxBodyAngleChange_deg,    CONSOLE_GROUP_NAME, 0.1f);
CONSOLE_VAR(f32,  kProxSensorImageAnalyzer_MaxPoseChange_mm,          CONSOLE_GROUP_NAME, 0.5f);
# undef CONSOLE_GROUP_NAME
}

static const char* kLogChannelName = "VisionSystem";

ProxSensorImageAnalyzer::ProxSensorImageAnalyzer(const Json::Value& config, const CozmoContext *context)
{
  _onlineClf.reset(new OnlineGrowingForestClassifier(context, 10, 10));
}

ProxSensorImageAnalyzer::~ProxSensorImageAnalyzer()
{

}

Result ProxSensorImageAnalyzer::Update(const Vision::ImageRGB& image, const VisionPoseData& crntPoseData,
                                       const VisionPoseData& prevPoseData,
                                       DebugImageList <Anki::Vision::ImageRGB>& debugImageRGBs) const
{

  if (! crntPoseData.groundPlaneVisible) {
    PRINT_CH_DEBUG(kLogChannelName, "DevProxSensorVisualizer.Update.GroundPlaneNotVisible", "");
    return RESULT_OK;
  }

  const bool poseSame = crntPoseData.IsBodyPoseSame(prevPoseData,
                                                    DEG_TO_RAD(kProxSensorImageAnalyzer_MaxBodyAngleChange_deg),
                                                    kProxSensorImageAnalyzer_MaxPoseChange_mm);
  if (poseSame) {
    // Not moving, no need to do stuff

    if (DEBUG_VISUALIZATION) {
      // just send a bogus image to signal no movement
      GroundPlaneROI groundPlaneROI;
      const Matrix_3x3f& H = crntPoseData.groundPlaneHomography;
      const Vision::ImageRGB groundPLaneImage = groundPlaneROI.GetOverheadImage(image, H);
      debugImageRGBs.emplace_back("ProxSensorOnGroundPlane", groundPLaneImage);
    }

    return RESULT_OK;
  }

  // Obtain the overhead ground plane image
  GroundPlaneROI groundPlaneROI;
  const Matrix_3x3f& H = crntPoseData.groundPlaneHomography;
  const Vision::ImageRGB groundPLaneImage = groundPlaneROI.GetOverheadImage(image, H);
  Vision::ImageRGB imageToDisplay;
  if (DEBUG_VISUALIZATION) {
    groundPLaneImage.CopyTo(imageToDisplay);
  }
  
  // Get the prox sensor reading
  const HistRobotState& hist = crntPoseData.histState;
  if (hist.WasProxSensorValid()) {
    // Draw a line where the prox indicates
    const int distance_mm = hist.GetProxSensorVal_mm();
    // the actual distance is shifted by 'dist' in the ground plane
    const int actual_distance = distance_mm - int(groundPlaneROI.GetDist());

    // if it's not too distant..
    if (actual_distance < imageToDisplay.GetNumCols()) {
      const int col = actual_distance;

      // the two extremities vary according to the distance
      // The sensor beam is a cone, and the cone's radius at a given distance is
      // given by distance * tan(fullFOV / 2)
      const float beamRadiusAtDist = distance_mm * std::tan(kProxSensorFullFOV_rad / 2.f);
      const float imageCenterRow = imageToDisplay.GetNumRows() / 2.0f;
      const float minRow = std::max(0.0f, imageCenterRow - beamRadiusAtDist);
      const float maxRow = std::min(float(imageToDisplay.GetNumRows() - 1), imageCenterRow + beamRadiusAtDist);

      if (DEBUG_VISUALIZATION) {
        imageToDisplay.DrawLine({float(col), minRow},
                                {float(col), maxRow},
                                NamedColors::RED,
                                3);
      }

      Anki::Array2d<float> drivable, nonDrivable;
      std::tie(drivable, nonDrivable) = GetDrivableNonDrivable(groundPLaneImage, minRow, maxRow, col,
                                                               imageToDisplay, debugImageRGBs);

      {
        // create training data and feed it to the classifier
        const cv::Mat_<float> drivableMat = drivable.get_CvMat_();
        const cv::Mat_<float> nonDrivableMat = nonDrivable.get_CvMat_();
        cv::Mat allInputs, allClasses;
        cv::vconcat(drivableMat, nonDrivableMat, allInputs);

        const uint numberOfPositives = uint(drivable.GetNumRows());
        cv::vconcat(cv::Mat::ones(numberOfPositives, 1, CV_32FC1),
                    cv::Mat::zeros(allInputs.rows - numberOfPositives, 1, CV_32FC1),
                    allClasses);

        DEV_ASSERT(allInputs.rows == allClasses.rows, "ProxSensorImageAnalyzer.Update.WrongConcatSizes");

        _onlineClf->Train(allInputs, allClasses, numberOfPositives);
      }

    } // if (actual_distance < imageToDisplay.GetNumCols())

  } // if (hist.WasProxSensorValid())

  debugImageRGBs.emplace_back("ProxSensorOnGroundPlane", imageToDisplay);

  return RESULT_OK;
}

std::pair<Anki::Array2d<float>, Anki::Array2d<float>>
ProxSensorImageAnalyzer::GetDrivableNonDrivable(const Vision::ImageRGB& image, const float minRow, const float maxRow,
                                                const float col, Vision::ImageRGB& debugImage,
                                                DebugImageList <Anki::Vision::ImageRGB>& debugImageRGBs) const
{

  // TODO doing the mean image here, shouldn't have to. Need to reconsider the data flow
  cv::Mat meanImage;
  const uint kernelSize = 3; // constant?
  cv::boxFilter(image.get_CvMat_(), meanImage, CV_32F,
                cv::Size(kernelSize, kernelSize),
                cv::Point(-1, -1), true,
                cv::BORDER_REPLICATE);
  DEV_ASSERT(meanImage.type() == CV_32FC3, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.WrongOutputType");

  Anki::Array2d<float> drivableArray2d;
  Anki::Array2d<float> nonDrivableArray2d;

  // first get the triangle for the drivableArray2d pixels
  const Anki::Point2f tip = {0.0, image.GetNumRows() / 2.0};
  const Anki::Point2f triangleUpperPoint = {col, minRow};
  const Anki::Point2f triangleLowerPoint = {col, maxRow};

  // The non-drivableArray2d pixels have a more complicated shape
  // Extend two rays and then intersect with the right edge of the image
  const float extension = 100.0f; // it just has to be a big number
  // extended point of a segment AB is B + (B-A)*extension
  const Anki::Point2f upperProjectedPoint = triangleUpperPoint + (triangleUpperPoint - tip) * extension;
  const Anki::Point2f lowerProjectedPoint = triangleLowerPoint + (triangleLowerPoint - tip) * extension;

  // Find the intersection between the ray starting from the tip and the image edge
  const Anki::LineSegment upperRay(tip, upperProjectedPoint);
  const Anki::LineSegment lowerRay(tip, lowerProjectedPoint);
  const Anki::LineSegment rightImageBoundary(Anki::Point2f(image.GetNumCols(), 0),
                                             Anki::Point2f(image.GetNumCols(), image.GetNumRows()));
  Anki::Point2f intersectionUpperPoint;
  const bool res1 = upperRay.IntersectsAt(rightImageBoundary, intersectionUpperPoint);
  if (!res1) {
    // TODO it happens when the obstacle is too close, investigate why!
    return std::make_pair(drivableArray2d, nonDrivableArray2d);
  }
  Anki::Point2f intersectionLowerPoint;
  const bool res2 = lowerRay.IntersectsAt(rightImageBoundary, intersectionLowerPoint);
  if (!res2) {
    // TODO it happens when the obstacle is too close, investigate why!
    return std::make_pair(drivableArray2d, nonDrivableArray2d);
  }

  // fill drivableArray2d and nonDrivableArray2d pixels
  {
    cv::Mat drivableMask = cv::Mat::zeros(meanImage.rows, meanImage.cols, CV_8U);
    {
      // I know these are convex polygons
      // Shrink the triangle a bit to avoid overlapping with the nonDrivable
      const cv::Point point1 (triangleLowerPoint.x()-1, triangleLowerPoint.y());
      const cv::Point point2 (triangleUpperPoint.x()-1, triangleUpperPoint.y());
      const std::vector<cv::Point> points{tip.get_CvPoint_(),
                                          point1,
                                          point2};
      cv::fillConvexPoly(drivableMask, points, cv::Scalar(255));
    }
    cv::Mat nonDrivableMask = cv::Mat::zeros(meanImage.rows, meanImage.cols, CV_8U);
    {
      // I know these are convex polygons
      const std::vector<cv::Point> points{triangleLowerPoint.get_CvPoint_(),
                                          intersectionLowerPoint.get_CvPoint_(),
                                          intersectionUpperPoint.get_CvPoint_(),
                                          triangleUpperPoint.get_CvPoint_()};
      cv::fillConvexPoly(nonDrivableMask, points, cv::Scalar(255));
    }
    if (DEBUG_VISUALIZATION) {
      // this looks like a lot of work to go from cv::Mat to Vision::ImageRGB
      Vision::Image tmp1((cv::Mat_<u8>(drivableMask)));
      debugImageRGBs.emplace_back("DrivableMask", Vision::ImageRGB(tmp1));
      Vision::Image tmp2((cv::Mat_<u8>(nonDrivableMask)));
      debugImageRGBs.emplace_back("NonDrivableMask", Vision::ImageRGB(tmp2));
    }

    // TODO is this the most efficient way to do this?
    std::vector<cv::Point3f> drivableVector;
    {
      // the drivableArray2d pixels are in the shape of a triangle, so get the area
      const float base = maxRow - minRow;
      const float height = col;
      const float area = base * height / 2.0f;
      drivableVector.reserve(uint(area)+1);
      PRINT_CH_DEBUG(kLogChannelName, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.EstimatedDrivable",
                     "Estimated number of drivable pixels is %f", area);
    }
    std::vector<cv::Point3f> nonDrivableVector;
    {
      // the non drivableArray2d pixels are in the shape of a trapezoid, so get the area
      const float base1 =  maxRow - minRow;
      const float base2 =  intersectionLowerPoint.y() - intersectionUpperPoint.y();
      DEV_ASSERT(base2 > 0, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.Base2<0");
      const float height = meanImage.cols - col;
      const float area = (base1 + base2) * height / 2.0f;
      nonDrivableVector.reserve(uint(area) + 1);
      PRINT_CH_DEBUG(kLogChannelName, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.EstimatedNonDrivable",
                     "Estimated number of non drivable pixels is %f", area);
    }

    DEV_ASSERT(meanImage.channels() == 3, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.WrongChannels");
    for (uint i=0; i<meanImage.rows; i++ ) {
      const uchar* drivableMaskRow = drivableMask.ptr<uchar>(i);
      const uchar* nonDrivableMaskRow = nonDrivableMask.ptr<uchar>(i);
      const cv::Point3f* meanImageRow = meanImage.ptr<cv::Point3f>(i);
      for (uint j=0; j<meanImage.cols; j++) {
        if (drivableMaskRow[j] == 255) { // drivableArray2d pixels
          drivableVector.push_back(meanImageRow[j]);
        }
        else if (nonDrivableMaskRow[j] == 255) {
          nonDrivableVector.push_back(meanImageRow[j]);
        }
        // Sanity check, no two elements should be drivableArray2d and not drivableArray2d
        DEV_ASSERT(! ((drivableMaskRow[j] == 255) && (nonDrivableMaskRow[j] == 255)),
                   "ProxSensorImageAnalyzer.GetDrivableNonDrivable.DrivableAndNotDrivable");
      }
    }
    PRINT_CH_DEBUG(kLogChannelName, "ProxSensorImageAnalyzer.GetDrivableNonDrivable.EffectiveDrivableAndNonDrivable",
                   "Effective number of drivable and non-drivable pixels is (%lu, %lu)",
                   drivableVector.size(),
                   nonDrivableVector.size());
    // first build a cv::Mat, (has to copy), then assign to the Array2d
    {
      const cv::Mat drivableMat(drivableVector, true);
      drivableArray2d = drivableMat;
      const cv::Mat nonDrivableMat(nonDrivableVector, true);
      nonDrivableArray2d = nonDrivableMat;
    }

  }

  if (DEBUG_VISUALIZATION) {
    debugImage.DrawLine(tip, triangleUpperPoint, NamedColors::GREEN, 2);
    debugImage.DrawLine(tip, triangleLowerPoint, NamedColors::GREEN, 2);
    debugImage.DrawLine(triangleUpperPoint, intersectionUpperPoint, NamedColors::BLUE, 2);
    debugImage.DrawLine(triangleLowerPoint, intersectionLowerPoint, NamedColors::BLUE, 2);
  }

  return std::make_pair(drivableArray2d, nonDrivableArray2d);

}

} // namespace Anki
} // namespace Cozmo