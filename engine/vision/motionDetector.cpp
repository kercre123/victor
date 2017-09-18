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

#include "engine/vision/visionPoseData.h"
#include "engine/viz/vizManager.h"

#include "util/console/consoleInterface.h"

#include <opencv2/highgui/highgui.hpp>
#include <iomanip>
#include <coretech/common/include/anki/common/basestation/jsonTools.h>

#define DEBUG_MOTION_DETECTION 1

namespace Anki {
namespace Cozmo {

namespace {
# define CONSOLE_GROUP_NAME "Vision.MotionDetection"
  
  // For speed, compute motion detection on half-resolution images
  CONSOLE_VAR(bool, kMotionDetection_UseHalfRes,          CONSOLE_GROUP_NAME, false);
  
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
  
  CONSOLE_VAR(bool, kMotionDetection_DrawGroundDetectionsInCameraView, CONSOLE_GROUP_NAME, true);

  // The smaller this value the more broken up will be the motion areas, leading to fragmented ones.
  // If too big artificially big motion areas can be created.
  CONSOLE_VAR(u32,  kMotionDetection_MorphologicalSize, CONSOLE_GROUP_NAME, 20);

  // The higher this value the less susceptible to noise motion detection will be. A too high value
  // will lead to discarding some motion areas.
  CONSOLE_VAR(u32,  kMotionDetection_MinAreaForMotion, CONSOLE_GROUP_NAME, 500);

  // How much blurring to apply to the camera image before doing motion detection. This value has to
  // be odd!!
  CONSOLE_VAR(u32,  kMotionDetection_GaussianBlurFilterSize, CONSOLE_GROUP_NAME, 21);
  
# undef CONSOLE_GROUP_NAME
}

// This class is used to accumulate data for peripheral motion detection. The image area is divided in three sections:
// top, right and left. If the centroid of a motion patch falls inside one these areas, it's increased, otherwise it's
// decreased. This follows a very simple impulse/decay model. The parameters *horizontalSize* and *verticalSize* control
// how much of the image is for the left sector and the top sector. The parameters *increaseFactor* and *decreaseFactor*
// control the impulse response.
//
// The centroid of the motion in the different sectors are also stored here. Every time one of the areas is activated,
// the centroid "moves" towards the new activation following an exponential moving average method.
class MotionDetector::ImageRegionSelector
{
public:
  ImageRegionSelector(int imageWidth, int imageHeight, float horizontalSize, float verticalSize,
                      float increaseFactor, float decreaseFactor, float maxValue) :
      _topID(increaseFactor, decreaseFactor, maxValue),
      _leftID(increaseFactor, decreaseFactor, maxValue),
      _rightID(increaseFactor, decreaseFactor, maxValue),
      _maxValue(maxValue)
  {
    DEV_ASSERT(horizontalSize <= 0.5, "MotionDetector::ImageRegionSelector: horizontal size has to be less then half"
                                      "of the image");
    DEV_ASSERT(verticalSize <= 0.5, "MotionDetector::ImageRegionSelector: vertical size has to be less then half"
                                    "of the image");

    _leftMargin = imageWidth * horizontalSize;
    _rightMargin = imageWidth - _leftMargin;
    _upperMargin = imageHeight * verticalSize;

  }

  float GetTopResponse() const
  {
    return _topID.Value();
  }
  float GetLeftResponse() const
  {
    return _leftID.Value();
  }
  float GetRightResponse() const
  {
    return _rightID.Value();
  }

  bool IsTopActivated() const {
    return GetTopResponse() >= _maxValue;
  }
  bool IsRightActivated() const {
    return GetRightResponse() >= _maxValue;
  }
  bool IsLeftActivated() const {
    return GetLeftResponse() >= _maxValue;
  }

  Anki::Point2f GetTopResponseCentroid() const {
    return _topCentroid;
  }
  Anki::Point2f GetLeftResponseCentroid() const {
    return _leftCentroid;
  }
  Anki::Point2f GetRightResponseCentroid() const {
    return _rightCentroid;
  }

  float GetLeftMargin() const {
    return _leftMargin;
  }
  float GetRightMargin() const {
    return _rightMargin;
  }
  float GetUpperMargin() const {
    return _upperMargin;
  }

