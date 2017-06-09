/**
 * File: motionDetector.cpp
 *
 * Author: Andrew Stein
 * Date:   2017-04-25
 *
 * Description: Vision system component for detecting motion in images and/or on the ground plane.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "motionDetector.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/linearAlgebra_impl.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/image_impl.h"
#include "anki/vision/basestation/imageCache.h"

#include "anki/cozmo/basestation/visionPoseData.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/console/consoleInterface.h"

#include <opencv2/highgui/highgui.hpp>

#define DEBUG_MOTION_DETECTION 0

namespace Anki {
namespace Cozmo {

namespace {
# define CONSOLE_GROUP_NAME "Vision.MotionDetection"
  
  // For speed, compute motion detection on half-resolution images
  CONSOLE_VAR(bool, kMotionDetection_UseHalfRes,          CONSOLE_GROUP_NAME, true);
  
  // How long we have to wait between motion detections. This may be reduce-able, but can't get
  // too small or we'll hallucinate image change (i.e. "motion") due to the robot moving.
  CONSOLE_VAR(u32,  kMotionDetection_LastMotionDelay_ms,  CONSOLE_GROUP_NAME, 500);
  
  // Affects sensitivity (darker pixels are inherently noisier and should be ignored for
  // change detection). Range is [0,255]
  CONSOLE_VAR(u8,   kMotionDetection_MinBrightness,       CONSOLE_GROUP_NAME, 10);
  
  // This is the main sensitivity parameter: higher means more image difference is required
  // to register a change and thus report motion.
  CONSOLE_VAR(f32,  kMotionDetection_RatioThreshold,      CONSOLE_GROUP_NAME, 1.25f);
  CONSOLE_VAR(f32,  kMotionDetection_MinAreaFraction,     CONSOLE_GROUP_NAME, 1.f/225.f); // 1/15 of each image dimension
  
  // For computing robust "centroid" of motion
  CONSOLE_VAR(f32,  kMotionDetection_CentroidPercentileX,       CONSOLE_GROUP_NAME, 0.5f);  // In image coordinates
  CONSOLE_VAR(f32,  kMotionDetection_CentroidPercentileY,       CONSOLE_GROUP_NAME, 0.5f);  // In image coordinates
  CONSOLE_VAR(f32,  kMotionDetection_GroundCentroidPercentileX, CONSOLE_GROUP_NAME, 0.05f); // In robot coordinates (Most important for pounce: distance from robot)
  CONSOLE_VAR(f32,  kMotionDetection_GroundCentroidPercentileY, CONSOLE_GROUP_NAME, 0.50f); // In robot coordinates
  
  // Tight constraints on max movement allowed to attempt frame differencing for "motion detection"
  CONSOLE_VAR(f32,  kMotionDetection_MaxHeadAngleChange_deg,    CONSOLE_GROUP_NAME, 0.1f);
  CONSOLE_VAR(f32,  kMotionDetection_MaxBodyAngleChange_deg,    CONSOLE_GROUP_NAME, 0.1f);
  CONSOLE_VAR(f32,  kMotionDetection_MaxPoseChange_mm,          CONSOLE_GROUP_NAME, 0.5f);
  
  CONSOLE_VAR(bool, kMotionDetection_DrawGroundDetectionsInCameraView, CONSOLE_GROUP_NAME, false);
  
# undef CONSOLE_GROUP_NAME
}
  
static const char * const kLogChannelName = "VisionSystem";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MotionDetector::MotionDetector(const Vision::Camera& camera, VizManager* vizManager)
: _camera(camera)
, _vizManager(vizManager)
{
  DEV_ASSERT(kMotionDetection_MinBrightness > 0, "MotionDetector.Constructor.MinBrightnessIsZero");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
bool MotionDetector::HavePrevImage<Vision::ImageRGB>() const
{
  return !_prevImageRGB.IsEmpty();
}

template<>
bool MotionDetector::HavePrevImage<Vision::Image>() const
{
  return !_prevImageGray.IsEmpty();
}

void MotionDetector::SetPrevImage(const Vision::Image &image)
{
  image.CopyTo(_prevImageGray);
  _prevImageRGB = Vision::ImageRGB();
}

void MotionDetector::SetPrevImage(const Vision::ImageRGB &image)
{
  image.CopyTo(_prevImageRGB);
  _prevImageGray = Vision::Image();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline f32 ratioTestHelper(u8 value1, u8 value2)
{
  // NOTE: not checking for divide-by-zero here because kMotionDetection_MinBrightness (DEV_ASSERTed to be > 0 in
  //  the constructor) prevents values of zero from getting to this helper
  if(value1 > value2) {
    return static_cast<f32>(value1) / std::max(1.f, static_cast<f32>(value2));
  } else {
    return static_cast<f32>(value2) / std::max(1.f, static_cast<f32>(value1));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 MotionDetector::RatioTest(const Vision::ImageRGB& image, Vision::Image& ratioImg)
{
  DEV_ASSERT(ratioImg.GetNumRows() == image.GetNumRows() && ratioImg.GetNumCols() == image.GetNumCols(),
             "MotionDetector.RatioTestColor.MismatchedSize");
  
  s32 numAboveThresh = 0;
  
  std::function<u8(const Vision::PixelRGB& thisElem, const Vision::PixelRGB& otherElem)> ratioTest = [&numAboveThresh](const Vision::PixelRGB& p1, const Vision::PixelRGB& p2)
  {
    u8 retVal = 0;
    if(p1.IsBrighterThan(kMotionDetection_MinBrightness) &&
       p2.IsBrighterThan(kMotionDetection_MinBrightness))
    {
      const f32 ratioR = ratioTestHelper(p1.r(), p2.r());
      const f32 ratioG = ratioTestHelper(p1.g(), p2.g());
      const f32 ratioB = ratioTestHelper(p1.b(), p2.b());
      if(ratioR > kMotionDetection_RatioThreshold || ratioG > kMotionDetection_RatioThreshold || ratioB > kMotionDetection_RatioThreshold) {
        ++numAboveThresh;
        retVal = 255; // use 255 because it will actually display
      }
    } // if both pixels are bright enough
    
    return retVal;
  };
  
  image.ApplyScalarFunction(ratioTest, _prevImageRGB, ratioImg);
  
  return numAboveThresh;
}

s32 MotionDetector::RatioTest(const Vision::Image& image, Vision::Image& ratioImg)
{
  DEV_ASSERT(ratioImg.GetNumRows() == image.GetNumRows() && ratioImg.GetNumCols() == image.GetNumCols(),
             "MotionDetector.RatioTestGray.MismatchedSize");
  
  s32 numAboveThresh = 0;
  
  std::function<u8(const u8& thisElem, const u8& otherElem)> ratioTest = [&numAboveThresh](const u8& p1, const u8& p2)
  {
    u8 retVal = 0;
    if((p1 > kMotionDetection_MinBrightness) && (p2 > kMotionDetection_MinBrightness))
    {
      const f32 ratio = ratioTestHelper(p1, p2);
      if(ratio > kMotionDetection_RatioThreshold)
      {
        ++numAboveThresh;
        retVal = 255; // use 255 because it will actually display
      }
    } // if both pixels are bright enough
    
    return retVal;
  };
  
  image.ApplyScalarFunction(ratioTest, _prevImageGray, ratioImg);
  
  return numAboveThresh;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result MotionDetector::Detect(Vision::ImageCache&     imageCache,
                              const VisionPoseData&   crntPoseData,
                              const VisionPoseData&   prevPoseData,
                              std::list<ExternalInterface::RobotObservedMotion>& observedMotions,
                              DebugImageList<Vision::ImageRGB>& debugImageRGBs)
{
  const f32 scaleMultiplier = (kMotionDetection_UseHalfRes ? 2.f : 1.f);

  const Vision::ImageCache::Size imageSize = Vision::ImageCache::GetSize((s32)scaleMultiplier,
                                                                         Vision::ResizeMethod::NearestNeighbor);
    
  if(imageCache.HasColor())
  {
    const Vision::ImageRGB& imageColor = imageCache.GetRGB(imageSize);
    return DetectHelper(imageColor, imageCache.GetOrigNumRows(), imageCache.GetOrigNumCols(), scaleMultiplier,
                        crntPoseData, prevPoseData, observedMotions, debugImageRGBs);
  }
  else
  {
    const Vision::Image& imageGray = imageCache.GetGray(imageSize);
    return DetectHelper(imageGray, imageCache.GetOrigNumRows(), imageCache.GetOrigNumCols(), scaleMultiplier,
                        crntPoseData, prevPoseData, observedMotions, debugImageRGBs);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class ImageType>
Result MotionDetector::DetectHelper(const ImageType&        image,
                                    s32 origNumRows, s32 origNumCols, f32 scaleMultiplier,
                                    const VisionPoseData&   crntPoseData,
                                    const VisionPoseData&   prevPoseData,
                                    std::list<ExternalInterface::RobotObservedMotion>& observedMotions,
                                    DebugImageList<Vision::ImageRGB>& debugImageRGBs)
{
  const bool headSame = crntPoseData.IsHeadAngleSame(prevPoseData, DEG_TO_RAD(kMotionDetection_MaxHeadAngleChange_deg));
  
  const bool poseSame = crntPoseData.IsBodyPoseSame(prevPoseData,
                                                    DEG_TO_RAD(kMotionDetection_MaxBodyAngleChange_deg),
                                                    kMotionDetection_MaxPoseChange_mm);
  
  //PRINT_STREAM_INFO("pose_angle diff = %.1f\n", RAD_TO_DEG(std::abs(_robotState.pose_angle - _prevRobotState.pose_angle)));
  
  const bool longEnoughSinceLastMotion = ((image.GetTimestamp() - _lastMotionTime) > kMotionDetection_LastMotionDelay_ms);
  
  if(headSame && poseSame &&
     HavePrevImage<ImageType>() &&
     !crntPoseData.histState.WasCameraMoving() &&
     longEnoughSinceLastMotion)
  {
    Vision::Image foregroundMotion(image.GetNumRows(), image.GetNumCols());
    s32 numAboveThresh = RatioTest(image, foregroundMotion);
    
    static const cv::Matx<u8, 3, 3> kernel(cv::Matx<u8, 3, 3>::ones());
    cv::morphologyEx(foregroundMotion.get_CvMat_(), foregroundMotion.get_CvMat_(), cv::MORPH_OPEN, kernel);
    
    Point2f centroid(0.f,0.f); // Not Embedded::
    Point2f groundPlaneCentroid(0.f,0.f);
    
    // Get overall image centroid
    const size_t minArea = std::round((f32)image.GetNumElements() * kMotionDetection_MinAreaFraction);
    f32 imgRegionArea    = 0.f;
    f32 groundRegionArea = 0.f;
    if(numAboveThresh > minArea) {
      imgRegionArea = GetCentroid(foregroundMotion, centroid, kMotionDetection_CentroidPercentileX, kMotionDetection_CentroidPercentileY);
    }
    
    // Get centroid of all the motion within the ground plane, if we have one to reason about
    if(crntPoseData.groundPlaneVisible && prevPoseData.groundPlaneVisible)
    {
      Quad2f imgQuad;
      crntPoseData.groundPlaneROI.GetImageQuad(crntPoseData.groundPlaneHomography, origNumCols, origNumRows, imgQuad);
      
      imgQuad *= 1.f / scaleMultiplier;
      
      Rectangle<s32> boundingRect(imgQuad);
      Vision::Image groundPlaneForegroundMotion;
      foregroundMotion.GetROI(boundingRect).CopyTo(groundPlaneForegroundMotion);
      
      // Zero out everything in the ratio image that's not inside the ground plane quad
      imgQuad -= boundingRect.GetTopLeft();
      Vision::Image mask(groundPlaneForegroundMotion.GetNumRows(),
                         groundPlaneForegroundMotion.GetNumCols());
      mask.FillWith(0);
      cv::fillConvexPoly(mask.get_CvMat_(), std::vector<cv::Point>{
        imgQuad[Quad::CornerName::TopLeft].get_CvPoint_(),
        imgQuad[Quad::CornerName::TopRight].get_CvPoint_(),
        imgQuad[Quad::CornerName::BottomRight].get_CvPoint_(),
        imgQuad[Quad::CornerName::BottomLeft].get_CvPoint_(),
      }, 255);
      
      for(s32 i=0; i<mask.GetNumRows(); ++i) {
        const u8* maskData_i = mask.GetRow(i);
        u8* fgMotionData_i = groundPlaneForegroundMotion.GetRow(i);
        for(s32 j=0; j<mask.GetNumCols(); ++j) {
          if(maskData_i[j] == 0) {
            fgMotionData_i[j] = 0;
          }
        }
      }
      
      // Find centroid of motion inside the ground plane
      // NOTE!! We swap X and Y for the percentiles because the ground centroid
      //        gets mapped to the ground plane in robot coordinates later, but
      //        small x on the ground corresponds to large y in the *image*, where
      //        the centroid is actually being computed here.
      groundRegionArea = GetCentroid(groundPlaneForegroundMotion,
                                     groundPlaneCentroid,
                                     kMotionDetection_GroundCentroidPercentileY,
                                     (1.f - kMotionDetection_GroundCentroidPercentileX));
      
      // Move back to image coordinates from ROI coordinates
      groundPlaneCentroid += boundingRect.GetTopLeft();
      
      /* Experimental: Try computing moments in an overhead warped view of the ratio image
       groundPlaneRatioImg = _poseData.groundPlaneROI.GetOverheadImage(ratioImg, _poseData.groundPlaneHomography);
       
       cv::Moments moments = cv::moments(groundPlaneRatioImg.get_CvMat_(), true);
       if(moments.m00 > 0) {
       groundMotionAreaFraction = moments.m00 / static_cast<f32>(groundPlaneRatioImg.GetNumElements());
       groundPlaneCentroid.x() = moments.m10 / moments.m00;
       groundPlaneCentroid.y() = moments.m01 / moments.m00;
       groundPlaneCentroid += _poseData.groundPlaneROI.GetOverheadImageOrigin();
       
       // TODO: return other moments?
       }
       */
      
      if(groundRegionArea > 0.f)
      {
        // Switch centroid back to original resolution, since that's where the
        // homography information is valid
        groundPlaneCentroid *= scaleMultiplier;
        
        // Make ground region area into a fraction of the ground ROI area
        const f32 imgQuadArea = imgQuad.ComputeArea();
        DEV_ASSERT(Util::IsFltGTZero(imgQuadArea), "MotionDetector.Detect.QuadWithZeroArea");
        groundRegionArea /= imgQuadArea;
        
        // Map the centroid onto the ground plane, by doing inv(H) * centroid
        Point3f homographyMappedPoint; // In homogenous coordinates
        Result solveResult = LeastSquares(crntPoseData.groundPlaneHomography,
                                          Point3f{groundPlaneCentroid.x(), groundPlaneCentroid.y(), 1.f},
                                          homographyMappedPoint);
        if(RESULT_OK != solveResult) {
          PRINT_NAMED_WARNING("MotionDetector.DetectMotion.LeastSquaresFailed",
                              "Failed to project centroid (%.1f,%.1f) to ground plane",
                              groundPlaneCentroid.x(), groundPlaneCentroid.y());
          // Don't report this centroid
          groundRegionArea = 0.f;
          groundPlaneCentroid = 0.f;
        } else if(homographyMappedPoint.z() <= 0.f) {
          PRINT_NAMED_WARNING("MotionDetector.DetectMotion.BadProjectedZ",
                              "z<=0 (%f) when projecting motion centroid to ground. Bad homography at head angle %.3fdeg?",
                              homographyMappedPoint.z(), RAD_TO_DEG(crntPoseData.histState.GetHeadAngle_rad()));
          // Don't report this centroid
          groundRegionArea = 0.f;
          groundPlaneCentroid = 0.f;
        } else {
          const f32 divisor = 1.f/homographyMappedPoint.z();
          groundPlaneCentroid.x() = homographyMappedPoint.x() * divisor;
          groundPlaneCentroid.y() = homographyMappedPoint.y() * divisor;
          
          // This is just a sanity check that the centroid is reasonable
          if(ANKI_DEVELOPER_CODE)
          {
            // Scale ground quad slightly to account for numerical inaccuracy.
            // Centroid just needs to be very nearly inside the ground quad.
            Quad2f testQuad(crntPoseData.groundPlaneROI.GetGroundQuad());
            testQuad.Scale(1.01f); // Allow for 1% error
            if(!testQuad.Contains(groundPlaneCentroid)) {
              PRINT_NAMED_WARNING("MotionDetector.DetectMotion.BadGroundPlaneCentroid",
                                  "Centroid=(%.2f,%.2f)", centroid.x(), centroid.y());
            }
          }
        }
      }
    } // if(groundPlaneVisible)
    
    if(imgRegionArea > 0 || groundRegionArea > 0.f)
    {
      if(DEBUG_MOTION_DETECTION)
      {
        PRINT_CH_INFO(kLogChannelName, "MotionDetector.DetectMotion.FoundCentroid",
                      "Found motion centroid for %.1f-pixel area region at (%.1f,%.1f) "
                      "-- %.1f%% of ground area at (%.1f,%.1f)",
                      imgRegionArea, centroid.x(), centroid.y(),
                      groundRegionArea*100.f, groundPlaneCentroid.x(), groundPlaneCentroid.y());
      }
      
      if(kMotionDetection_DrawGroundDetectionsInCameraView && (nullptr != _vizManager))
      {
        const f32 radius = std::max(1.f, std::sqrtf(groundRegionArea*(f32)image.GetNumElements() / M_PI_F));
        _vizManager->DrawCameraOval(centroid * scaleMultiplier, radius, radius, NamedColors::YELLOW);
      }
      
      _lastMotionTime = image.GetTimestamp();
      
      ExternalInterface::RobotObservedMotion msg;
      msg.timestamp = image.GetTimestamp();
      
      if(imgRegionArea > 0)
      {
        DEV_ASSERT(centroid.x() >= 0.f && centroid.x() <= image.GetNumCols() &&
                   centroid.y() >= 0.f && centroid.y() <= image.GetNumRows(),
                   "MotionDetector.DetectMotion.CentroidOOB");
        
        // make relative to image center *at processing resolution*
        DEV_ASSERT(_camera.IsCalibrated(), "MotionDetector.Detect.CameraNotCalibrated");
        centroid -= _camera.GetCalibration()->GetCenter() * (1.f/scaleMultiplier);
        
        // Convert area to fraction of image area (to be resolution-independent)
        // Using scale multiplier to return the coordinates in original image coordinates
        msg.img_x = centroid.x() * scaleMultiplier;
        msg.img_y = centroid.y() * scaleMultiplier;
        msg.img_area = imgRegionArea / static_cast<f32>(image.GetNumElements());
      } else {
        msg.img_area = 0;
        msg.img_x = 0;
        msg.img_y = 0;
      }
      
      if(groundRegionArea > 0.f)
      {
        msg.ground_x = std::round(groundPlaneCentroid.x());
        msg.ground_y = std::round(groundPlaneCentroid.y());
        msg.ground_area = groundRegionArea;
      } else {
        msg.ground_area = 0;
        msg.ground_x = 0;
        msg.ground_y = 0;
      }
      
      observedMotions.emplace_back(std::move(msg));
      
      if(DEBUG_MOTION_DETECTION)
      {
        char tempText[128];
        Vision::ImageRGB ratioImgDisp(foregroundMotion);
        ratioImgDisp.DrawCircle(centroid + (_camera.GetCalibration()->GetCenter() * (1.f/scaleMultiplier)),
                                NamedColors::RED, 4);
        snprintf(tempText, 127, "Area:%.2f X:%d Y:%d", imgRegionArea, msg.img_x, msg.img_y);
        cv::putText(ratioImgDisp.get_CvMat_(), std::string(tempText),
                    cv::Point(0,ratioImgDisp.GetNumRows()), CV_FONT_NORMAL, .4f, CV_RGB(0,255,0));
        debugImageRGBs.push_back({"RatioImg", ratioImgDisp});
        
        //_currentResult.debugImages.push_back({"PrevRatioImg", _prevRatioImg});
        //_currentResult.debugImages.push_back({"ForegroundMotion", foregroundMotion});
        //_currentResult.debugImages.push_back({"AND", cvAND});
        
        Vision::Image foregroundMotionFullSize(origNumRows, origNumCols);
        foregroundMotion.Resize(foregroundMotionFullSize, Vision::ResizeMethod::NearestNeighbor);
        Vision::ImageRGB ratioImgDispGround(crntPoseData.groundPlaneROI.GetOverheadImage(foregroundMotionFullSize,
                                                                                      crntPoseData.groundPlaneHomography));
        if(groundRegionArea > 0.f) {
          Point2f dispCentroid(groundPlaneCentroid.x(), -groundPlaneCentroid.y()); // Negate Y for display
          ratioImgDispGround.DrawCircle(dispCentroid - crntPoseData.groundPlaneROI.GetOverheadImageOrigin(),
                                        NamedColors::RED, 2);
          snprintf(tempText, 127, "Area:%.2f X:%d Y:%d", groundRegionArea, msg.ground_x, msg.ground_y);
          cv::putText(ratioImgDispGround.get_CvMat_(), std::string(tempText),
                      cv::Point(0,crntPoseData.groundPlaneROI.GetWidthFar()), CV_FONT_NORMAL, .4f,
                      CV_RGB(0,255,0));
        }
        debugImageRGBs.push_back({"RatioImgGround", ratioImgDispGround});
        
        //
        //_currentResult.debugImageRGBs.push_back({"CurrentImg", image});
      }
    }
    
    //_prevRatioImg = ratio12;
    
  } // if(headSame && poseSame)
  
  // Store a copy of the current image for next time (at correct resolution!)
  SetPrevImage(image);
  
  return RESULT_OK;
  
} // Detect()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Explicit instantiation of Detect() method for Gray and RGB images
template Result MotionDetector::DetectHelper(const Vision::Image&, s32, s32, f32,
                                             const VisionPoseData&,
                                             const VisionPoseData&,
                                             std::list<ExternalInterface::RobotObservedMotion>&,
                                             DebugImageList<Vision::ImageRGB>&);

