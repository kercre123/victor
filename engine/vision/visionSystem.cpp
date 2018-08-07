/**
 * File: visionSystem.cpp [Basestation]
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: High-level module that controls the basestation vision system
 *              Runs on its own thread inside VisionComponent.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "visionSystem.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/linearAlgebra_impl.h"
#include "coretech/common/engine/math/linearClassifier.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/vision/groundPlaneClassifier.h"
#include "engine/vision/illuminationDetector.h"
#include "engine/vision/imageSaver.h"
#include "engine/vision/laserPointDetector.h"
#include "engine/vision/motionDetector.h"
#include "engine/vision/overheadEdgesDetector.h"
#include "engine/vision/overheadMap.h"
#include "engine/vision/visionModesHelpers.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "coretech/vision/engine/benchmark.h"
#include "coretech/vision/engine/cameraImagingPipeline.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/markerDetector.h"
#include "coretech/vision/engine/neuralNetRunner.h"
#include "coretech/vision/engine/petTracker.h"

#include "clad/vizInterface/messageViz.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/cleanupHelper.h"
#include "util/helpers/templateHelpers.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"

#include <thread>
#include <fstream>

#include "opencv2/calib3d/calib3d.hpp"

// Cozmo-Specific Library Includes
#include "anki/cozmo/shared/cozmoConfig.h"

#define DEBUG_MOTION_DETECTION    0
#define DEBUG_FACE_DETECTION      0
#define DEBUG_DISPLAY_CLAHE_IMAGE 0

#define DRAW_TOOL_CODE_DEBUG 0

namespace Anki {
namespace Vector {
  
CONSOLE_VAR_RANGED(u8,  kUseCLAHE_u8,     "Vision.PreProcessing", 0, 0, 4);  // One of MarkerDetectionCLAHE enum
CONSOLE_VAR(s32, kClaheClipLimit,         "Vision.PreProcessing", 32);
CONSOLE_VAR(s32, kClaheTileSize,          "Vision.PreProcessing", 4);
CONSOLE_VAR(u8,  kClaheWhenDarkThreshold, "Vision.PreProcessing", 80); // In MarkerDetectionCLAHE::WhenDark mode, only use CLAHE when img avg < this
CONSOLE_VAR(s32, kPostClaheSmooth,        "Vision.PreProcessing", -3); // 0: off, +ve: Gaussian sigma, -ve (& odd): Box filter size
CONSOLE_VAR(s32, kMarkerDetector_ScaleMultiplier, "Vision.MarkerDetection", 1);

// How long to disable auto exposure after using detections to meter
CONSOLE_VAR(u32, kMeteringHoldTime_ms,    "Vision.PreProcessing", 2000);
  
CONSOLE_VAR(f32, kEdgeThreshold,  "Vision.OverheadEdges", 50.f);
CONSOLE_VAR(u32, kMinChainLength, "Vision.OverheadEdges", 3); // in number of edge pixels

// Loose constraints on how fast Cozmo can move and still trust tracker (which has no
// knowledge of or access to camera movement). Rough means of deciding these angles:
// look at angle created by distance between two faces seen close together at the max
// distance we care about seeing them from. If robot turns by that angle between two
// consecutve frames, it is possible the tracker will be confused and jump from one
// to the other.
CONSOLE_VAR(f32,  kFaceTrackingMaxHeadAngleChange_deg, "Vision.FaceDetection", 8.f);
CONSOLE_VAR(f32,  kFaceTrackingMaxBodyAngleChange_deg, "Vision.FaceDetection", 8.f);
CONSOLE_VAR(f32,  kFaceTrackingMaxPoseChange_mm,       "Vision.FaceDetection", 10.f);

// Sample rate for estimating the mean of an image (increment in both X and Y)
CONSOLE_VAR_RANGED(s32, kImageMeanSampleInc, "VisionSystem.Statistics", 10, 1, 32);

// For testing artificial slowdowns of the vision thread
CONSOLE_VAR(u32, kVisionSystemSimulatedDelay_ms, "Vision.General", 0);

CONSOLE_VAR(u32, kCalibTargetType, "Vision.Calibration", (u32)CameraCalibrator::CalibTargetType::CHECKERBOARD);

#if REMOTE_CONSOLE_ENABLED
// If non-zero, toggles the corresponding VisionMode and sets back to 0
CONSOLE_VAR(u32, kToggleVisionMode, "Vision.General", 0);
#endif
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  // These are initialized from Json config:
  u8 kTooDarkValue   = 15;
  u8 kTooBrightValue = 230;
  f32 kLowPercentile = 0.10f;
  f32 kTargetPercentile = 0.50f;
  f32 kHighPercentile = 0.90f;
  bool kMeterFromDetections = true;
}

static const char * const kLogChannelName = "VisionSystem";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionSystem::VisionSystem(const CozmoContext* context)
: _rollingShutterCorrector()
, _lastRollingShutterCorrectionTime(0)
, _imageCache(new Vision::ImageCache())
, _context(context)
, _imagingPipeline(new Vision::ImagingPipeline())
, _poseOrigin("VisionSystemOrigin")
, _vizManager(context == nullptr ? nullptr : context->GetVizManager())
, _petTracker(new Vision::PetTracker())
, _markerDetector(new Vision::MarkerDetector(_camera))
, _laserPointDetector(new LaserPointDetector(_vizManager))
, _overheadEdgeDetector(new OverheadEdgesDetector(_camera, _vizManager, *this))
, _cameraCalibrator(new CameraCalibrator(*this))
, _illuminationDetector(new IlluminationDetector())
, _imageSaver(new ImageSaver())
, _benchmark(new Vision::Benchmark())
, _neuralNetRunner(new Vision::NeuralNetRunner())
, _clahe(cv::createCLAHE())
{
  DEV_ASSERT(_context != nullptr, "VisionSystem.Constructor.NullContext");
} // VisionSystem()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::Init(const Json::Value& config)
{
  _isInitialized = false;
  
  std::string dataPath("");
  std::string cachePath("");
  if(_context->GetDataPlatform() != nullptr) {
    dataPath = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                           Util::FileUtils::FullFilePath({"config", "engine", "vision"}));
    cachePath = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "vision");
  } else {
    PRINT_NAMED_WARNING("VisionSystem.Init.NullDataPlatform",
                        "Initializing VisionSystem with no data platform.");
  }
  if(!config.isMember("ImageQuality"))
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingImageQualityConfigField", "");
    return RESULT_FAIL;
  }
    
  
  // Helper macro to try to get the specified field and store it in the given variable
  // and return RESULT_FAIL if that doesn't work
#   define GET_JSON_PARAMETER(__json__, __fieldName__, __variable__) \
  do { \
  if(!JsonTools::GetValueOptional(__json__, __fieldName__, __variable__)) { \
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingJsonParameter", "%s", __fieldName__); \
    return RESULT_FAIL; \
  }} while(0)

  {
    // Set up auto-exposure
    const Json::Value& imageQualityConfig = config["ImageQuality"];
    GET_JSON_PARAMETER(imageQualityConfig, "TooBrightValue",      kTooBrightValue);
    GET_JSON_PARAMETER(imageQualityConfig, "TooDarkValue",        kTooDarkValue);
    GET_JSON_PARAMETER(imageQualityConfig, "MeterFromDetections", kMeterFromDetections);
    GET_JSON_PARAMETER(imageQualityConfig, "LowPercentile",       kLowPercentile);
    GET_JSON_PARAMETER(imageQualityConfig, "HighPercentile",      kHighPercentile);
    
    u8  targetValue=0;
    f32 maxChangeFraction = -1.f;
    s32 subSample = 0;
    
    GET_JSON_PARAMETER(imageQualityConfig, "TargetPercentile",    kTargetPercentile);
    GET_JSON_PARAMETER(imageQualityConfig, "TargetValue",         targetValue);
    GET_JSON_PARAMETER(imageQualityConfig, "MaxChangeFraction",   maxChangeFraction);
    GET_JSON_PARAMETER(imageQualityConfig, "SubSample",           subSample);
    
    std::vector<u8> cyclingTargetValues;
    if(!JsonTools::GetVectorOptional(imageQualityConfig, "CyclingTargetValues", cyclingTargetValues))
    {
      PRINT_NAMED_ERROR("VisionSystem.Init.MissingJsonParameter", "%s", "CyclingTargetValues");
      return RESULT_FAIL;
    }
    
    const Result result = _imagingPipeline->SetExposureParameters(targetValue,
                                                                  cyclingTargetValues,
                                                                  kTargetPercentile,
                                                                  maxChangeFraction,
                                                                  subSample);
    
    if(RESULT_OK == result)
    {
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.Init.SetAutoExposureParams",
                    "subSample:%d tarVal:%d tarPerc:%.3f changeFrac:%.3f",
                    subSample, targetValue, kTargetPercentile, maxChangeFraction);
    }
    else
    {
      PRINT_NAMED_ERROR("VisionSystem.Init.SetExposureParametersFailed", "");
      return result;
    }
  }
  
  {
    // Set up profiler logging frequencies
    f32 timeBetweenProfilerInfoPrints_sec = 5.f;
    f32 timeBetweenProfilerDasLogs_sec = 60.f;
    
    const Json::Value& performanceConfig = config["PerformanceLogging"];
    GET_JSON_PARAMETER(performanceConfig, "TimeBetweenProfilerInfoPrints_sec", timeBetweenProfilerInfoPrints_sec);
    GET_JSON_PARAMETER(performanceConfig, "TimeBetweenProfilerDasLogs_sec",    timeBetweenProfilerDasLogs_sec);
    
    Profiler::SetProfileGroupName("VisionSystem.Profiler");
    Profiler::SetPrintChannelName(kLogChannelName);
    Profiler::SetPrintFrequency(Util::SecToMilliSec(timeBetweenProfilerInfoPrints_sec));
    Profiler::SetDasLogFrequency(Util::SecToMilliSec(timeBetweenProfilerDasLogs_sec));
  }
  
  PRINT_CH_INFO(kLogChannelName, "VisionSystem.Init.InstantiatingFaceTracker",
                "With model path %s.", dataPath.c_str());
  _faceTracker.reset(new Vision::FaceTracker(_camera, dataPath, config));
  PRINT_CH_INFO(kLogChannelName, "VisionSystem.Init.DoneInstantiatingFaceTracker", "");

  _motionDetector.reset(new MotionDetector(_camera, _vizManager, config));

  if (!config.isMember("OverheadMap")) {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingJsonParameter", "OverheadMap");
    return RESULT_FAIL;
  }
  _overheadMap.reset(new OverheadMap(config["OverheadMap"], _context));

  // TODO check config entry here
  _groundPlaneClassifier.reset(new GroundPlaneClassifier(config["GroundPlaneClassifier"], _context));

  const Result petTrackerInitResult = _petTracker->Init(config);
  if(RESULT_OK != petTrackerInitResult) {
    PRINT_NAMED_ERROR("VisionSystem.Init.PetTrackerInitFailed", "");
    return petTrackerInitResult;
  }
  
  if(!config.isMember("NeuralNets"))
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingNeuralNetsConfigField", "");
    return RESULT_FAIL;
  }
  
  const std::string modelPath = Util::FileUtils::FullFilePath({dataPath, "dnn_models"});
  if(Util::FileUtils::DirectoryExists(modelPath)) // TODO: Remove once DNN models are checked in somewhere (VIC-1071)
  {
    const Json::Value& neuralNetConfig = config["NeuralNets"];
    
#   ifdef VICOS
    // Use faster tmpfs partition for the cache, to make I/O less of a bottleneck
    const std::string dnnCachePath = "/tmp/vision/neural_nets";
#   else
    const std::string dnnCachePath = Util::FileUtils::FullFilePath({cachePath, "neural_nets"});
#   endif
    Result neuralNetResult = _neuralNetRunner->Init(modelPath,
                                                    dnnCachePath,
                                                    neuralNetConfig);
    if(RESULT_OK != neuralNetResult)
    {
      PRINT_NAMED_ERROR("VisionSystem.Init.NeuralNetInitFailed", "");
    }
  }
   
  if(!config.isMember("IlluminationDetector"))
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingIlluminationDetectorConfigField", "");
    return RESULT_FAIL;
  }
  Result illuminationResult = _illuminationDetector->Init(config["IlluminationDetector"], _context);
  if( illuminationResult != RESULT_OK )
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.IlluminationDetectorInitFailed", "");
    return RESULT_FAIL;
  }

  // Default processing modes should are set in vision_config.json
  if(!config.isMember("InitialVisionModes"))
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingInitialVisionModesConfigField", "");
    return RESULT_FAIL;
  }
  
  const Json::Value& configModes = config["InitialVisionModes"];
  for(auto & modeName : configModes.getMemberNames())
  {
    VisionMode mode = GetModeFromString(modeName);
    if(mode == VisionMode::Idle) {
      PRINT_NAMED_WARNING("VisionSystem.Init.BadVisionMode",
                          "Ignoring initial Idle mode for string '%s' in vision config",
                          modeName.c_str());
    } else {
      EnableMode(mode, configModes[modeName].asBool());
    }
  }
  
  if(!config.isMember("InitialModeSchedules"))
  {
    PRINT_NAMED_ERROR("VisionSystem.Init.MissingInitialModeSchedulesConfigField", "");
    return RESULT_FAIL;
  }
  
  const Json::Value& modeSchedulesConfig = config["InitialModeSchedules"];
  
  for(s32 modeIndex = 0; modeIndex < (s32)VisionMode::Count; ++modeIndex)
  {
    const VisionMode mode = (VisionMode)modeIndex;
    const char* modeStr = EnumToString(mode);
    
    if(modeSchedulesConfig.isMember(modeStr))
    {
      const Json::Value& jsonSchedule = modeSchedulesConfig[modeStr];
      if(jsonSchedule.isArray())
      {
        std::vector<bool> schedule;
        schedule.reserve(jsonSchedule.size());
        for(auto jsonIter = jsonSchedule.begin(); jsonIter != jsonSchedule.end(); ++jsonIter)
        {
          schedule.push_back(jsonIter->asBool());
        }
        AllVisionModesSchedule::SetDefaultSchedule(mode, VisionModeSchedule(std::move(schedule)));
      }
      else if(jsonSchedule.isInt())
      {
        AllVisionModesSchedule::SetDefaultSchedule(mode, VisionModeSchedule(jsonSchedule.asInt()));
      }
      else if(jsonSchedule.isBool())
      {
        AllVisionModesSchedule::SetDefaultSchedule(mode, VisionModeSchedule(jsonSchedule.asBool()));
      }
      else
      {
        PRINT_NAMED_ERROR("VisionSystem.Init.UnrecognizedModeScheduleValue",
                          "Mode:%s Expecting int, bool, or array of bools", modeStr);
        return RESULT_FAIL;
      }
    }
  }
  
  // Put the default schedule on the stack. We will never pop this.
  _modeScheduleStack.push_front(AllVisionModesSchedule());
  
  _clahe->setClipLimit(kClaheClipLimit);
  _clahe->setTilesGridSize(cv::Size(kClaheTileSize, kClaheTileSize));
  _lastClaheTileSize = kClaheTileSize;
  _lastClaheClipLimit = kClaheClipLimit;
  
  _isInitialized = true;
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::UpdateCameraCalibration(std::shared_ptr<Vision::CameraCalibration> camCalib)
{
  Result result = RESULT_OK;
  const bool updatedCalibration = _camera.SetCalibration(camCalib);
  if(!updatedCalibration)
  {
    // Camera already calibrated with same settings, no need to do anything
    return result;
  }
  
  // Re-initialize the marker detector for the new image size
  _markerDetector->Init(camCalib->GetNrows(), camCalib->GetNcols());

  // Provide the ImageSaver with the camera calibration so that it can remove distortion if needed
  // Also pre-cache distortion maps for Sensor resolution, since we use that for photos
  _imageSaver->SetCalibration(camCalib);
  _imageSaver->CacheUndistortionMaps(CAMERA_SENSOR_RESOLUTION_HEIGHT, CAMERA_SENSOR_RESOLUTION_WIDTH);
  
  return result;
} // Init()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionSystem::~VisionSystem()
{
  
}

#if 0
#pragma mark --- Mode Controls ---
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::PushNextModeSchedule(AllVisionModesSchedule&& schedule)
{
  _nextSchedules.push({true, std::move(schedule)});
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::PopModeSchedule()
{
  _nextSchedules.push({false, AllVisionModesSchedule()});
  return RESULT_OK;
}

Result VisionSystem::SetNextMode(VisionMode mode, bool enable)
{
  _nextModes.push({mode, enable});
  return RESULT_OK;
}

Result VisionSystem::SetNextCameraExposure(s32 exposure_ms, f32 gain)
{
  bool& nextParamsSet = _nextCameraParams.first;
  if(nextParamsSet)
  {
    PRINT_NAMED_WARNING("VisionSystem.SetNextCameraParams.OverwritingPreviousParams",
                        "Params already requested (%dms,%.2f) but not sent. Replacing with (%dms,%.2f)",
                        _nextCameraParams.second.exposureTime_ms, _nextCameraParams.second.gain,
                        exposure_ms, gain);
  }
  
  _nextCameraParams.second.exposureTime_ms = exposure_ms;
  _nextCameraParams.second.gain = gain;
  nextParamsSet = true;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::SetNextCameraWhiteBalance(f32 whiteBalanceGainR,
                                               f32 whiteBalanceGainG,
                                               f32 whiteBalanceGainB)
{
  bool& nextParamsSet = _nextCameraParams.first;
  if(nextParamsSet)
  {
    PRINT_NAMED_WARNING("VisionSystem.SetNextCameraWhiteBalance.OverwritingPreviousParams",
                        "Params already requested (%.2f,%.2f,%.2f) but not sent. Replacing with (%.2f,%.2f,%.2f)",
                        _nextCameraParams.second.whiteBalanceGainR,
                        _nextCameraParams.second.whiteBalanceGainG,
                        _nextCameraParams.second.whiteBalanceGainB,
                        whiteBalanceGainR,
                        whiteBalanceGainG,
                        whiteBalanceGainB);
  }
  
  _nextCameraParams.second.whiteBalanceGainR = whiteBalanceGainR;
  _nextCameraParams.second.whiteBalanceGainG = whiteBalanceGainG;
  _nextCameraParams.second.whiteBalanceGainB = whiteBalanceGainB;
  nextParamsSet = true;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionSystem::SetSaveParameters(const ImageSaverParams& params)
{
  const Result result = _imageSaver->SetParams(params);
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("VisionSystem.SetSaveParameters.BadParams", "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::EnableMode(VisionMode whichMode, bool enabled)
{
  switch(whichMode)
  {
    case VisionMode::Idle:
    {
      if(enabled) {
        // "Enabling" idle means to turn everything off
        PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnableMode.Idle",
                      "Disabling all vision modes");
        _mode.ClearFlags();
        _mode.SetBitFlag(whichMode, true);
      } else {
        PRINT_NAMED_WARNING("VisionSystem.EnableMode.InvalidRequest", "Ignoring request to 'disable' idle mode.");
      }
      
      break;
    }
      
    case VisionMode::EstimatingFacialExpression:
    {
      DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.EnableEstimatingExpression.NullFaceTracker");
      
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnableMode.EnableExpressionEstimation",
                    "Enabled=%c", (enabled ? 'Y' : 'N'));

      _faceTracker->EnableEmotionDetection(enabled);
      break;
    }
      
    case VisionMode::DetectingSmileAmount:
    {
      DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.EnableDetectingSmileAmount.NullFaceTracker");
      
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnableMode.EnableDetectingSmileAmount", "Enabled=%c", (enabled ? 'Y' : 'N'));
      
      _faceTracker->EnableSmileDetection(enabled);
      break;
    }
      
    case VisionMode::DetectingGaze:
    {
      DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.EnableDetectingGaze.NullFaceTracker");
      
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnableMode.EnableDetectingGaze", "Enabled=%c", (enabled ? 'Y' : 'N'));
      
      _faceTracker->EnableGazeDetection(enabled);
      break;
    }
      
    case VisionMode::DetectingBlinkAmount:
    {
      DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.EnableDetectingBlinkAmount.NullFaceTracker");
      
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnableMode.DetectingBlinkAmount", "Enabled=%c", (enabled ? 'Y' : 'N'));
      
      _faceTracker->EnableBlinkDetection(enabled);
      break;
    }
      
    default:
    {
      if(enabled) {
        const bool modeAlreadyEnabled = _mode.IsBitFlagSet(whichMode);
        if(!modeAlreadyEnabled) {
          PRINT_CH_INFO(kLogChannelName, "VisionSystem.EnablingMode",
                        "Adding mode %s to current mode %s.",
                        EnumToString(whichMode),
                        VisionSystem::GetModeName(_mode).c_str());
          
          _mode.SetBitFlag(VisionMode::Idle, false);
          _mode.SetBitFlag(whichMode, true);
        }
      } else {
        const bool modeAlreadyDisabled = !_mode.IsBitFlagSet(whichMode);
        if(!modeAlreadyDisabled) {
          PRINT_CH_INFO(kLogChannelName, "VisionSystem.DisablingMode",
                        "Removing mode %s from current mode %s.",
                        EnumToString(whichMode),
                        VisionSystem::GetModeName(_mode).c_str());
          _mode.SetBitFlag(whichMode, false);
          if (!_mode.AreAnyFlagsSet())
          {
            _mode.SetBitFlag(VisionMode::Idle, true);
          }
        }
      }
      break;
    }
  } // switch(whichMode)
  
  return RESULT_OK;
} // EnableMode()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::UpdatePoseData(const VisionPoseData& poseData)
{
  std::swap(_prevPoseData, _poseData);
  _poseData = poseData;
  
  // Update cameraPose and historical state's pose to use the vision system's pose origin
  {
    // We expect the passed-in historical pose to be w.r.t. to an origin and have its parent removed
    // so that we can set its parent to be our poseOrigin on this thread. The cameraPose
    // should use the histState's pose as its parent.
    DEV_ASSERT(!poseData.histState.GetPose().HasParent(), "VisionSystem.UpdatePoseData.HistStatePoseHasParent");
    DEV_ASSERT(poseData.cameraPose.IsChildOf(poseData.histState.GetPose()),
               "VisionSystem.UpdatePoseData.BadPoseDataCameraPose");
    _poseData.histState.SetPoseParent(_poseOrigin);
  }
  
  if(_wasCalledOnce) {
    _havePrevPoseData = true;
  } else {
    _wasCalledOnce = true;
  }
  
  return RESULT_OK;
} // UpdateRobotState()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Radians VisionSystem::GetCurrentHeadAngle()
{
  return _poseData.histState.GetHeadAngle_rad();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Radians VisionSystem::GetPreviousHeadAngle()
{
  return _prevPoseData.histState.GetHeadAngle_rad();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::CheckMailbox(VisionProcessingResult& result)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if(_results.empty()) {
    return false;
  } else {
    std::swap(result, _results.front());
    _results.pop();
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::IsInitialized() const
{
  bool retVal = _isInitialized;
#   if ANKI_COZMO_USE_MATLAB_VISION
  retVal &= _matlab.ep != NULL;
#   endif
  return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u8 VisionSystem::ComputeMean(Vision::ImageCache& imageCache, const s32 sampleInc)
{
  DEV_ASSERT(sampleInc >= 1, "VisionSystem.ComputeMean.BadIncrement");
  
  const Vision::Image& inputImageGray = imageCache.GetGray();
  s32 sum=0;
  const s32 numRows = inputImageGray.GetNumRows();
  const s32 numCols = inputImageGray.GetNumCols();
  for(s32 i=0; i<numRows; i+=sampleInc)
  {
    const u8* image_i = inputImageGray.GetRow(i);
    for(s32 j=0; j<numCols; j+=sampleInc)
    {
      sum += image_i[j];
    }
  }
  // Consider that in the loop above, we always start at row 0, and we always start at column 0
  const s32 count = ((numRows + sampleInc - 1) / sampleInc) *
                    ((numCols + sampleInc - 1) / sampleInc);

  const u8 mean = Util::numeric_cast_clamped<u8>(sum/count);
  return mean;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionSystem::UpdateMeteringRegions(TimeStamp_t currentTime_ms, DetectionRectsByMode&& detectionsByMode)
{
  const bool meterFromChargerOnly = ShouldProcessVisionMode(VisionMode::MeteringFromChargerOnly);
  
  // Before we do image quality / auto exposure, swap in the detections for any mode that actually ran,
  // in case we need them for metering
  // - if a mode ran but detected nothing, then an empty vector of rects will be swapped in
  // - if a mode did not run, the previous detections for that mode will persist until it runs again
  for(auto & current : detectionsByMode)
  {
    const VisionMode mode = current.first;
    if(meterFromChargerOnly && (mode != VisionMode::DetectingMarkers))
    {
      continue;
    }
    std::swap(_meteringRegions[mode], current.second);
  }
  
  // Clear out any stale "previous" detections for modes that are completely disabled by the current schedule
  // Also remove empty vectors of rectangles that got swapped in above
  auto iter = _meteringRegions.begin();
  while(iter != _meteringRegions.end())
  {
    const VisionMode mode = iter->first;
    if(iter->second.empty() || !IsModeScheduledToEverRun(mode)) {
      iter = _meteringRegions.erase(iter);
    }
    else if(meterFromChargerOnly && (mode != VisionMode::DetectingMarkers)) {
      iter = _meteringRegions.erase(iter);
    }
    else {
      ++iter;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::CheckImageQuality(Vision::ImageCache& imageCache)
{
# define DEBUG_IMAGE_HISTOGRAM 0
  
  const Vision::Image& inputImage = imageCache.GetGray();
  
  // Compute the exposure we would like to have
  f32 exposureAdjFrac = 1.f;
  
  Result expResult = RESULT_FAIL;
  if(!kMeterFromDetections || _meteringRegions.empty())
  {
    if(imageCache.GetTimeStamp() <= (_lastMeteringTimestamp_ms + kMeteringHoldTime_ms))
    {
      // Don't update auto exposure for a little while after we lose metered regions
      PRINT_CH_INFO("VisionSystem", "VisionSystem.CheckImageQuality.HoldingExposureAfterRecentMeteredRegions", "");
      return RESULT_OK;
    }
    
    const bool useCycling = ShouldProcessVisionMode(VisionMode::CyclingExposure);
    expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, useCycling, exposureAdjFrac);
  }
  else
  {
    // Give half the weight to the detections, the other half to the rest of the image
    
    // When we have detections to weight, don't use cycling exposures. Just let the detections drive the exposure.
    const bool kUseCycling = false;
    
    _lastMeteringTimestamp_ms = imageCache.GetTimeStamp();
    
    s32 totalMeteringArea = 0;
    for(auto const& entry : _meteringRegions)
    {
      for(auto const& rect : entry.second)
      {
        totalMeteringArea += rect.Area();
      }
    }
    DEV_ASSERT(totalMeteringArea >= 0, "VisionSystem.CheckImageQuality.NegativeROIArea");
    
    if(2*totalMeteringArea < inputImage.GetNumElements())
    {
      const u8 backgroundWeight = Util::numeric_cast<u8>(255.f * static_cast<f32>(totalMeteringArea)/static_cast<f32>(inputImage.GetNumElements()));
      const u8 roiWeight = 255 - backgroundWeight;
      
      Vision::Image weightMask(inputImage.GetNumRows(), inputImage.GetNumCols());
      weightMask.FillWith(backgroundWeight);
      
      for(auto const& entry : _meteringRegions)
      {
        for(auto rect : entry.second) // deliberate copy b/c GetROI can modify rect
        {
          weightMask.GetROI(rect).FillWith(roiWeight);
        }
      }
      
      expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, weightMask, kUseCycling, exposureAdjFrac);
      
      if(DEBUG_IMAGE_HISTOGRAM)
      {
        Vision::ImageRGB dispWeights(weightMask);
        dispWeights.DrawText({1.f,9.f},
                             "F:" + std::to_string(roiWeight) +
                             " B:" + std::to_string(backgroundWeight),
                             NamedColors::RED, 0.5f);
        _currentResult.debugImageRGBs.emplace_back("HistWeights", dispWeights);
      }
    }
    else
    {
      // Detections already make up more than half the image, so they'll already
      // get more focus. Just expose normally
      expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, kUseCycling, exposureAdjFrac);
    }
  }
  
  if(RESULT_OK != expResult)
  {
    PRINT_NAMED_WARNING("VisionSystem.CheckImageQuality.ComputeNewExposureFailed", "");
    return expResult;
  }
  
  if(DEBUG_IMAGE_HISTOGRAM)
  {
    const Vision::ImageBrightnessHistogram& hist = _imagingPipeline->GetHistogram();
    std::vector<u8> values = hist.ComputePercentiles({kLowPercentile, kTargetPercentile, kHighPercentile});
    auto valueIter = values.begin();
    
    Vision::ImageRGB histImg(hist.GetDisplayImage(128));
    histImg.DrawText(Anki::Point2f((s32)hist.GetCounts().size()/3, 12),
                     std::string("L:")  + std::to_string(*valueIter++) +
                     std::string(" M:") + std::to_string(*valueIter++) +
                     std::string(" H:") + std::to_string(*valueIter++),
                     NamedColors::RED, 0.45f);
    _currentResult.debugImageRGBs.emplace_back("ImageHist", histImg);
    
  } // if(DEBUG_IMAGE_HISTOGRAM)
  
  // Default: we checked the image quality and it's fine (no longer "Unchecked")
  // Desired exposure settings are what they already were.
  _currentResult.imageQuality = ImageQuality::Good;
  
  s32 desiredExposureTime_ms = _currentCameraParams.exposureTime_ms;
  f32 desiredGain = _currentCameraParams.gain;
  
  enum class AutoExpMode : u8 {
    MinExposure,  // Prefer keeping exposure low to avoid motion blur
    MinGain,      // Prefer keeping gain low to make less noisy/grainy images
  };
  
  static AutoExpMode aeMode_prev = (ShouldProcessVisionMode(VisionMode::MinGainAutoExposure) ?
                                    AutoExpMode::MinGain : AutoExpMode::MinExposure);
  
  const AutoExpMode aeMode = (ShouldProcessVisionMode(VisionMode::MinGainAutoExposure) ?
                              AutoExpMode::MinGain : AutoExpMode::MinExposure);
  
  if(aeMode != aeMode_prev)
  {
    // Switching modes: try to preserve roughly the same product of gain and exposure
    const f32 prod = static_cast<f32>(_currentCameraParams.exposureTime_ms) * _currentCameraParams.gain;
    
    switch(aeMode)
    {
      case AutoExpMode::MinGain:
      {
        // Make the new exposure the same fraction of its max that gain currently is.
        // Then pick a new gain to "match".
        const f32 frac = _currentCameraParams.gain / GetMaxCameraGain();
        desiredExposureTime_ms = static_cast<s32>(std::round(frac * static_cast<f32>(GetMaxCameraExposureTime_ms())));
        desiredExposureTime_ms = std::max(desiredExposureTime_ms, GetMinCameraExposureTime_ms());
        desiredGain = Util::Clamp(prod/static_cast<f32>(desiredExposureTime_ms),
                                  GetMinCameraGain(), GetMaxCameraGain());
        break;
      }
        
      case AutoExpMode::MinExposure:
      {
        // Make the new gain the same fraction of its max that exposure currently is.
        // Then pick a new exposure to "match".
        const f32 frac = (static_cast<f32>(_currentCameraParams.exposureTime_ms) /
                          static_cast<f32>(GetMaxCameraExposureTime_ms()));
        desiredGain = std::max(frac*GetMaxCameraGain(), GetMinCameraGain());
        desiredExposureTime_ms = Util::Clamp(static_cast<s32>(std::round(prod / desiredGain)),
                                             GetMinCameraExposureTime_ms(), GetMaxCameraExposureTime_ms());
        break;
      }
    }
    
    PRINT_CH_INFO(kLogChannelName, "VisionSystem.CheckImageQuality.ToggleExpMode",
                  "Mode:%s, Gain:%.3f->%.3f, Exp:%d->%dms",
                  (aeMode_prev == AutoExpMode::MinGain ? "MinGain->MinExp" : "MinExp->MinGain"),
                  _currentCameraParams.gain, desiredGain,
                  _currentCameraParams.exposureTime_ms, desiredExposureTime_ms);
    
    aeMode_prev = aeMode;
  }
  
  if(Util::IsFltLT(exposureAdjFrac, 1.f))
  {
    // Want to bring brightness down
    bool madeAdjustment = false;
    switch(aeMode)
    {
      case AutoExpMode::MinExposure:
        if(_currentCameraParams.exposureTime_ms > _minCameraExposureTime_ms)
        {
          desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
          desiredExposureTime_ms = std::max(_minCameraExposureTime_ms, desiredExposureTime_ms);
          madeAdjustment = true;
        }
        else if(Util::IsFltGT(_currentCameraParams.gain, _minCameraGain))
        {
          // Already at min exposure time; reduce gain
          desiredGain *= exposureAdjFrac;
          desiredGain = std::max(_minCameraGain, desiredGain);
          madeAdjustment = true;
        }
        break;
        
      case AutoExpMode::MinGain:
        if(Util::IsFltGT(_currentCameraParams.gain, _minCameraGain))
        {
          desiredGain *= exposureAdjFrac;
          desiredGain = std::max(_minCameraGain, desiredGain);
          madeAdjustment = true;
        }
        else if(_currentCameraParams.exposureTime_ms > _minCameraExposureTime_ms)
        {
          // Already at min gain time; reduce exposure
          desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
          desiredExposureTime_ms = std::max(_minCameraExposureTime_ms, desiredExposureTime_ms);
          madeAdjustment = true;
        }
        break;
    }
    
    if(!madeAdjustment)
    {
      const u8 currentLowValue = _imagingPipeline->GetHistogram().ComputePercentile(kLowPercentile);
      if(currentLowValue > kTooBrightValue)
      {
        // Both exposure and gain are as low as they can go and the low value in the
        // image is still too high: it's too bright!
        _currentResult.imageQuality = ImageQuality::TooBright;
      }
    }
  }
  else if(Util::IsFltGT(exposureAdjFrac, 1.f))
  {
    // Want to bring brightness up
    bool madeAdjustment = false;
    switch(aeMode)
    {
      case AutoExpMode::MinExposure:
        if(Util::IsFltLT(_currentCameraParams.gain, _maxCameraGain))
        {
          desiredGain *= exposureAdjFrac;
          desiredGain = std::min(_maxCameraGain, desiredGain);
          madeAdjustment = true;
        }
        else if(_currentCameraParams.exposureTime_ms < _maxCameraExposureTime_ms)
        {
          // Already at max gain; increase exposure
          desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
          desiredExposureTime_ms = std::min(_maxCameraExposureTime_ms, desiredExposureTime_ms);
          madeAdjustment = true;
        }
        break;
        
      case AutoExpMode::MinGain:
        if(_currentCameraParams.exposureTime_ms < _maxCameraExposureTime_ms)
        {
          desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
          desiredExposureTime_ms = std::min(_maxCameraExposureTime_ms, desiredExposureTime_ms);
          madeAdjustment = true;
        }
        else if(Util::IsFltLT(_currentCameraParams.gain, _maxCameraGain))
        {
          // Already at max exposure; increase gain
          desiredGain *= exposureAdjFrac;
          desiredGain = std::min(_maxCameraGain, desiredGain);
          madeAdjustment = true;
        }
        break;
    }
    
    if(!madeAdjustment)
    {
      const u8 currentHighValue = _imagingPipeline->GetHistogram().ComputePercentile(kHighPercentile);
      if(currentHighValue < kTooDarkValue)
      {
        // Both exposure and gain are as high as they can go and the high value in the
        // image is still too low: it's too dark!
        _currentResult.imageQuality = ImageQuality::TooDark;
      }
    }
  }
    
  _currentResult.cameraParams.exposureTime_ms = desiredExposureTime_ms;
  _currentResult.cameraParams.gain = desiredGain;

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::CheckWhiteBalance(Vision::ImageCache& imageCache)
{
  DEV_ASSERT(imageCache.HasColor(), "VisionSystem.CheckWhiteBalance.NoColor");
  const Vision::ImageRGB& img = imageCache.GetRGB();

  f32 adjR = 1.f, adjB = 1.f;
  
  Result result = _imagingPipeline->ComputeWhiteBalanceAdjustment(img, adjR, adjB);
  
  if(RESULT_OK != result) {
    PRINT_NAMED_ERROR("VisionSystem.CheckWhiteBalance.ComputeWhiteBalanceAdjustmentFailed", "");
    return result;
  }
  
  // Actually report the new white balance gains, based on the current values and the
  // computed adjustments
  _currentResult.cameraParams.whiteBalanceGainR = _currentCameraParams.whiteBalanceGainR * adjR;
  _currentResult.cameraParams.whiteBalanceGainG = _currentCameraParams.whiteBalanceGainG; // Constant!
  _currentResult.cameraParams.whiteBalanceGainB = _currentCameraParams.whiteBalanceGainB * adjB;
    
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::CanAddNamedFace() const
{
  return _faceTracker->CanAddNamedFace();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::AssignNameToFace(Vision::FaceID_t faceID, const std::string& name, Vision::FaceID_t mergeWithID)
{
  if(!_isInitialized) {
    PRINT_NAMED_WARNING("VisionSystem.AssignNameToFace.NotInitialized",
                        "Cannot assign name '%s' to face ID %d before being initialized",
                        name.c_str(), faceID);
    return RESULT_FAIL;
  }
  
  DEV_ASSERT(_faceTracker != nullptr, "VisionSystem.AssignNameToFace.NullFaceTracker");
  
  return _faceTracker->AssignNameToID(faceID, name, mergeWithID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::EraseFace(Vision::FaceID_t faceID)
{
  return _faceTracker->EraseFace(faceID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionSystem::SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
                                            Vision::FaceID_t forFaceID,
                                            s32 numEnrollments)
{
  _faceTracker->SetFaceEnrollmentMode(pose, forFaceID, numEnrollments);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionSystem::EraseAllFaces()
{
  _faceTracker->EraseAllFaces();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<Vision::LoadedKnownFace> VisionSystem::GetEnrolledNames() const
{
  return _faceTracker->GetEnrolledNames();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName,
                                Vision::RobotRenamedEnrolledFace& renamedFace)
{
  return _faceTracker->RenameFace(faceID, oldName, newName, renamedFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::Image BlackOutRects(const Vision::Image& img, const std::vector<Anki::Rectangle<s32>>& rects)
{
  // Black out detected markers so we don't find faces in them
  Vision::Image maskedImage;
  img.CopyTo(maskedImage);
  
  DEV_ASSERT(maskedImage.GetTimestamp() == img.GetTimestamp(), "VisionSystem.DetectFaces.BadImageTimestamp");
  
  for(auto rect : rects) // Deliberate copy because GetROI can modify 'rect'
  {
    Vision::Image roi = maskedImage.GetROI(rect);
    
    if(!roi.IsEmpty())
    {
      roi.FillWith(0);
    }
  }
  
  return maskedImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectFaces(Vision::ImageCache& imageCache,
                                 std::vector<Anki::Rectangle<s32>>& detectionRects)
{
  DEV_ASSERT(_faceTracker != nullptr, "VisionSystem.DetectFaces.NullFaceTracker");
 
  const Vision::Image& grayImage = imageCache.GetGray();

  /*
  // Periodic printouts of face tracker timings
  static TimeStamp_t lastProfilePrint = 0;
  if(grayImage.GetTimestamp() - lastProfilePrint > 2000) {
    _faceTracker->PrintTiming();
    lastProfilePrint = grayImage.GetTimestamp();
  }
   */
  
  if(_faceTracker == nullptr) {
    PRINT_NAMED_ERROR("VisionSystem.Update.NullFaceTracker",
                      "In detecting faces mode, but face tracker is null.");
    return RESULT_FAIL;
  }
  
  // If we've moved too much, reset the tracker so we don't accidentally mistake
  // one face for another. (If one face it was tracking from the last image is
  // now on top of a nearby face in the image, the tracker can't tell if that's
  // because the face moved or the camera moved.)
  const bool hasHeadMoved = !_poseData.IsHeadAngleSame(_prevPoseData, DEG_TO_RAD(kFaceTrackingMaxHeadAngleChange_deg));
  const bool hasBodyMoved = !_poseData.IsBodyPoseSame(_prevPoseData,
                                                      DEG_TO_RAD(kFaceTrackingMaxBodyAngleChange_deg),
                                                      kFaceTrackingMaxPoseChange_mm);
  if(hasHeadMoved || hasBodyMoved)
  {
    PRINT_NAMED_DEBUG("VisionSystem.Update.ResetFaceTracker",
                      "HeadMoved:%d BodyMoved:%d", hasHeadMoved, hasBodyMoved);
    _faceTracker->Reset();
  }
  
  if(!detectionRects.empty())
  {
    // Black out previous detections so we don't find faces in them
    Vision::Image maskedImage = BlackOutRects(grayImage, detectionRects);
    
#     if DEBUG_FACE_DETECTION
    //_currentResult.debugImages.push_back({"MaskedFaceImage", maskedImage});
#     endif
    
    _faceTracker->Update(maskedImage, _currentResult.faces, _currentResult.updatedFaceIDs);
  } else {
    // Nothing already detected, so nothing to black out before looking for faces
    _faceTracker->Update(grayImage, _currentResult.faces, _currentResult.updatedFaceIDs);
  }
  
  for(auto faceIter = _currentResult.faces.begin(); faceIter != _currentResult.faces.end(); ++faceIter)
  {
    auto & currentFace = *faceIter;
    
    DEV_ASSERT(currentFace.GetTimeStamp() == grayImage.GetTimestamp(), "VisionSystem.DetectFaces.BadFaceTimestamp");
    
    detectionRects.emplace_back((s32)std::round(faceIter->GetRect().GetX()),
                                (s32)std::round(faceIter->GetRect().GetY()),
                                (s32)std::round(faceIter->GetRect().GetWidth()),
                                (s32)std::round(faceIter->GetRect().GetHeight()));
    
    // Make head pose w.r.t. the historical world origin
    Pose3d headPose = currentFace.GetHeadPose();
    headPose.SetParent(_poseData.cameraPose);
    headPose = headPose.GetWithRespectToRoot();

    DEV_ASSERT(headPose.IsChildOf(_poseOrigin), "VisionSystem.DetectFaces.BadHeadPoseParent");
    
    // Leave faces in the output result with no parent pose (b/c we will assume they are w.r.t. the origin)
    headPose.ClearParent();
    
    currentFace.SetHeadPose(headPose);
  }
  
  return RESULT_OK;
} // DetectFaces()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectPets(Vision::ImageCache& imageCache,
                                std::vector<Anki::Rectangle<s32>>& detections)
{
  const Vision::Image& grayImage = imageCache.GetGray();
  Result result = RESULT_FAIL;
  
  if(detections.empty())
  {
    result = _petTracker->Update(grayImage, _currentResult.pets);
  }
  else
  {
    // Don't look for pets where we've already found something else
    Vision::Image maskedImage = BlackOutRects(grayImage, detections);
    result = _petTracker->Update(maskedImage, _currentResult.pets);
  }
  
  if(RESULT_OK != result) {
    PRINT_NAMED_WARNING("VisionSystem.DetectPets.PetTrackerUpdateFailed", "");
  }
  
  for(auto const& pet : _currentResult.pets)
  {
    detections.emplace_back((s32)std::round(pet.GetRect().GetX()),
                            (s32)std::round(pet.GetRect().GetY()),
                            (s32)std::round(pet.GetRect().GetWidth()),
                            (s32)std::round(pet.GetRect().GetHeight()));
  }
  return result;
  
} // DetectPets()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectMotion(Vision::ImageCache& imageCache)
{

  Result result = RESULT_OK;
  
  _motionDetector->Detect(imageCache, _poseData, _prevPoseData,
                          _currentResult.observedMotions, _currentResult.debugImageRGBs);
  
  return result;
  
} // DetectMotion()