  void Update(const Anki::Point2f &point,
              float value)
  {

    float x = point.x();
    float y = point.y();
    //where does the point lie?

    //top
    if (y <= _upperMargin) {
      _topID.Update(value);
      _topCentroid = updatePoint(_topCentroid, point);
    }
    else {
      _topID.Decay();
    }
    //left and right are not in exclusion with top
    //left
    if (x <= _leftMargin) {
      _rightID.Decay();
      _leftID.Update(value);
      _leftCentroid = updatePoint(_leftCentroid, point);
    }
      //right
    else if (x >= _rightMargin) {
      _rightID.Update(value);
      _rightCentroid = updatePoint(_rightCentroid, point);
      _leftID.Decay();

    }
    else { //they both go down
      _rightID.Decay();
      _leftID.Decay();
    }

  }

  void Decay() {
    _topID.Decay();
    _rightID.Decay();
    _leftID.Decay();
  }


private:

  // Implements an exponential moving average, a.k.a. low pass filter
  Anki::Point2f updatePoint(const Anki::Point2f &oldPoint, const Anki::Point2f &newPoint) const
  {
    if (oldPoint.x() < 0) {
      return newPoint;
    }
    else {
      return newPoint * _alpha  + oldPoint * (1.0 - _alpha);
    }
  }

  class ImpulseDecay
  {
  public:
    ImpulseDecay(float increaseFactor, float decreaseFactor, float maxValue) :
        _increaseFactor(increaseFactor),
        _decreaseFactor(decreaseFactor),
        _maxValue(maxValue)
    {

    }

    float Update(float value = 0)
    {
      _value = _value + _increaseFactor * value - _decreaseFactor;
      _value = Util::Clamp(_value, 0.0f, _maxValue);

      return _value;
    }

    float Decay()
    {
      return Update(0);
    }

    float Value() const {
      return _value;
    }

  private:
    float _increaseFactor;
    float _decreaseFactor;
    float _maxValue;
    float _value = 0;

  };

  float _leftMargin;

private:
  float _rightMargin;
  float _upperMargin;

  ImpulseDecay _topID;
  ImpulseDecay _leftID;
  ImpulseDecay _rightID;