template Result MotionDetector::DetectHelper(const Vision::ImageRGB&, s32, s32, f32,
                                             const VisionPoseData&,
                                             const VisionPoseData&,
                                             std::list<ExternalInterface::RobotObservedMotion>&,
                                             DebugImageList<Vision::ImageRGB>&);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Computes "centroid" at specified percentiles in X and Y
size_t MotionDetector::GetCentroid(const Vision::Image& motionImg, Point2f& centroid, f32 xPercentile, f32 yPercentile)
{
  std::vector<s32> xValues, yValues;
  
  for(s32 y=0; y<motionImg.GetNumRows(); ++y)
  {
    const u8* motionData_y = motionImg.GetRow(y);
    for(s32 x=0; x<motionImg.GetNumCols(); ++x) {
      if(motionData_y[x] != 0) {
        xValues.push_back(x);
        yValues.push_back(y);
      }
    }
  }
  
  DEV_ASSERT(xValues.size() == yValues.size(), "MotionDetector.GetCentroid.xyValuesSizeMismatch");
  
  if(xValues.empty()) {
    centroid = 0.f;
    return 0;
  } else {
    DEV_ASSERT(xPercentile >= 0.f && xPercentile <= 1.f, "MotionDetector.GetCentroid.xPercentileOOR");
    DEV_ASSERT(yPercentile >= 0.f && yPercentile <= 1.f, "MotionDetector.GetCentroid.yPercentileOOR");
    const size_t area = xValues.size(); // NOTE: area > 0 if we get here
    auto xcen = xValues.begin() + std::round(xPercentile * (f32)(area-1));
    auto ycen = yValues.begin() + std::round(yPercentile * (f32)(area-1));
    std::nth_element(xValues.begin(), xcen, xValues.end());
    std::nth_element(yValues.begin(), ycen, yValues.end());
    centroid.x() = *xcen;
    centroid.y() = *ycen;
    DEV_ASSERT_MSG(centroid.x() >= 0.f && centroid.x() < motionImg.GetNumCols(),
                   "MotionDetector.GetCentroid.xCenOOR",
                   "xcen=%f, not in [0,%d)", centroid.x(), motionImg.GetNumCols());
    DEV_ASSERT_MSG(centroid.y() >= 0.f && centroid.y() < motionImg.GetNumRows(),
                   "MotionDetector.GetCentroid.yCenOOR",
                   "ycen=%f, not in [0,%d)", centroid.y(), motionImg.GetNumRows());
    return area;
  }
  
} // GetCentroid()

} // namespace Cozmo
} // namespace Anki