Result VisionSystem::UpdateOverheadMap(Vision::ImageCache& imageCache)
{
  DEV_ASSERT(imageCache.HasColor(), "VisionSystem.UpdateOverheadMap.NoColor");
  const Vision::ImageRGB& image = imageCache.GetRGB();
  Result result = _overheadMap->Update(image, _poseData, _currentResult.debugImageRGBs);
  return result;
}

Result VisionSystem::UpdateGroundPlaneClassifier(Vision::ImageCache& imageCache)
{
  DEV_ASSERT(imageCache.HasColor(), "VisionSystem.UpdateGroundPlaneClassifier.NoColor");
  const Vision::ImageRGB& image = imageCache.GetRGB();
  Result result = _groundPlaneClassifier->Update(image, _poseData, _currentResult.debugImageRGBs,
                                                 _currentResult.visualObstacles);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectLaserPoints(Vision::ImageCache& imageCache)
{
  const bool isDarkExposure = (Util::IsNear(_currentCameraParams.exposureTime_ms, _minCameraExposureTime_ms) &&
                               Util::IsNear(_currentCameraParams.gain, _minCameraGain));
  
  Result result = _laserPointDetector->Detect(imageCache, _poseData, isDarkExposure,
                                              _currentResult.laserPoints,
                                              _currentResult.debugImageRGBs);
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectIllumination(Vision::ImageCache& imageCache)
{
  Result result = _illuminationDetector->Detect(imageCache, 
                                                _poseData, 
                                                _currentResult.illumination);
  return result;
}

#if 0
#pragma mark --- Public VisionSystem API Implementations ---
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string VisionSystem::GetCurrentModeName() const {
  return VisionSystem::GetModeName(_mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string VisionSystem::GetModeName(Util::BitFlags32<VisionMode> mode) const
{
  std::string retStr("");
  for (auto modeIter = VisionMode::Idle; modeIter < VisionMode::Count; ++modeIter)
  {
    if (mode.IsBitFlagSet(modeIter))
    {
      if(!retStr.empty()) {
        retStr += "+";
      }
      retStr += EnumToString(modeIter);
    }
  }
  return retStr;
  
} // GetModeName()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionMode VisionSystem::GetModeFromString(const std::string& str) const
{
  return VisionModeFromString(str.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::ApplyCLAHE(Vision::ImageCache& imageCache,
                                const MarkerDetectionCLAHE useCLAHE,
                                Vision::Image& claheImage)
{
  const Vision::ImageCache::Size whichSize = imageCache.GetSize(kMarkerDetector_ScaleMultiplier,
                                                                Vision::ResizeMethod::Linear);
  const Vision::Image& inputImageGray = imageCache.GetGray(whichSize);
  
  switch(useCLAHE)
  {
    case MarkerDetectionCLAHE::Off:
      _currentUseCLAHE = false;
      break;
      
    case MarkerDetectionCLAHE::On:
    case MarkerDetectionCLAHE::Both:
      _currentUseCLAHE = true;
      break;
      
    case MarkerDetectionCLAHE::Alternating:
      _currentUseCLAHE = !_currentUseCLAHE;
      break;
      
    case MarkerDetectionCLAHE::WhenDark:
    {
      // Use CLAHE on the current image if it is dark enough
      static const s32 subSample = 3;
      const s32 numRows = inputImageGray.GetNumRows();
      const s32 numCols = inputImageGray.GetNumCols();
      // Consider that in the loop below, we always start at row 0, and we always start at column 0
      const s32 count = ((numRows + subSample - 1) / subSample) *
                        ((numCols + subSample - 1) / subSample);
      const s32 threshold = kClaheWhenDarkThreshold * count;

      _currentUseCLAHE = true;
      s32 meanValue = 0;
      for(s32 i=0; i<numRows; i+=subSample)
      {
        const u8* img_i = inputImageGray.GetRow(i);
        for(s32 j=0; j<numCols; j+=subSample)
        {
          meanValue += img_i[j];
        }
        if (meanValue >= threshold)
        {
          // Image is not dark enough; early out
          _currentUseCLAHE = false;
          break;
        }
      }
      break;
    }
      
    case MarkerDetectionCLAHE::Count:
      assert(false);
      break;
  }
  
  if(!_currentUseCLAHE)
  {
    // Nothing to do: not currently using CLAHE
    return RESULT_OK;
  }
  
  if(_lastClaheTileSize != kClaheTileSize) {
    PRINT_NAMED_DEBUG("VisionSystem.Update.ClaheTileSizeUpdated",
                      "%d -> %d", _lastClaheTileSize, kClaheTileSize);
    
    _clahe->setTilesGridSize(cv::Size(kClaheTileSize, kClaheTileSize));
    _lastClaheTileSize = kClaheTileSize;
  }
  
  if(_lastClaheClipLimit != kClaheClipLimit) {
    PRINT_NAMED_DEBUG("VisionSystem.Update.ClaheClipLimitUpdated",
                      "%d -> %d", _lastClaheClipLimit, kClaheClipLimit);
    
    _clahe->setClipLimit(kClaheClipLimit);
    _lastClaheClipLimit = kClaheClipLimit;
  }
  
  Tic("CLAHE");
  _clahe->apply(inputImageGray.get_CvMat_(), claheImage.get_CvMat_());
  
  if(kPostClaheSmooth > 0)
  {
    s32 kSize = 3*kPostClaheSmooth;
    if(kSize % 2 == 0) {
      ++kSize; // Make sure it's odd
    }
    cv::GaussianBlur(claheImage.get_CvMat_(), claheImage.get_CvMat_(),
                     cv::Size(kSize,kSize), kPostClaheSmooth);
  }
  else if(kPostClaheSmooth < 0)
  {
    static Vision::Image temp(claheImage.GetNumRows(), claheImage.GetNumCols());
    claheImage.BoxFilter(temp, -kPostClaheSmooth);
    std::swap(claheImage, temp);
  }
  Toc("CLAHE");
  
  if(DEBUG_DISPLAY_CLAHE_IMAGE) {
    _currentResult.debugImageRGBs.push_back({"ImageCLAHE", claheImage});
  }
  
  claheImage.SetTimestamp(inputImageGray.GetTimestamp()); // make sure to preserve timestamp!
  
  return RESULT_OK;
  
} // ApplyCLAHE()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::DetectMarkersWithCLAHE(Vision::ImageCache& imageCache,
                                            const Vision::Image& claheImage,
                                            std::vector<Anki::Rectangle<s32>>& detectionRects,
                                            MarkerDetectionCLAHE useCLAHE)
{
  Result lastResult = RESULT_OK;

  // Currently assuming we detect markers first, so we won't make use of anything already detected
  DEV_ASSERT(detectionRects.empty(), "VisionSystem.DetectMarkersWithCLAHE.ExpectingEmptyDetectionRects");
  
  const auto whichSize = imageCache.GetSize(kMarkerDetector_ScaleMultiplier, Vision::ResizeMethod::Linear);
  
  switch(useCLAHE)
  {
    case MarkerDetectionCLAHE::Off:
    {
      lastResult = _markerDetector->Detect(imageCache.GetGray(whichSize), _currentResult.observedMarkers);
      break;
    }
      
    case MarkerDetectionCLAHE::On:
    {
      DEV_ASSERT(!claheImage.IsEmpty(), "VisionSystem.DetectMarkersWithCLAHE.useOn.ImageIsEmpty");
      
      lastResult = _markerDetector->Detect(claheImage, _currentResult.observedMarkers);
      
      break;
    }
      
    case MarkerDetectionCLAHE::Both:
    {
      DEV_ASSERT(!claheImage.IsEmpty(), "VisionSystem.DetectMarkersWithCLAHE.useBoth.ImageIsEmpty");
      
      // First run will put quads into detectionRects
      lastResult = _markerDetector->Detect(imageCache.GetGray(whichSize), _currentResult.observedMarkers);
      
      if(RESULT_OK == lastResult)
      {
        // Second run will white out existing markerQuads (so we don't
        // re-detect) and also add new ones
        lastResult = _markerDetector->Detect(claheImage, _currentResult.observedMarkers);
      }
      
      break;
    }
      
    case MarkerDetectionCLAHE::Alternating:
    {
      if(_currentUseCLAHE) {
        DEV_ASSERT(!claheImage.IsEmpty(), "VisionSystem.DetectMarkersWithCLAHE.useAlternating.ImageIsEmpty");
        lastResult = _markerDetector->Detect(claheImage, _currentResult.observedMarkers);
      }
      else {
        lastResult = _markerDetector->Detect(imageCache.GetGray(whichSize), _currentResult.observedMarkers);
      }
      
      break;
    }
      
    case MarkerDetectionCLAHE::WhenDark:
    {
      // NOTE: _currentUseCLAHE should have been set based on image brightness already
      
      if(_currentUseCLAHE)
      {
        DEV_ASSERT(!claheImage.IsEmpty(), "VisionSystem.DetectMarkersWithCLAHE.useWhenDark.ImageIsEmpty");
        lastResult = _markerDetector->Detect(claheImage, _currentResult.observedMarkers);
      }
      else {
        lastResult = _markerDetector->Detect(imageCache.GetGray(whichSize), _currentResult.observedMarkers);
      }
      
      break;
    }
      
    case MarkerDetectionCLAHE::Count:
      assert(false); // should never get here
      break;
  }
  
  const bool meterFromChargerOnly = ShouldProcessVisionMode(VisionMode::MeteringFromChargerOnly);
  
  auto markerIter = _currentResult.observedMarkers.begin();
  while(markerIter != _currentResult.observedMarkers.end())
  {
    auto & marker = *markerIter;
    
    if(meterFromChargerOnly && (marker.GetCode() != Vision::MARKER_CHARGER_HOME))
    {
      markerIter = _currentResult.observedMarkers.erase(markerIter);
      continue;
    }
    
    // Add the bounding rect of the (unwarped) marker to the detection rectangles
    Quad2f scaledCorners(marker.GetImageCorners());
    if(kMarkerDetector_ScaleMultiplier != 1)
    {
      for(auto & corner : scaledCorners)
      {
        corner *= kMarkerDetector_ScaleMultiplier;
      }
      
      marker.SetImageCorners(scaledCorners);
    }
    
    // Note that the scaled corners might get changed slightly by rolling shutter warping
    // below, making the detection rect slightly inaccurate, but these rectangles don't need
    // to be super accurate. And we'd rather still report something being detected here
    // even if the warping pushes a corner OOB, thereby removing the marker from the
    // actual reported list.
    detectionRects.emplace_back(scaledCorners);
    
    // Instead of correcting the entire image only correct the quads
    // Apply the appropriate shift to each of the corners of the quad to get a shifted quad
    if(_doRollingShutterCorrection)
    {
      bool allCornersInBounds = true;
      for(auto & corner : scaledCorners)
      {
        const s32 fullNumRows = imageCache.GetNumRows(Vision::ImageCache::Size::Full);
        const s32 fullNumCols = imageCache.GetNumCols(Vision::ImageCache::Size::Full);
        const int warpIndex = std::floor(corner.y() / (fullNumRows / _rollingShutterCorrector.GetNumDivisions()));
        DEV_ASSERT_MSG(warpIndex >= 0 && warpIndex < _rollingShutterCorrector.GetPixelShifts().size(),
                       "VisionSystem.DetectMarkersWithCLAHE.WarpIndexOOB", "Index:%d Corner y:%f",
                       warpIndex, corner.y());
        
        const Vec2f& pixelShift = _rollingShutterCorrector.GetPixelShifts().at(warpIndex);
        corner -= pixelShift;

        if(Util::IsFltLTZero(corner.x()) ||
           Util::IsFltLTZero(corner.y()) ||
           Util::IsFltGE(corner.x(), (f32)fullNumCols) ||
           Util::IsFltGE(corner.y(), (f32)fullNumRows))
        {
          // Warped corner is outside image bounds. Just drop this entire marker. Technically, we could still
          // probably estimate its pose just fine, but other things later may expect all corners to be in bounds
          // so easier just not to risk it.
          allCornersInBounds = false;
          break; // no need to warp remaining corners once any one is OOB
        }
      }
      
      if(!allCornersInBounds)
      {
        // Remove this OOB marker from the list entirely
        PRINT_CH_DEBUG(kLogChannelName, "VisionSystem.DetectMarkersWithCLAHE.RemovingMarkerOOB",
                       "%s", Vision::MarkerTypeStrings[marker.GetCode()]);
        markerIter = _currentResult.observedMarkers.erase(markerIter);
        continue;
      }
      
      marker.SetImageCorners(scaledCorners); // "scaled" corners are now the warped corners
    }
    
    ++markerIter;
  }
  
  return lastResult;
  
} // DetectMarkersWithCLAHE()

void VisionSystem::CheckForNeuralNetResults()
{
  std::list<Vision::SalientPoint> salientPoints;
  const bool resultReady = _neuralNetRunner->GetDetections(salientPoints);
  if(resultReady)
  {
    VisionProcessingResult detectionResult;
    detectionResult.timestamp = _neuralNetRunnerTimestamp;
    detectionResult.modesProcessed.SetBitFlag(VisionMode::RunningNeuralNet, true);
    std::swap(detectionResult.salientPoints, salientPoints);
    
    _mutex.lock();
    _results.emplace(std::move(detectionResult));
    _mutex.unlock();
  }
}

void VisionSystem::UpdateRollingShutter(const VisionPoseData& poseData, const Vision::ImageCache& imageCache)
{
  // If we've already updated the corrector at this timestamp, don't have to do it again
  if(!_doRollingShutterCorrection || (imageCache.GetTimeStamp() <= _lastRollingShutterCorrectionTime))
  {
    return;
  }

  Tic("RollingShutterComputePixelShifts");
  s32 numRows = imageCache.GetNumRows(Vision::ImageCache::Size::Full);
  _rollingShutterCorrector.ComputePixelShifts(poseData, _prevPoseData, numRows);
  Toc("RollingShutterComputePixelShifts");
  _lastRollingShutterCorrectionTime = imageCache.GetTimeStamp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::Update(const VisionPoseData&      poseData,
                            const Vision::ImageRGB&    image,
                            const f32                  fullScaleFactor,
                            const Vision::ResizeMethod fullScaleMethod)
{
  _imageCache->Reset(image, fullScaleFactor, fullScaleMethod);
  
  return Update(poseData, *_imageCache);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This is the regular Update() call
Result VisionSystem::Update(const VisionPoseData& poseData, Vision::ImageCache& imageCache)
{
  Result lastResult = RESULT_OK;
  
  if(!_isInitialized || !_camera.IsCalibrated())
  {
    PRINT_NAMED_WARNING("VisionSystem.Update.NotReady",
                        "Must be initialized and have calibrated camera to Update");
    return RESULT_FAIL;
  }

#if REMOTE_CONSOLE_ENABLED
  // Make it possible to toggle vision modes using console vars:
  if(kToggleVisionMode > 0)
  {
    const VisionMode mode = static_cast<VisionMode>(kToggleVisionMode);
    bool enable = true;
    if(_mode.IsBitFlagSet(mode))
    {
      enable = false;
    }
    
    PRINT_CH_INFO(kLogChannelName, "VisionSystem.Update.TogglingVisionModeByConsoleVar",
                  "%s mode %s", (enable ? "Enabling" : "Disabling"), EnumToString(mode));
    EnableMode(mode, enable);
    kToggleVisionMode = 0;
  }
#endif
  
  _frameNumber++;
  
  // Set up the results for this frame:
  VisionProcessingResult result;
  result.timestamp = imageCache.GetTimeStamp();
  result.imageQuality = ImageQuality::Unchecked;
  result.cameraParams.exposureTime_ms = -1;
  std::swap(result, _currentResult);
  
  auto& visionModesProcessed = _currentResult.modesProcessed;
  visionModesProcessed.ClearFlags();
  
  // Process mode flags and schedules
  while(!_nextModes.empty())
  {
    const auto& mode = _nextModes.front();
    EnableMode(mode.first, mode.second);
    _nextModes.pop();
  }
  
  while(!_nextSchedules.empty())
  {
    const auto& entry = _nextSchedules.front();

    const bool isPush = entry.first;
    if(isPush)
    {
      const AllVisionModesSchedule& schedule = entry.second;
      _modeScheduleStack.push_front(schedule);
    }
    else if(_modeScheduleStack.size() > 1)
    {
      _modeScheduleStack.pop_front();
    }
    else
    {
      PRINT_NAMED_WARNING("VisionSystem.Update.NotPoppingLastScheduleInStack", "");
    }
    
    _nextSchedules.pop();
  }

  // Store the new robot state and keep a copy of the previous one
  UpdatePoseData(poseData);
  
  bool& cameraParamsRequested = _nextCameraParams.first;
  if(cameraParamsRequested)
  {
    _currentCameraParams = _nextCameraParams.second;
    cameraParamsRequested = false;
  }
  
  if(IsModeEnabled(VisionMode::Idle))
  {
    // Push the empty result and bail
    _mutex.lock();
    _results.push(_currentResult);
    _mutex.unlock();
    return RESULT_OK;
  }
  
  if(kVisionSystemSimulatedDelay_ms > 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(kVisionSystemSimulatedDelay_ms));
  }
  
  // Begin image processing
  // Apply CLAHE if enabled:
  DEV_ASSERT(kUseCLAHE_u8 < Util::EnumToUnderlying(MarkerDetectionCLAHE::Count),
             "VisionSystem.ApplyCLAHE.BadUseClaheVal");
  MarkerDetectionCLAHE useCLAHE = static_cast<MarkerDetectionCLAHE>(kUseCLAHE_u8);
  Vision::Image claheImage;
  
  // Note: this will do nothing and leave claheImage empty if CLAHE is disabled
  // entirely or for this frame.
  lastResult = ApplyCLAHE(imageCache, useCLAHE, claheImage);
  ANKI_VERIFY(RESULT_OK == lastResult, "VisionSystem.Update.FailedCLAHE", "ApplyCLAHE supposedly has no failure mode");
  
  if(ShouldProcessVisionMode(VisionMode::ComputingStatistics))
  {
    Tic("TotalComputingStatistics");
    _currentResult.imageMean = ComputeMean(imageCache, kImageMeanSampleInc);
    visionModesProcessed.SetBitFlag(VisionMode::ComputingStatistics, true);
    Toc("TotalComputingStatistics");
  }

  DetectionRectsByMode detectionsByMode;

  bool anyModeFailures = false;
  
  if(ShouldProcessVisionMode(VisionMode::DetectingMarkers))
  {
    // Marker detection uses rolling shutter compensation
    UpdateRollingShutter(poseData, imageCache);

    Tic("TotalDetectingMarkers");
    lastResult = DetectMarkersWithCLAHE(imageCache, claheImage, detectionsByMode[VisionMode::DetectingMarkers], useCLAHE);
    
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectMarkersFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingMarkers, true);
    }
    Toc("TotalDetectingMarkers");
  }
  
  if(ShouldProcessVisionMode(VisionMode::DetectingFaces))
  {
    Tic("TotalDetectingFaces");
    // NOTE: To use rolling shutter in DetectFaces, call UpdateRollingShutterHere
    // See: VIC-1417 
    // UpdateRollingShutter(poseData, imageCache);
    if((lastResult = DetectFaces(imageCache, detectionsByMode[VisionMode::DetectingFaces])) != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectFacesFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingFaces, true);
    }
    Toc("TotalDetectingFaces");
  }
  
  if(ShouldProcessVisionMode(VisionMode::DetectingPets))
  {
    Tic("TotalDetectingPets");
    if((lastResult = DetectPets(imageCache, detectionsByMode[VisionMode::DetectingPets])) != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectPetsFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingPets, true);
    }
    Toc("TotalDetectingPets");
  }
  
  if(ShouldProcessVisionMode(VisionMode::DetectingMotion))
  {
    Tic("TotalDetectingMotion");
    if((lastResult = DetectMotion(imageCache)) != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectMotionFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingMotion, true);
    }
    Toc("TotalDetectingMotion");
  }

  if (ShouldProcessVisionMode(VisionMode::BuildingOverheadMap))
  {
    if (imageCache.HasColor()) {
      Tic("UpdateOverheadMap");
      lastResult = UpdateOverheadMap(imageCache);
      Toc("UpdateOverheadMap");
      if (lastResult != RESULT_OK) {
        anyModeFailures = true;
      } else {
        visionModesProcessed.SetBitFlag(VisionMode::BuildingOverheadMap, true);
      }
    }
    else {
      PRINT_NAMED_WARNING("VisionSystem.Update.NoColorImage", "Could not process overhead map. No color image!");
    }
  }
  
  if (ShouldProcessVisionMode(VisionMode::DetectingVisualObstacles))
  {
    if (imageCache.HasColor()) {
      Tic("DetectVisualObstacles");
      lastResult = UpdateGroundPlaneClassifier(imageCache);
      Toc("DetectVisualObstacles");
      if (lastResult != RESULT_OK) {
        anyModeFailures = true;
      } else {
        visionModesProcessed.SetBitFlag(VisionMode::DetectingVisualObstacles, true);
      }
    }
    else {
      PRINT_NAMED_WARNING("VisionSystem.Update.NoColorImage", "Could not process visual obstacles. No color image!");
    }
  }

  if(ShouldProcessVisionMode(VisionMode::DetectingOverheadEdges))
  {
    Tic("TotalDetectingOverheadEdges");

    lastResult = _overheadEdgeDetector->Detect(imageCache, _poseData, _currentResult);
    
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectOverheadEdgesFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingOverheadEdges, true);
    }
    Toc("TotalDetectingOverheadEdges");
  }
  
  if(ShouldProcessVisionMode(VisionMode::ComputingCalibration))
  {
    switch(kCalibTargetType)
    {
      case CameraCalibrator::CalibTargetType::CHECKERBOARD:
      {
        lastResult = _cameraCalibrator->ComputeCalibrationFromCheckerboard(_currentResult.cameraCalibration,
                                                                           _currentResult.debugImageRGBs);
        break;
      }
      case CameraCalibrator::CalibTargetType::QBERT:
      case CameraCalibrator::CalibTargetType::INVERTED_BOX:
      {
        // Marker detection needs to have run before trying to do single taret calibration
        DEV_ASSERT(visionModesProcessed.IsBitFlagSet(VisionMode::DetectingMarkers),
                   "VisionSystem.Update.ComputingCalibration.MarkersNotDetected");
        
        CameraCalibrator::CalibTargetType targetType = static_cast<CameraCalibrator::CalibTargetType>(kCalibTargetType);
        lastResult = _cameraCalibrator->ComputeCalibrationFromSingleTarget(targetType,
                                                                           _currentResult.observedMarkers,
                                                                           _currentResult.cameraCalibration,
                                                                           _currentResult.debugImageRGBs);
        break;
      }
    }
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.ComputeCalibrationFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::ComputingCalibration, true);
    }
  }
  
  if(ShouldProcessVisionMode(VisionMode::DetectingLaserPoints))
  {
    // Skip laser point detection if the Laser FeatureGate is disabled.
    // TODO: Remove this once laser feature is enabled (COZMO-11185)
    if(_context->GetFeatureGate()->IsFeatureEnabled(FeatureType::Laser))
    {
      Tic("TotalDetectingLaserPoints");
      if((lastResult = DetectLaserPoints(imageCache)) != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectlaserPointsFailed", "");
        anyModeFailures = true;
      } else {
        visionModesProcessed.SetBitFlag(VisionMode::DetectingLaserPoints, true);
      }
      Toc("TotalDetectingLaserPoints");
    }
  }

  // Check for any objects from the detector. It runs asynchronously, so these objects
  // will be from a different image than the one in the cache and will use their own
  // VisionProcessingResult.
  CheckForNeuralNetResults();
  
  if(ShouldProcessVisionMode(VisionMode::RunningNeuralNet))
  {
    const bool started = _neuralNetRunner->StartProcessingIfIdle(imageCache);
    if(started)
    {
      // Remember the timestamp of the image used to do object detection
      _neuralNetRunnerTimestamp = imageCache.GetTimeStamp();
    }
  }
  
  // Check for illumination state
  if(ShouldProcessVisionMode(VisionMode::DetectingIllumination))
  {
    Tic("DetectingIllumination");
    lastResult = DetectIllumination(imageCache);
    Toc("DetectingIllumination");
    if (lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("VisionSystem.Update.DetectIlluminationFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::DetectingIllumination, true);
    }
  }

  UpdateMeteringRegions(imageCache.GetTimeStamp(), std::move(detectionsByMode));
  
  // NOTE: This should come after any detectors that add things to "detectionRects"
  //       since it meters exposure based on those.
  if(ShouldProcessVisionMode(VisionMode::CheckingQuality))
  {
    Tic("CheckingImageQuality");
    lastResult = CheckImageQuality(imageCache);
    Toc("CheckingImageQuality");
    
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_ERROR("VisionSystem.Update.CheckImageQualityFailed", "");
      anyModeFailures = true;
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::CheckingQuality, true);
    }
  }

  if(ShouldProcessVisionMode(VisionMode::CheckingWhiteBalance))
  {
    if (imageCache.HasColor()) {
      Tic("CheckingWhiteBalance");
      lastResult = CheckWhiteBalance(imageCache);
      Toc("CheckingWhiteBalance");
      
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("VisionSystem.Update.CheckWhiteBalanceFailed", "");
        anyModeFailures = true;
      } else {
        visionModesProcessed.SetBitFlag(VisionMode::CheckingWhiteBalance, true);
      }
    }
    else {
      PRINT_NAMED_WARNING("VisionSystem.Update.NoColorImage", "Could not check white balance. No color image!" );
    }
  }
  
  if(ShouldProcessVisionMode(VisionMode::Benchmarking))
  {
    Tic("Benchmarking");
    const Result benchMarkResult = _benchmark->Update(imageCache);
    Toc("BenchMarking");
    
    if(RESULT_OK != benchMarkResult) {
      PRINT_NAMED_ERROR("VisionSystem.Update.BenchmarkFailed", "");
      // Continue processing, since this should be independent of other modes
    } else {
      visionModesProcessed.SetBitFlag(VisionMode::Benchmarking, true);
    }
  }
  
  if(ShouldProcessVisionMode(VisionMode::SavingImages) && _imageSaver->WantsToSave())
  {
    Tic("SavingImages");
    
    // Check this before calling Save(), since that can modify imageSaver's state
    const bool shouldSaveSensorData = _imageSaver->ShouldSaveSensorData();
    
    const Result saveResult = _imageSaver->Save(imageCache, _frameNumber);
    
    const Result saveSensorResult = (shouldSaveSensorData ? SaveSensorData() : RESULT_OK);
    
    Toc("SavingImages");

    if((RESULT_OK != saveResult) || (RESULT_OK != saveSensorResult)) // || (RESULT_OK != thumbnailResult))
    {
      PRINT_NAMED_ERROR("VisionSystem.Update.SavingImagesFailed", "Image:%s SensorData:%s",
                        (RESULT_OK == saveResult ? "OK" : "FAIL"),
                        (RESULT_OK == saveSensorResult ? "OK" : "FAIL"));
      // Continue processing, since this should be independent of other modes
    }
    else {
      visionModesProcessed.SetBitFlag(VisionMode::SavingImages, true);
    }
  }

  // We've computed everything from this image that we're gonna compute.
  // Push it onto the queue of results all together.
  _mutex.lock();
  _results.push(_currentResult);
  _mutex.unlock();
  
  // Advance the vision mode schedule for next call
  _modeScheduleStack.front().Advance();
  
  return (anyModeFailures ? RESULT_FAIL : RESULT_OK);
} // Update()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::SaveSensorData() const {

  const std::string fullFilename = _imageSaver->GetFullFilename(_frameNumber, "json");

  PRINT_CH_DEBUG(kLogChannelName, "VisionSystem.SaveSensorData.Filename", "Saving to %s",
                 fullFilename.c_str());
  
  std::ofstream outFile(fullFilename);
  if (! outFile.is_open()) {
    PRINT_NAMED_ERROR("VisionSystem.SaveSensorData.CantOpenFile", "Can't open file %s for writing",
                      fullFilename.c_str());
    return RESULT_FAIL;
  }

  Json::Value config;
  {

    const HistRobotState& state = _poseData.histState;
    // prox sensor
    if (state.WasProxSensorValid()) {
      config["proxSensor"] = state.GetProxSensorVal_mm();
    }
    else {
      config["proxSensor"] = -1;
    }

    // cliff sensor
    config["frontLeftCliff"]  = state.WasCliffDetected(CliffSensor::CLIFF_FL);
    config["frontRightCliff"] = state.WasCliffDetected(CliffSensor::CLIFF_FR);
    config["backLeftCliff"]   = state.WasCliffDetected(CliffSensor::CLIFF_BL);
    config["backRightCliff"]  = state.WasCliffDetected(CliffSensor::CLIFF_BR);

    // robot motion flags
    // We don't record "WasCameraMoving" since it's HeadMoving || WheelsMoving
    config["wasCarryingObject"] = state.WasCarryingObject();
    config["wasMoving"] = state.WasMoving();
    config["WasHeadMoving"] = state.WasHeadMoving();
    config["WasLiftMoving"] = state.WasLiftMoving();
    config["WereWheelsMoving"] = state.WereWheelsMoving();
    config["wasPickedUp"] = state.WasPickedUp();

    // head angle
    config["headAngle"] = state.GetHeadAngle_rad();
    // lift angle
    config["liftAngle"] = state.GetLiftAngle_rad();

    // camera exposure, gain, white balance
    // Make sure to get parameters for current image, not next image
    // NOTE: Due to latency between interface call and actual register writes,
    // the so-called current params may not actually be current
    config["requestedCamExposure"] = _currentCameraParams.exposureTime_ms;
    config["requestedCamGain"] = _currentCameraParams.gain;
    config["requestedCamWhiteBalanceRed"] = _currentCameraParams.whiteBalanceGainR;
    config["requestedCamWhiteBalanceGreen"] = _currentCameraParams.whiteBalanceGainG;
    config["requestedCamWhiteBalanceBlue"] = _currentCameraParams.whiteBalanceGainB;

    // image timestamp
    config["imageTimestamp"] = (TimeStamp_t)_currentResult.timestamp;
  }

  Json::StyledWriter writer;
  outFile << writer.write(config);
  outFile.close();

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::ShouldProcessVisionMode(VisionMode mode) const
{
  if (!IsModeEnabled(mode))
  {
    return false;
  }

  if(_modeScheduleStack.empty())
  {
    PRINT_NAMED_ERROR("VisionSystem.ShouldProcessVisionMode.EmptyScheduleStack",
                      "Mode: %s", EnumToString(mode));
    return false;
  }

  // See if it's time to process based on the schedule
  const bool isTimeToProcess = _modeScheduleStack.front().IsTimeToProcess(mode);
  return isTimeToProcess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::IsModeScheduledToEverRun(VisionMode mode) const
{
  return _modeScheduleStack.front().GetScheduleForMode(mode).WillEverRun();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CameraParams VisionSystem::GetCurrentCameraParams() const
{
  // Return nextParams if they have not been set yet otherwise use currentParams
  return (_nextCameraParams.first ? _nextCameraParams.second : _currentCameraParams);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::SetCameraExposureParams(const s32 currentExposureTime_ms,
                                             const s32 minExposureTime_ms,
                                             const s32 maxExposureTime_ms,
                                             const f32 currentGain,
                                             const f32 minGain,
                                             const f32 maxGain,
                                             const GammaCurve& gammaCurve)
{
  // TODO: Expose these x values ("knee locations") somewhere. These are specific to the camera.
  // (So I'm keeping them out of Vision::ImagingPipeline and defined in Cozmo namespace)
  static const std::vector<u8> kKneeLocations{
    0, 8, 16, 24, 32, 40, 48, 64, 80, 96, 112, 128, 144, 160, 192, 224, 255
  };
  
  std::vector<u8> gammaVector(gammaCurve.begin(), gammaCurve.end());
  
  Result result = _imagingPipeline->SetGammaTable(kKneeLocations, gammaVector);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("VisionSystem.SetCameraExposureParams.BadGammaCurve", "");
  }
  
  if(minExposureTime_ms <= 0)
  {
    PRINT_CH_DEBUG(kLogChannelName, "VisionSystem.SetCameraExposureParams.ZeroMinExposureTime",
                   "Will use 1.");
    _minCameraExposureTime_ms = 1;
  }
  else
  {
    _minCameraExposureTime_ms = minExposureTime_ms;
  }
  
  _maxCameraExposureTime_ms = maxExposureTime_ms;
  
  _minCameraGain     = minGain;
  _maxCameraGain     = maxGain;
  
  SetNextCameraExposure(currentExposureTime_ms, currentGain);
  
  PRINT_CH_INFO(kLogChannelName, "VisionSystem.SetCameraExposureParams.Success",
                "Current Gain:%dms Limits:[%d %d], Current Exposure:%.3f Limits:[%.3f %.3f]",
                currentExposureTime_ms, minExposureTime_ms, maxExposureTime_ms,
                currentGain, minGain, maxGain);

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::GetSerializedFaceData(std::vector<u8>& albumData,
                                           std::vector<u8>& enrollData) const
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.GetSerializedFaceData.NullFaceTracker");
  return _faceTracker->GetSerializedData(albumData, enrollData);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::SetSerializedFaceData(const std::vector<u8>& albumData,
                                           const std::vector<u8>& enrollData,
                                           std::list<Vision::LoadedKnownFace>& loadedFaces)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SetSerializedFaceData.NullFaceTracker");
  return _faceTracker->SetSerializedData(albumData, enrollData, loadedFaces);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::LoadFaceAlbum(const std::string& albumName, std::list<Vision::LoadedKnownFace> &loadedFaces)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.LoadFaceAlbum.NullFaceTracker");
  return _faceTracker->LoadAlbum(albumName, loadedFaces);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result VisionSystem::SaveFaceAlbum(const std::string& albumName)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SaveFaceAlbum.NullFaceTracker");
  return _faceTracker->SaveAlbum(albumName);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VisionSystem::SetFaceRecognitionIsSynchronous(bool isSynchronous)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SetFaceRecognitionRunMode.NullFaceTracker");
  _faceTracker->SetRecognitionIsSynchronous(isSynchronous);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::IsExposureValid(s32 exposure) const
{
  const bool inRange = IN_RANGE(exposure, _minCameraExposureTime_ms, _maxCameraExposureTime_ms);
  if(!inRange)
  {
    PRINT_NAMED_WARNING("VisionSystem.IsExposureValid.OOR", "Exposure %dms not in range %dms to %dms",
                        exposure, _minCameraExposureTime_ms, _maxCameraExposureTime_ms);
  }
  return inRange;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionSystem::IsGainValid(f32 gain) const
{
  const bool inRange = IN_RANGE(gain, _minCameraGain, _maxCameraGain);
  if(!inRange)
  {
    PRINT_NAMED_WARNING("VisionSystem.IsGainValid.OOR", "Gain %f not in range %f to %f",
                        gain, _minCameraGain, _maxCameraGain);
  }
  return inRange;
}

void VisionSystem::ClearImageCache()
{
  _imageCache->ReleaseMemory();
}

} // namespace Vector
} // namespace Anki