  Anki::Point2f _topCentroid   = Anki::Point2f(-1, -1);
  Anki::Point2f _leftCentroid  = Anki::Point2f(-1, -1);
  Anki::Point2f _rightCentroid = Anki::Point2f(-1, -1);
  const float _alpha = 0.6;
  float _maxValue;
};

static const char * const kLogChannelName = "VisionSystem";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MotionDetector::MotionDetector(const Vision::Camera &camera, VizManager *vizManager, const Json::Value &config)
:
  _regionSelector(nullptr) //need image size information before we can build this
, _camera(camera)
, _vizManager(vizManager)
, _config(config)
{
  DEV_ASSERT(kMotionDetection_MinBrightness > 0, "MotionDetector.Constructor.MinBrightnessIsZero");

}

// Need to do this for the pimpl implementation of ImageRegionSelector
MotionDetector::~MotionDetector() = default;

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

void MotionDetector::SetPrevImage(const Vision::Image &image, bool wasBlurred)
{
  image.CopyTo(_prevImageGray);
  _wasPrevImageGrayBlurred = wasBlurred;
  _wasPrevImageRGBBlurred = false;
  _prevImageRGB = Vision::ImageRGB();
}

void MotionDetector::SetPrevImage(const Vision::ImageRGB &image, bool wasBlurred)
{
  image.CopyTo(_prevImageRGB);
  _wasPrevImageRGBBlurred = wasBlurred;
  _wasPrevImageGrayBlurred = false;
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

  // Create the ImageRegionSelector. It has to be done here since the image size is not known before
  if (_regionSelector == nullptr) {

    // Helper macro to try to get the specified field and store it in the given variable
    // and return RESULT_FAIL if that doesn't work
#   define GET_JSON_PARAMETER(__json__, __fieldName__, __variable__) \
    do { \
    if(!JsonTools::GetValueOptional(__json__, __fieldName__, __variable__)) { \
      PRINT_NAMED_ERROR("MotionDetection.DetectHelper.MissingJsonParameter", "%s", __fieldName__); \
      return RESULT_FAIL; \
    }} while(0)

    const Json::Value& detectionConfig = _config["MotionDetector"];
    float kHorizontalSize;
    float kVerticalSize;
    float kIncreaseFactor;
    float kDecreaseFactor;
    float kMaxValue;

    GET_JSON_PARAMETER(detectionConfig, "HorizontalSize", kHorizontalSize);
    GET_JSON_PARAMETER(detectionConfig, "VerticalSize", kVerticalSize);
    GET_JSON_PARAMETER(detectionConfig, "IncreaseFactor", kIncreaseFactor);
    GET_JSON_PARAMETER(detectionConfig, "DecreaseFactor", kDecreaseFactor);
    GET_JSON_PARAMETER(detectionConfig, "MaxValue", kMaxValue);

    _regionSelector.reset( new ImageRegionSelector(image.GetNumCols(), image.GetNumRows(),
                                                   kHorizontalSize, kVerticalSize, kIncreaseFactor,
                                                   kDecreaseFactor, kMaxValue));
  }


  const bool headSame = crntPoseData.IsHeadAngleSame(prevPoseData, DEG_TO_RAD(kMotionDetection_MaxHeadAngleChange_deg));
  
  const bool poseSame = crntPoseData.IsBodyPoseSame(prevPoseData,
                                                    DEG_TO_RAD(kMotionDetection_MaxBodyAngleChange_deg),
                                                    kMotionDetection_MaxPoseChange_mm);
  
  //PRINT_STREAM_INFO("pose_angle diff = %.1f\n", RAD_TO_DEG(std::abs(_robotState.pose_angle - _prevRobotState.pose_angle)));

  //Often this will be false
  const bool longEnoughSinceLastMotion = ((image.GetTimestamp() - _lastMotionTime) > kMotionDetection_LastMotionDelay_ms);
  bool blurHappened = false;

  if(headSame && poseSame &&
     HavePrevImage<ImageType>() &&
     !crntPoseData.histState.WasCameraMoving() &&
     longEnoughSinceLastMotion)
  {
    // Save timestamp and prepare the msg
    _lastMotionTime = image.GetTimestamp();
    ExternalInterface::RobotObservedMotion msg;
    msg.timestamp = image.GetTimestamp();

    // Remove noise here before motion detection
    Vision::Image foregroundMotion(image.GetNumRows(), image.GetNumCols());
    blurHappened = true;

    // Main method for motion detection
    s32 numAboveThresh = preprocessRatioImage(image, foregroundMotion);

    // Run the peripheral motion detection
    Result peripheralMotionResult = DetectPeripheralMotion(foregroundMotion, debugImageRGBs, msg, scaleMultiplier);
    if (peripheralMotionResult != RESULT_OK) {
      //something went very wrong here, bail out
      return peripheralMotionResult;
    }
    
    Point2f centroid(0.f,0.f); // Not Embedded::
    Point2f groundPlaneCentroid(0.f,0.f);
    
    // Get overall image centroid
    const size_t minArea = std::round((f32)image.GetNumElements() * kMotionDetection_MinAreaFraction);
    f32 imgRegionArea    = 0.f;
    f32 groundRegionArea = 0.f;
    if(numAboveThresh > minArea) {
      imgRegionArea = GetCentroid(foregroundMotion, centroid, kMotionDetection_CentroidPercentileX,
                                  kMotionDetection_CentroidPercentileY);
    }

    // Get centroid of all the motion within the ground plane, if we have one to reason about
    if(crntPoseData.groundPlaneVisible && prevPoseData.groundPlaneVisible)
    {
      extractGroundPlaneMotion(origNumRows, origNumCols, scaleMultiplier, crntPoseData,
                               foregroundMotion, centroid, groundPlaneCentroid, groundRegionArea);
    }

    // If there's motion either in the image or in the ground area
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
  SetPrevImage(image, blurHappened);
  
  return RESULT_OK;
  
}

void MotionDetector::extractGroundPlaneMotion(s32 origNumRows, s32 origNumCols, f32 scaleMultiplier,
                                              const VisionPoseData &crntPoseData,
                                              const Vision::Image &foregroundMotion,
                                              const Point2f &centroid,
                                              Point2f &groundPlaneCentroid, f32 &groundRegionArea) const
{
  Quad2f imgQuad;
  crntPoseData.groundPlaneROI.GetImageQuad(crntPoseData.groundPlaneHomography, origNumCols, origNumRows, imgQuad);

  imgQuad *= 1.f / scaleMultiplier;

  Rectangle<s32> boundingRect(imgQuad);
  Vision::Image groundPlaneForegroundMotion;
  foregroundMotion.GetROI(boundingRect).CopyTo(groundPlaneForegroundMotion);

  // Zero out everything in the ratio image that's not inside the ground plane quad
  imgQuad -= boundingRect.GetTopLeft().CastTo<float>();

  Vision::Image mask(groundPlaneForegroundMotion.GetNumRows(),
                     groundPlaneForegroundMotion.GetNumCols());
  mask.FillWith(0);
  fillConvexPoly(mask.get_CvMat_(), std::vector<cv::Point>{
        imgQuad[Quad::TopLeft].get_CvPoint_(),
        imgQuad[Quad::TopRight].get_CvPoint_(),
        imgQuad[Quad::BottomRight].get_CvPoint_(),
        imgQuad[Quad::BottomLeft].get_CvPoint_(),
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
  groundPlaneCentroid += boundingRect.GetTopLeft().CastTo<float>(); // casting is explicit

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
}

template<class ImageType>
s32 MotionDetector::preprocessRatioImage(const ImageType &image, Vision::Image &foregroundMotion) {
  const cv::Mat& imageCV = image.get_CvMat_();
  GaussianBlur(imageCV, imageCV,
               cv::Size(kMotionDetection_GaussianBlurFilterSize, kMotionDetection_GaussianBlurFilterSize), 0);


  // If the previous image hadn't been blurred before, do it now
  if (std::is_same<ImageType, Vision::Image>::value) {
    if (!_wasPrevImageGrayBlurred) {
        PRINT_CH_INFO(kLogChannelName, "MotionDetector.DetectMotion.FoundCentroid",
                      "Blurring the previous image Grey");
        GaussianBlur(_prevImageGray.get_CvMat_(), _prevImageGray.get_CvMat_(),
                     cv::Size(kMotionDetection_GaussianBlurFilterSize, kMotionDetection_GaussianBlurFilterSize), 0);
      }
    }
  else if (std::is_same<ImageType, Vision::ImageRGB>::value) {
    if (!_wasPrevImageRGBBlurred) {
        GaussianBlur(_prevImageRGB.get_CvMat_(), _prevImageRGB.get_CvMat_(),
                     cv::Size(kMotionDetection_GaussianBlurFilterSize, kMotionDetection_GaussianBlurFilterSize), 0);
        PRINT_CH_INFO(kLogChannelName, "MotionDetector.DetectMotion.FoundCentroid",
                      "Blurring the previous image RGB");
      }
    }
    else {
      PRINT_NAMED_ERROR("MotionDetector.DetectMotion.FoundCentroid",
                        "Not know type for image???");
    }

  s32 numAboveThresh = RatioTest(image, foregroundMotion);
  return numAboveThresh;
} // Detect()

Result MotionDetector::DetectPeripheralMotion(const Vision::Image &inputImage,
                                              DebugImageList <Anki::Vision::ImageRGB> &debugImageRGBs,
                                              ExternalInterface::RobotObservedMotion &msg, f32 scaleMultiplier) {

  // The image has several disjoint components, try to join them

  cv::Mat structuringElement = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                         cv::Size(kMotionDetection_MorphologicalSize,
                                                                  kMotionDetection_MorphologicalSize));
  const cv::Mat& cvInputImage = inputImage.get_CvMat_();
  cv::morphologyEx( cvInputImage, cvInputImage, cv::MORPH_CLOSE, structuringElement );

  // Get the connected components with stats
  Anki::Array2d<s32> labelImage;
  std::vector<Vision::Image::ConnectedComponentStats> stats;
  inputImage.GetConnectedComponents(labelImage, stats);

  // Update the impulse/decay model
  bool updated = false;
  for (const auto& stat: stats) {
    if (stat.area < kMotionDetection_MinAreaForMotion) { //too small
      continue;
    }
    updated  = true;
    _regionSelector->Update(stat.centroid, stat.area);
  }
  // No movement = global decay
  if (! updated)
    _regionSelector->Decay();

  // Filling the message
  {
    // top
    if (_regionSelector->IsTopActivated())
    {
      float value = _regionSelector->GetTopResponse();
      Anki::Point2f centroid = _regionSelector->GetTopResponseCentroid();
      msg.top_img_area = value; //not really the area here, but the response value
      msg.top_img_x = int16_t(centroid.x() * scaleMultiplier);
      msg.top_img_y = int16_t(centroid.y() * scaleMultiplier);
    }
    else
    {
      msg.top_img_area = 0;
      msg.top_img_x = 0;
      msg.top_img_y = 0;
    }
    // left
    if (_regionSelector->IsLeftActivated())
    {
      float value = _regionSelector->GetLeftResponse();
      Anki::Point2f centroid = _regionSelector->GetLeftResponseCentroid();
      msg.left_img_area = value; //not really the area here, but the response value
      msg.left_img_x = int16_t(centroid.x() * scaleMultiplier);
      msg.left_img_y = int16_t(centroid.y() * scaleMultiplier);
    }
    else
    {
      msg.left_img_area = 0;
      msg.left_img_x = 0;
      msg.left_img_y = 0;
    }
    // right
    if (_regionSelector->IsRightActivated())
    {
      float value = _regionSelector->GetRightResponse();
      Anki::Point2f centroid = _regionSelector->GetRightResponseCentroid();
      msg.right_img_area = value; //not really the area here, but the response value
      msg.right_img_x = int16_t(centroid.x() * scaleMultiplier);
      msg.right_img_y = int16_t(centroid.y() * scaleMultiplier);
    }
    else
    {
      msg.right_img_area = 0;
      msg.right_img_x = 0;
      msg.right_img_y = 0;
    }
  }

  if (DEBUG_MOTION_DETECTION) {
    Vision::ImageRGB imageToDisplay(inputImage);
    // Draw the text
    {
      auto to_string_with_precision = [] (const float a_value, const int n = 6) {
        std::ostringstream out;
        out << std::setprecision(n) << a_value;
        return out.str();
      };

      float scale = 0.5;
      {
        float value = _regionSelector->GetTopResponse();
        std::string text = to_string_with_precision(value, 3);
        Anki::Point2f origin(imageToDisplay.GetNumCols()/2 - 10, 30);
        imageToDisplay.DrawText(origin, text, Anki::ColorRGBA(u8(255), u8(0), u8(0)), scale);
      }
      {
        float value = _regionSelector->GetRightResponse();
        std::string text = to_string_with_precision(value, 3);
        Anki::Point2f origin(imageToDisplay.GetNumCols()-50, imageToDisplay.GetNumRows()/2 );
        imageToDisplay.DrawText(origin, text, Anki::ColorRGBA(u8(255), u8(0), u8(0)), scale);
      }
      {
        float value = _regionSelector->GetLeftResponse();
        std::string text = to_string_with_precision(value, 3);
        Anki::Point2f origin(10, imageToDisplay.GetNumRows()/2);
        imageToDisplay.DrawText(origin, text, Anki::ColorRGBA(u8(255), u8(0), u8(0)), scale);
      }
    }

    // Draw the bounding lines
    { // Top line
      int thickness = 1;
      Anki::Point2f topLeft(0, _regionSelector->GetUpperMargin());
      Anki::Point2f topRight(imageToDisplay.GetNumCols(), _regionSelector->GetUpperMargin());
      imageToDisplay.DrawLine(topLeft, topRight, Anki::ColorRGBA(u8(255), u8(0), u8(0)), thickness);
    }
    { // Left line
      int thickness = 1;
      Anki::Point2f topLeft(_regionSelector->GetLeftMargin(), 0);
      Anki::Point2f bottomLeft(_regionSelector->GetLeftMargin(), imageToDisplay.GetNumRows());
      imageToDisplay.DrawLine(topLeft, bottomLeft, Anki::ColorRGBA(u8(255), u8(0), u8(0)), thickness);
    }
    { // Right line
      int thickness = 1;
      Anki::Point2f topRight(_regionSelector->GetRightMargin(), 0);
      Anki::Point2f BottomRight(_regionSelector->GetRightMargin(), imageToDisplay.GetNumCols());
      imageToDisplay.DrawLine(topRight, BottomRight, Anki::ColorRGBA(u8(255), u8(0), u8(0)), thickness);
    }

    //Draw the motion centroids
    {
      {
        Anki::Point2f centroid = _regionSelector->GetTopResponseCentroid();
        imageToDisplay.DrawFilledCircle(centroid, Anki::ColorRGBA(u8(255), u8(255), u8(0)), 10);
      }
      {
        Anki::Point2f centroid = _regionSelector->GetLeftResponseCentroid();
        imageToDisplay.DrawFilledCircle(centroid, Anki::ColorRGBA(u8(255), u8(255), u8(0)), 10);
      }
      {
        Anki::Point2f centroid = _regionSelector->GetRightResponseCentroid();
        imageToDisplay.DrawFilledCircle(centroid, Anki::ColorRGBA(u8(255), u8(255), u8(0)), 10);
      }
    }

    debugImageRGBs.push_back({"PeripheralMotion", imageToDisplay});
  }

  return RESULT_OK;
}

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
  
}
// GetCentroid()

} // namespace Cozmo
} // namespace Anki

