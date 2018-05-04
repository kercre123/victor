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
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/linearAlgebra_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/vision/groundPlaneClassifier.h"
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
#include "coretech/vision/engine/objectDetector.h"
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
namespace Cozmo {
  
  CONSOLE_VAR_RANGED(u8,  kUseCLAHE_u8,     "Vision.PreProcessing", 0, 0, 4);  // One of MarkerDetectionCLAHE enum
  CONSOLE_VAR(s32, kClaheClipLimit,         "Vision.PreProcessing", 32);
  CONSOLE_VAR(s32, kClaheTileSize,          "Vision.PreProcessing", 4);
  CONSOLE_VAR(u8,  kClaheWhenDarkThreshold, "Vision.PreProcessing", 80); // In MarkerDetectionCLAHE::WhenDark mode, only use CLAHE when img avg < this
  CONSOLE_VAR(s32, kPostClaheSmooth,        "Vision.PreProcessing", -3); // 0: off, +ve: Gaussian sigma, -ve (& odd): Box filter size
  CONSOLE_VAR(s32, kMarkerDetector_ScaleMultiplier, "Vision.MarkerDetection", 1);
  
  CONSOLE_VAR(f32, kEdgeThreshold,  "Vision.OverheadEdges", 50.f);
  CONSOLE_VAR(u32, kMinChainLength, "Vision.OverheadEdges", 3); // in number of edge pixels
  
  CONSOLE_VAR(f32, kCalibDotSearchWidth_mm,   "Vision.ToolCode",  4.5f);
  CONSOLE_VAR(f32, kCalibDotSearchHeight_mm,  "Vision.ToolCode",  6.5f);
  CONSOLE_VAR(f32, kCalibDotMinContrastRatio, "Vision.ToolCode",  1.1f);
  
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
  
  // If non-zero, toggles the corresponding VisionMode and sets back to 0
  CONSOLE_VAR(u32, kToggleVisionMode, "Vision.General", 0);
  
  namespace {
    // These are initialized from Json config:
    u8 kTooDarkValue   = 15;
    u8 kTooBrightValue = 230;
    f32 kLowPercentile = 0.10f;
    f32 kMidPercentile = 0.50f;
    f32 kHighPercentile = 0.90f;
    bool kMeterFromDetections = true;
  }
  
  static const char * const kLogChannelName = "VisionSystem";
  
  VisionSystem::VisionSystem(const CozmoContext* context)
  : _rollingShutterCorrector()
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
  , _benchmark(new Vision::Benchmark())
  , _generalObjectDetector(new Vision::ObjectDetector())
  , _clahe(cv::createCLAHE())
  {
    DEV_ASSERT(_context != nullptr, "VisionSystem.Constructor.NullContext");
  } // VisionSystem()
  
  
  Result VisionSystem::Init(const Json::Value& config)
  {
    _isInitialized = false;
    
    _isReadingToolCode = false;
    
    std::string dataPath("");
    if(_context->GetDataPlatform() != nullptr) {
      dataPath = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                             Util::FileUtils::FullFilePath({"config", "engine", "vision"}));
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
      GET_JSON_PARAMETER(imageQualityConfig, "MidPercentile",       kMidPercentile);
      GET_JSON_PARAMETER(imageQualityConfig, "HighPercentile",      kHighPercentile);
      
      u8  targetMidValue=0;
      f32 maxChangeFraction = -1.f;
      s32 subSample = 0;
      
      GET_JSON_PARAMETER(imageQualityConfig, "MidValue",                 targetMidValue);
      GET_JSON_PARAMETER(imageQualityConfig, "MaxChangeFraction",        maxChangeFraction);
      GET_JSON_PARAMETER(imageQualityConfig, "SubSample",                subSample);
      
      Result expResult = SetAutoExposureParams(subSample, targetMidValue, kMidPercentile, maxChangeFraction);
      
      if(RESULT_OK != expResult)
      {
        PRINT_NAMED_ERROR("VisionSystem.Init.SetExposureParametersFailed", "");
        return expResult;
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
    
    {
      if(!config.isMember("ObjectDetector"))
      {
        PRINT_NAMED_ERROR("VisionSystem.Init.MissingObjectDetectorConfigField", "");
        return RESULT_FAIL;
      }
      
      
      const std::string modelPath = Util::FileUtils::FullFilePath({dataPath, "dnn_models"});
      if(Util::FileUtils::DirectoryExists(modelPath)) // TODO: Remove once DNN models are checked in somewhere (VIC-1071)
      {
        const Json::Value& objDetectorConfig = config["ObjectDetector"];
        Result objDetectorResult = _generalObjectDetector->Init(modelPath, objDetectorConfig);
        if(RESULT_OK != objDetectorResult)
        {
          PRINT_NAMED_ERROR("VisionSystem.Init.ObjectDetectorInitFailed", "");
        }
      }
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

    return result;
  } // Init()

  VisionSystem::~VisionSystem()
  {
    
  }
  
#if 0
#pragma mark --- Mode Controls ---
#endif

  Result VisionSystem::PushNextModeSchedule(AllVisionModesSchedule&& schedule)
  {
    _nextSchedules.push({true, std::move(schedule)});
    return RESULT_OK;
  }

  
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

  void VisionSystem::SetSaveParameters(const ImageSendMode saveMode, const std::string& path, const int8_t quality)
  {
    _imageSaveMode = saveMode;
    _imageSavePath = path;
    _imageSaveQuality = std::min(int8_t(100), quality);
  }

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
  
  Result VisionSystem::EnableToolCodeCalibration(bool enable)
  {
    if(IsModeEnabled(VisionMode::ReadingToolCode)) {
      PRINT_NAMED_WARNING("VisionSystem.EnableToolCodeCalibration.AlreadyReadingToolCode",
                          "Cannot enable/disable tool code calibration while in the middle "
                          "of reading tool code.");
      return RESULT_FAIL;
    }
    
    _calibrateFromToolCode = enable;
    return RESULT_OK;
  }
  
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
  
  
  Radians VisionSystem::GetCurrentHeadAngle()
  {
    return _poseData.histState.GetHeadAngle_rad();
  }
  
  
  Radians VisionSystem::GetPreviousHeadAngle()
  {
    return _prevPoseData.histState.GetHeadAngle_rad();
  }
  
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
  
  bool VisionSystem::IsInitialized() const
  {
    bool retVal = _isInitialized;
#   if ANKI_COZMO_USE_MATLAB_VISION
    retVal &= _matlab.ep != NULL;
#   endif
    return retVal;
  }
  
  u8 VisionSystem::ComputeMean(const Vision::Image& inputImageGray, const s32 sampleInc)
  {
    DEV_ASSERT(sampleInc >= 1, "VisionSystem.ComputeMean.BadIncrement");
    
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
  
  Result VisionSystem::CheckImageQuality(const Vision::Image& inputImage,
                                         const std::vector<Anki::Rectangle<s32>>& detections)
  {
#   define DEBUG_IMAGE_HISTOGRAM 0

    // Compute the exposure we would like to have
    f32 exposureAdjFrac = 1.f;
    
    Result expResult = RESULT_FAIL;
    if(!kMeterFromDetections || detections.empty())
    {
      expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, exposureAdjFrac);
    }
    else
    {
      // Give half the weight to the detections, the other half to the rest of the image
      std::vector<Anki::Rectangle<s32>> roiRects;
      s32 totalRoiArea = 0;
      for(auto const& quad : detections)
      {
        roiRects.emplace_back(quad);
        totalRoiArea += roiRects.back().Area();
      }
      
      DEV_ASSERT(totalRoiArea >= 0, "VisionSystem.CheckImageQuality.NegativeROIArea");
      
      if(2*totalRoiArea < inputImage.GetNumElements())
      {
        const u8 backgroundWeight = Util::numeric_cast<u8>(255.f * static_cast<f32>(totalRoiArea)/static_cast<f32>(inputImage.GetNumElements()));
        const u8 roiWeight = 255 - backgroundWeight;
        
        Vision::Image weightMask(inputImage.GetNumRows(), inputImage.GetNumCols());
        weightMask.FillWith(backgroundWeight);
        
        for(auto & rect : roiRects)
        {
          weightMask.GetROI(rect).FillWith(roiWeight);
        }
        
        expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, weightMask, exposureAdjFrac);
        
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
        expResult = _imagingPipeline->ComputeExposureAdjustment(inputImage, exposureAdjFrac);
      }
    }
    
    if(RESULT_OK != expResult)
    {
      PRINT_NAMED_WARNING("VisionSystem.CheckImageQuality.ComputeNewExposureFailed",
                          "Detection Quads=%zu", detections.size());
      return expResult;
    }
    
    if(DEBUG_IMAGE_HISTOGRAM)
    {
      const Vision::ImageBrightnessHistogram& hist = _imagingPipeline->GetHistogram();
      std::vector<u8> values = hist.ComputePercentiles({kLowPercentile, kMidPercentile, kHighPercentile});
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
    
    if(FLT_LT(exposureAdjFrac, 1.f))
    {
      // Want to bring brightness down: reduce exposure first, if possible
      if(_currentCameraParams.exposureTime_ms > _minCameraExposureTime_ms)
      {
        desiredExposureTime_ms = std::round(static_cast<f32>(_currentCameraParams.exposureTime_ms) * exposureAdjFrac);
        desiredExposureTime_ms = std::max(_minCameraExposureTime_ms, desiredExposureTime_ms);
      }
      else if(FLT_GT(_currentCameraParams.gain, _minCameraGain))
      {
        // Already at min exposure time; reduce gain
        desiredGain *= exposureAdjFrac;
        desiredGain = std::max(_minCameraGain, desiredGain);
      }
      else
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
    else if(FLT_GT(exposureAdjFrac, 1.f))
    {
      // Want to bring brightness up: increase gain first, if possible
      if(FLT_LT(_currentCameraParams.gain, _maxCameraGain))
      {
        desiredGain *= exposureAdjFrac;
        desiredGain = std::min(_maxCameraGain, desiredGain);
      }
      else if(_currentCameraParams.exposureTime_ms < _maxCameraExposureTime_ms)
      {
        // Already at max gain; increase exposure
        desiredExposureTime_ms = std::round(static_cast<f32>(_currentCameraParams.exposureTime_ms) * exposureAdjFrac);
        desiredExposureTime_ms = std::min(_maxCameraExposureTime_ms, desiredExposureTime_ms);
      }
      else
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
    
    // If we are in limited exposure mode limit the exposure to multiples of 10 ms
    // This is to prevent image artifacts from mismatched exposure and head light pulsing
    if(_mode.IsBitFlagSet(VisionMode::LimitedExposure))
    {
      static const int kExposureMultiple = 10;

      const int remainder = desiredExposureTime_ms % kExposureMultiple;
      // Round _maxCameraExposureTime_ms down to the nearest multiple of kExposureMultiple
      const int maxCameraExposureRounded_ms = _maxCameraExposureTime_ms - (_maxCameraExposureTime_ms % kExposureMultiple);
      if(remainder != 0)
      {
        desiredExposureTime_ms += (kExposureMultiple - remainder);
        desiredExposureTime_ms = std::min(maxCameraExposureRounded_ms, desiredExposureTime_ms);
      }
    }
    
    _currentResult.cameraParams.exposureTime_ms = desiredExposureTime_ms;
    _currentResult.cameraParams.gain = desiredGain;

    return RESULT_OK;
  }

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

  Result VisionSystem::EraseFace(Vision::FaceID_t faceID)
  {
    return _faceTracker->EraseFace(faceID);
  }
  
  void VisionSystem::SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
 																						  Vision::FaceID_t forFaceID,
																						  s32 numEnrollments)
  {
    _faceTracker->SetFaceEnrollmentMode(pose, forFaceID, numEnrollments);
  }
  
  void VisionSystem::EraseAllFaces()
  {
    _faceTracker->EraseAllFaces();
  }
  
  std::vector<Vision::LoadedKnownFace> VisionSystem::GetEnrolledNames() const
  {
    return _faceTracker->GetEnrolledNames();
  }
  
  Result VisionSystem::RenameFace(Vision::FaceID_t faceID, const std::string& oldName, const std::string& newName,
                                  Vision::RobotRenamedEnrolledFace& renamedFace)
  {
    return _faceTracker->RenameFace(faceID, oldName, newName, renamedFace);
  }
  
  
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
  
  
  Result VisionSystem::DetectFaces(const Vision::Image& grayImage,
                                   std::vector<Anki::Rectangle<s32>>& detectionRects)
  {
    DEV_ASSERT(_faceTracker != nullptr, "VisionSystem.DetectFaces.NullFaceTracker");
   
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
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result VisionSystem::DetectPets(const Vision::Image& grayImage,
                                  std::vector<Anki::Rectangle<s32>>& detections)
  {
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
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result VisionSystem::DetectMotion(Vision::ImageCache& imageCache)
  {

    Result result = RESULT_OK;
    
    _motionDetector->Detect(imageCache, _poseData, _prevPoseData,
                            _currentResult.observedMotions, _currentResult.debugImageRGBs);
    
    return result;
    
  } // DetectMotion()

  Result VisionSystem::UpdateOverheadMap(const Vision::ImageRGB& image)
  {
    Result result = _overheadMap->Update(image, _poseData, _currentResult.debugImageRGBs);
    return result;
  }

  Result VisionSystem::UpdateGroundPlaneClassifier(const Vision::ImageRGB& image)
  {
    Result result = _groundPlaneClassifier->Update(image, _poseData, _currentResult.debugImageRGBs,
                                                   _currentResult.visualObstacles);
    return result;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result VisionSystem::DetectLaserPoints(Vision::ImageCache& imageCache)
  {
    const bool isDarkExposure = (Util::IsNear(_currentCameraParams.exposureTime_ms, _minCameraExposureTime_ms) &&
                                 Util::IsNear(_currentCameraParams.gain, _minCameraGain));
    
    Result result = _laserPointDetector->Detect(imageCache, _poseData, isDarkExposure,
                                                _currentResult.laserPoints,
                                                _currentResult.debugImageRGBs);
    
    return result;
  }
  
#if 0
#pragma mark --- Public VisionSystem API Implementations ---
#endif
  
  std::string VisionSystem::GetCurrentModeName() const {
    return VisionSystem::GetModeName(_mode);
  }
  
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
  
  VisionMode VisionSystem::GetModeFromString(const std::string& str) const
  {
    return VisionModeFromString(str.c_str());
  }
  
#if defined(SEND_IMAGE_ONLY)
#  error SEND_IMAGE_ONLY doesn't really make sense for Basestation vision system.
#elif defined(RUN_GROUND_TRUTHING_CAPTURE)
#  error RUN_GROUND_TRUTHING_CAPTURE not implemented in Basestation vision system.
#endif

  Result VisionSystem::ClearToolCodeImages()
  {
    if(_isReadingToolCode) {
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.ClearToolCodeImages.AlreadyReadingToolCode",
                    "Cannot clear tool code images while already in the middle of reading tool codes.");
      return RESULT_FAIL;
    }
    
    _toolCodeImages.clear();
    return RESULT_OK;
  }

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
    
    auto markerIter = _currentResult.observedMarkers.begin();
    while(markerIter != _currentResult.observedMarkers.end())
    {
      auto & marker = *markerIter;
      
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
          const int warpIndex = std::floor(corner.y() / (imageCache.GetOrigNumRows() / _rollingShutterCorrector.GetNumDivisions()));
          DEV_ASSERT_MSG(warpIndex >= 0 && warpIndex < _rollingShutterCorrector.GetPixelShifts().size(),
                         "VisionSystem.DetectMarkersWithCLAHE.WarpIndexOOB", "Index:%d Corner y:%f",
                         warpIndex, corner.y());
          
          const Vec2f& pixelShift = _rollingShutterCorrector.GetPixelShifts().at(warpIndex);
          corner -= pixelShift;

          if(Util::IsFltLTZero(corner.x()) ||
             Util::IsFltLTZero(corner.y()) ||
             Util::IsFltGE(corner.x(), (f32)imageCache.GetOrigNumCols()) ||
             Util::IsFltGE(corner.y(), (f32)imageCache.GetOrigNumRows()))
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
  
  void VisionSystem::CheckForGeneralObjectDetections()
  {
    std::list<Vision::ObjectDetector::DetectedObject> objects;
    const bool resultReady = _generalObjectDetector->GetObjects(objects);
    if(resultReady)
    {
      VisionProcessingResult detectionResult;
      detectionResult.timestamp = _generalObjectDetectionTimestamp;
      detectionResult.modesProcessed.SetBitFlag(VisionMode::DetectingGeneralObjects, true);
      
      // Convert returned Vision::DetectedObject list to our (Cozmo) ExternalInterface message
      for(const auto& object : objects)
      {
        detectionResult.generalObjects.emplace_back(CladRect(object.rect.GetX(), object.rect.GetY(),
                                                             object.rect.GetWidth(), object.rect.GetHeight()),
                                                    object.name, object.timestamp, object.score);
      }
      
      _mutex.lock();
      _results.emplace(std::move(detectionResult));
      _mutex.unlock();
    }
  }
  
  Result VisionSystem::Update(const VisionPoseData&   poseData,
                              const Vision::ImageRGB& image)
  {
    _imageCache->Reset(image);
    
    return Update(poseData, *_imageCache);
  }
  
  
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
    
    _frameNumber++;
    
    // Store the new robot state and keep a copy of the previous one
    UpdatePoseData(poseData);
    
    const Vision::Image& inputImageGray = imageCache.GetGray();
    
    // Set up the results for this frame:
    VisionProcessingResult result;
    result.timestamp = inputImageGray.GetTimestamp();
    result.imageQuality = ImageQuality::Unchecked;
    result.cameraParams.exposureTime_ms = -1;
    std::swap(result, _currentResult);
    
    auto& visionModesProcessed = _currentResult.modesProcessed;
    visionModesProcessed.ClearFlags();
    
    if(kVisionSystemSimulatedDelay_ms > 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(kVisionSystemSimulatedDelay_ms));
    }
    
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
    
    bool& cameraParamsRequested = _nextCameraParams.first;
    if(cameraParamsRequested)
    {
      _currentCameraParams = _nextCameraParams.second;
      cameraParamsRequested = false;
    }

    // Apply CLAHE if enabled:
    DEV_ASSERT(kUseCLAHE_u8 < Util::EnumToUnderlying(MarkerDetectionCLAHE::Count),
               "VisionSystem.ApplyCLAHE.BadUseClaheVal");
    MarkerDetectionCLAHE useCLAHE = static_cast<MarkerDetectionCLAHE>(kUseCLAHE_u8);
    Vision::Image claheImage;
    
    // Note: this will do nothing and leave claheImage empty if CLAHE is disabled
    // entirely or for this frame.
    lastResult = ApplyCLAHE(imageCache, useCLAHE, claheImage);
    if(RESULT_OK != lastResult) {
      PRINT_NAMED_WARNING("VisionSystem.Update.FailedCLAHE", "");
      return lastResult;
    }

    // Rolling shutter correction
    if(_doRollingShutterCorrection)
    {
      Tic("RollingShutterComputePixelShifts");
      _rollingShutterCorrector.ComputePixelShifts(poseData, _prevPoseData, inputImageGray.GetNumRows());
      Toc("RollingShutterComputePixelShifts");
    }
    
    if(ShouldProcessVisionMode(VisionMode::ComputingStatistics))
    {
      Tic("TotalComputingStatistics");
      _currentResult.imageMean = ComputeMean(inputImageGray, kImageMeanSampleInc);
      visionModesProcessed.SetBitFlag(VisionMode::ComputingStatistics, true);
      Toc("TotalComputingStatistics");
    }
    
    std::vector<Anki::Rectangle<s32>> detectionRects;

    if(ShouldProcessVisionMode(VisionMode::DetectingMarkers)) {
      Tic("TotalDetectingMarkers");

      lastResult = DetectMarkersWithCLAHE(imageCache, claheImage, detectionRects, useCLAHE);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectMarkersFailed", "");
        return lastResult;
      }
      
      visionModesProcessed.SetBitFlag(VisionMode::DetectingMarkers, true);
      
      Toc("TotalDetectingMarkers");
    }
    
    if(ShouldProcessVisionMode(VisionMode::DetectingFaces)) {
      Tic("TotalDetectingFaces");
      if((lastResult = DetectFaces(inputImageGray, detectionRects)) != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectFacesFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::DetectingFaces, true);
      Toc("TotalDetectingFaces");
    }
    
    if(ShouldProcessVisionMode(VisionMode::DetectingPets)) {
      Tic("TotalDetectingPets");
      if((lastResult = DetectPets(inputImageGray, detectionRects)) != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectPetsFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::DetectingPets, true);
      Toc("TotalDetectingPets");
    }
    
    if(ShouldProcessVisionMode(VisionMode::DetectingMotion))
    {
      Tic("TotalDetectingMotion");
      if((lastResult = DetectMotion(imageCache)) != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectMotionFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::DetectingMotion, true);
      Toc("TotalDetectingMotion");
    }

    if (ShouldProcessVisionMode(VisionMode::BuildingOverheadMap))
    {
      if (imageCache.HasColor()) {
        Tic("UpdateOverheadMap");
        lastResult = UpdateOverheadMap(imageCache.GetRGB());
        Toc("UpdateOverheadMap");
        if (lastResult != RESULT_OK) {
          return lastResult;
        }
        visionModesProcessed.SetBitFlag(VisionMode::BuildingOverheadMap, true);
      }
      else {
        PRINT_NAMED_WARNING("VisionSystem.Update.NoColorImage", "No color image!");
      }
    }
    
    if (ShouldProcessVisionMode(VisionMode::DetectingVisualObstacles))
    {
      Tic("DetectVisualObstacles");
      lastResult = UpdateGroundPlaneClassifier(imageCache.GetRGB());
      Toc("DetectVisualObstacles");
      if (lastResult != RESULT_OK) {
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::DetectingVisualObstacles, true);
    }

    if(ShouldProcessVisionMode(VisionMode::DetectingOverheadEdges))
    {
      Tic("TotalDetectingOverheadEdges");

      lastResult = _overheadEdgeDetector->Detect(imageCache, _poseData, _currentResult);
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.DetectOverheadEdgesFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::DetectingOverheadEdges, true);
      Toc("TotalDetectingOverheadEdges");
    }
    
    if(ShouldProcessVisionMode(VisionMode::ReadingToolCode))
    {
      if((lastResult = ReadToolCode(inputImageGray)) != RESULT_OK) {
        PRINT_NAMED_ERROR("VisionSystem.Update.ReadToolCodeFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::ReadingToolCode, true);
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
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::ComputingCalibration, true);
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
          return lastResult;
        }
        visionModesProcessed.SetBitFlag(VisionMode::DetectingLaserPoints, true);
        Toc("TotalDetectingLaserPoints");
      }
    }

    // Check for any objects from the detector. It runs asynchronously, so these objects
    // will be from a different image than the one in the cache and will use their own
    // VisionProcessingResult.
    CheckForGeneralObjectDetections();
    
    if(ShouldProcessVisionMode(VisionMode::DetectingGeneralObjects))
    {
      const bool started = _generalObjectDetector->StartProcessingIfIdle(imageCache);
      if(started)
      {
        // Remember the timestamp of the image used to do object detection
        _generalObjectDetectionTimestamp = imageCache.GetTimeStamp();
      }
    }
    
    // NOTE: This should come after any detectors that add things to "detectionRects"
    //       since it meters exposure based on those.
    if(ShouldProcessVisionMode(VisionMode::CheckingQuality))
    {
      Tic("CheckingImageQuality");
      lastResult = CheckImageQuality(inputImageGray, detectionRects);
      Toc("CheckingImageQuality");
      
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("VisionSystem.Update.CheckImageQualityFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::CheckingQuality, true);
    }

    if(ShouldProcessVisionMode(VisionMode::CheckingWhiteBalance))
    {
      Tic("CheckingWhiteBalance");
      lastResult = _imagingPipeline->ComputeWhiteBalanceAdjustment(imageCache.GetRGB(),
                                                                   _currentResult.cameraParams.whiteBalanceGainR,
                                                                   _currentResult.cameraParams.whiteBalanceGainG,
                                                                   _currentResult.cameraParams.whiteBalanceGainB);
      Toc("CheckingWhiteBalance");
      
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("VisionSystem.Update.CheckWhiteBalanceFailed", "");
        return lastResult;
      }
      visionModesProcessed.SetBitFlag(VisionMode::CheckingWhiteBalance, true);
    }
    
    if(ShouldProcessVisionMode(VisionMode::Benchmarking))
    {
      Tic("Benchmarking");
      const Result benchMarkResult = _benchmark->Update(imageCache);
      Toc("BenchMarking");
      
      if(RESULT_OK != benchMarkResult) {
        PRINT_NAMED_ERROR("VisionSystem.Update.BenchmarkFailed", "");
        // Continue processing, since this should be independent of other modes
      }
      else {
        visionModesProcessed.SetBitFlag(VisionMode::Benchmarking, true);
      }
    }
    
    if(ShouldProcessVisionMode(VisionMode::SavingImages) && (ImageSendMode::Off != _imageSaveMode))
    {
      Tic("SavingImages");
      const std::string fullFilename = GetFileNameBasedOnFrameNumber((_imageSaveQuality < 0 ? "png" : "jpg"));

      PRINT_CH_DEBUG(kLogChannelName, "VisionSystem.Update.SavingImage", "Saving to %s",
                     fullFilename.c_str());

      const Result saveResult = imageCache.GetRGB().Save(fullFilename, _imageSaveQuality);

      Result saveSensorResult = RESULT_OK;
      if ((ImageSendMode::SingleShotWithSensorData == _imageSaveMode)) {
        saveSensorResult = SaveSensorData();
      }

      if((ImageSendMode::SingleShot == _imageSaveMode) || (ImageSendMode::SingleShotWithSensorData == _imageSaveMode))
      {
        _imageSaveMode = ImageSendMode::Off;
      }

      Toc("SavingImages");

      if((RESULT_OK != saveResult) || (RESULT_OK != saveSensorResult))
      {
        PRINT_NAMED_ERROR("VisionSystem.Update.SavingImagesFailed", "");
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
    
    return lastResult;
  } // Update()

  Result VisionSystem::SaveSensorData() const {

    const std::string fullFilename = GetFileNameBasedOnFrameNumber("json");

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

      // was picked up
      config["wasPickedUp"] = state.WasPickedUp();

      // head angle
      config["headAngle"] = state.GetHeadAngle_rad();
      // lift angle
      config["liftAngle"] = state.GetLiftAngle_rad();
    }

    Json::StyledWriter writer;
    outFile << writer.write(config);
    outFile.close();

    return RESULT_OK;
  }

std::string VisionSystem::GetFileNameBasedOnFrameNumber(const char *extension) const
{
  // Zero-padded frame number as filename:
  char filename[32];
  snprintf(filename, sizeof(filename)-1, "%08d.%s", _frameNumber, extension);

  const std::string fullFilename = Util::FileUtils::FullFilePath({_imageSavePath, filename});
  PRINT_CH_DEBUG(kLogChannelName, "VisionSystem.SaveSensorData.GetFileNameBasedOnFrameNumber", "Saving to %s",
                 fullFilename.c_str());
  return fullFilename;
}

bool VisionSystem::ShouldProcessVisionMode(VisionMode mode)
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
  const bool isTimeToProcess = _modeScheduleStack.front().CheckTimeToProcessAndAdvance(mode);
  return isTimeToProcess;
}

CameraParams VisionSystem::GetCurrentCameraParams() const
{
  // Return nextParams if they have not been set yet otherwise use currentParams
  return (_nextCameraParams.first ? _nextCameraParams.second : _currentCameraParams);
}

Result VisionSystem::SetAutoExposureParams(const s32 subSample,
                                           const u8  midValue,
                                           const f32 midPercentile,
                                           const f32 maxChangeFraction)
{
  Result result = _imagingPipeline->SetExposureParameters(midValue, midPercentile,
                                                          maxChangeFraction, subSample);

  if(RESULT_OK == result)
  {
    PRINT_CH_INFO(kLogChannelName, "VisionSystem.SetAutoExposureParams",
                  "subSample:%d midVal:%d midPerc:%.3f changeFrac:%.3f",
                  subSample, midValue, midPercentile, maxChangeFraction);
  }

  return result;
}

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

Result VisionSystem::ReadToolCode(const Vision::Image& image)
{
  //    // DEBUG!
  //    Vision::Image image;
  //    //image.Load("/Users/andrew/Dropbox (Anki, Inc)/ToolCode/cozmo1_151585ms_0.jpg");
  //    //image.Load("/Users/andrew/Dropbox (Anki, Inc)/ToolCode/cozmo1_251585ms_1.jpg");
  //    image.Load("/Users/andrew/Dropbox (Anki, Inc)/ToolCode/cozmo1_366670ms_0.jpg");
  //    if(image.IsEmpty()) {
  //      PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.ReadImageFileFail", "");
  //      return RESULT_FAIL;
  //    }
  //    _clahe->apply(image.get_CvMat_(), image.get_CvMat_());

  ToolCodeInfo readToolCodeMessage;
  readToolCodeMessage.code = ToolCode::UnknownTool;
  _isReadingToolCode = true;

  auto cleanupLambda = [this,&readToolCodeMessage]() {
    this->_currentResult.toolCodes.push_back(readToolCodeMessage);
    this->EnableMode(VisionMode::ReadingToolCode, false);
    this->_firstReadToolCodeTime_ms = 0;
    this->_isReadingToolCode = false;
    PRINT_CH_INFO(kLogChannelName, "VisionSystem.ReadToolCode.DisabledReadingToolCode", "");
  };

  if(_firstReadToolCodeTime_ms == 0) {
    _firstReadToolCodeTime_ms = image.GetTimestamp();
  }
  else if(image.GetTimestamp() - _firstReadToolCodeTime_ms > kToolCodeMotionTimeout_ms) {
    PRINT_NAMED_WARNING("VisionSystem.ReadToolCode.TimeoutWaitingForHeadOrLift",
                        "start: %d, current: %d, timeout=%dms",
                        _firstReadToolCodeTime_ms, image.GetTimestamp(), kToolCodeMotionTimeout_ms);
    cleanupLambda();
    return RESULT_FAIL;
  }

  // All the conditions that must be met to bother trying to read the tool code:
  const bool headMoving = (_poseData.histState.WasHeadMoving() || _prevPoseData.histState.WasHeadMoving());

  const bool liftMoving = (_poseData.histState.WasLiftMoving() || _prevPoseData.histState.WasLiftMoving());

  const bool headDown = _poseData.histState.GetHeadAngle_rad() <= MIN_HEAD_ANGLE + HEAD_ANGLE_TOL;

  const bool liftDown = _poseData.histState.GetLiftHeight_mm() <= LIFT_HEIGHT_LOWDOCK + READ_TOOL_CODE_LIFT_HEIGHT_TOL_MM;

  // Sanity checks: we should not even be calling ReadToolCode if everybody
  // hasn't done their job and got us into position
  if(headMoving || liftMoving || !headDown || !liftDown)
  {
    PRINT_CH_INFO(kLogChannelName, "VisionSystem.ReadToolCode.NotInPosition",
                  "Waiting for head / lift (headMoving %d, lifMoving %d, headDown %d, liftDown %d",
                  headMoving, liftMoving, headDown, liftDown);
    return RESULT_OK;
  }

  // Guarantee CheckingToolCode mode gets disabled and code read gets sent,
  // no matter how we return from this function
  Util::CleanupHelper disableCheckToolCode(cleanupLambda);

  // Center points of the calibration dots, in lift coordinate frame
  // TODO: Move these to be defined elsewhere
  const s32 LEFT_DOT = 0, RIGHT_DOT = 1;
  const std::vector<Point3f> toolCodeDotsWrtLift = {
      {1.5f,  10.f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT}, // Left in image
      {1.5f, -10.f, LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT}, // Right in image
  };

  const Pose3d liftBasePose(0.f, Y_AXIS_3D(), {LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]},
                            _poseData.histState.GetPose(), "RobotLiftBase");

  Pose3d liftPose(0.f, Y_AXIS_3D(), {LIFT_ARM_LENGTH, 0.f, 0.f}, liftBasePose, "RobotLift");

  Robot::ComputeLiftPose(_poseData.histState.GetLiftAngle_rad(), liftPose);

  Pose3d liftPoseWrtCam;
  if(false == liftPose.GetWithRespectTo(_poseData.cameraPose, liftPoseWrtCam)) {
    PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.PoseTreeError",
                      "Could not get lift pose w.r.t. camera pose.");
    return RESULT_FAIL;
  }

  // Put tool code dots in camera coordinate frame
  std::vector<Point3f> toolCodeDotsWrtCam;
  liftPoseWrtCam.ApplyTo(toolCodeDotsWrtLift, toolCodeDotsWrtCam);

  // Project into camera
  std::vector<Anki::Point2f> projectedToolCodeDots;
  _camera.Project3dPoints(toolCodeDotsWrtCam, projectedToolCodeDots);

  // Only proceed if all dots are visible with the current head/lift pose
  for(auto & point : projectedToolCodeDots) {
    if(!_camera.IsWithinFieldOfView(point)) {
      PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.DotsNotInFOV", "");
      return RESULT_FAIL;
    }
  }

  readToolCodeMessage.expectedCalibDotLeft_x  = projectedToolCodeDots[LEFT_DOT].x();
  readToolCodeMessage.expectedCalibDotLeft_y  = projectedToolCodeDots[LEFT_DOT].y();
  readToolCodeMessage.expectedCalibDotRight_x = projectedToolCodeDots[RIGHT_DOT].x();
  readToolCodeMessage.expectedCalibDotRight_y = projectedToolCodeDots[RIGHT_DOT].y();

#   if DRAW_TOOL_CODE_DEBUG
  Vision::ImageRGB dispImg(image);
#   endif

  // Tool code calibration dot parameters
  const f32 kDotWidth_mm = 2.5f;
  const f32 kDotHole_mm  = 2.5f/3.f;
  const s32 kBinarizeKernelSize = 11;
  const f32 kBinarizeKernelSigma = 7.f;
  const bool kIsCircularDot = true; // false for square dot with rounded corners
  const f32 holeArea = kDotHole_mm*kDotHole_mm * (kIsCircularDot ? 0.25f*M_PI : 1.f);
  const f32 filledDotArea = kDotWidth_mm*kDotWidth_mm * (kIsCircularDot ? 0.25f*M_PI : 1.f);
  const f32 kDotAreaFrac =  (filledDotArea - holeArea) /
                            (4.f*kCalibDotSearchWidth_mm * kCalibDotSearchHeight_mm);
  const f32 kMinDotAreaFrac   = 0.25f * kDotAreaFrac;
  const f32 kMaxDotAreaFrac   = 2.00f * kDotAreaFrac;
  const f32 kHoleAreaFrac     = holeArea / filledDotArea;
  const f32 kMaxHoleAreaFrac  = 4.f * kHoleAreaFrac;
  //const f32 kMinSolidity      = 0.5f*(filledDotArea - holeArea) * (kIsCircularDot ? 1.f/(kDotWidth_mm*kDotWidth_mm) : 1.f);

  Anki::Point2f camCen;
  std::vector<Anki::Point2f> observedPoints;
  _toolCodeImages.clear();
  for(size_t iDot=0; iDot<projectedToolCodeDots.size(); ++iDot)
  {
    // Get an ROI around where we expect to see the dot in the image
    const Point3f& dotWrtLift3d = toolCodeDotsWrtLift[iDot];
    Quad3f dotQuadRoi3d = {
        {dotWrtLift3d.x() - kCalibDotSearchHeight_mm, dotWrtLift3d.y() - kCalibDotSearchWidth_mm, dotWrtLift3d.z()},
        {dotWrtLift3d.x() - kCalibDotSearchHeight_mm, dotWrtLift3d.y() + kCalibDotSearchWidth_mm, dotWrtLift3d.z()},
        {dotWrtLift3d.x() + kCalibDotSearchHeight_mm, dotWrtLift3d.y() - kCalibDotSearchWidth_mm, dotWrtLift3d.z()},
        {dotWrtLift3d.x() + kCalibDotSearchHeight_mm, dotWrtLift3d.y() + kCalibDotSearchWidth_mm, dotWrtLift3d.z()},
    };

    Quad3f dotQuadRoi3dWrtCam;
    liftPoseWrtCam.ApplyTo(dotQuadRoi3d, dotQuadRoi3dWrtCam);

    if(DRAW_TOOL_CODE_DEBUG)
    {
      Quad3f dotQuadRoi3dWrtWorld;
      liftPose.GetWithRespectToRoot().ApplyTo(dotQuadRoi3d, dotQuadRoi3dWrtWorld);
      dotQuadRoi3dWrtWorld += Point3f(0,0,0.5f);
      _vizManager->DrawQuad(VizQuadType::VIZ_QUAD_GENERIC_3D, 9324+(u32)iDot, dotQuadRoi3dWrtWorld, NamedColors::RED);

      Quad3f dotQuad3d = {
          {dotWrtLift3d.x() - kDotWidth_mm*0.5f, dotWrtLift3d.y() - kDotWidth_mm*0.5f, dotWrtLift3d.z()},
          {dotWrtLift3d.x() - kDotWidth_mm*0.5f, dotWrtLift3d.y() + kDotWidth_mm*0.5f, dotWrtLift3d.z()},
          {dotWrtLift3d.x() + kDotWidth_mm*0.5f, dotWrtLift3d.y() - kDotWidth_mm*0.5f, dotWrtLift3d.z()},
          {dotWrtLift3d.x() + kDotWidth_mm*0.5f, dotWrtLift3d.y() + kDotWidth_mm*0.5f, dotWrtLift3d.z()},
      };
      Quad3f dotQuadWrtWorld;
      liftPose.GetWithRespectToRoot().ApplyTo(dotQuad3d, dotQuadWrtWorld);
      dotQuadWrtWorld += Point3f(0,0,0.5f);
      _vizManager->DrawQuad(VizQuadType::VIZ_QUAD_GENERIC_3D, 9337+(u32)iDot, dotQuadWrtWorld, NamedColors::GREEN);
    }

    Quad2f dotQuadRoi2d;
    _camera.Project3dPoints(dotQuadRoi3dWrtCam, dotQuadRoi2d);

    Anki::Rectangle<s32> dotRectRoi(dotQuadRoi2d);

    // Save ROI image for writing to robot's NVStorage
    _toolCodeImages.emplace_back(image.GetROI(dotRectRoi));
    const Vision::Image& dotRoi = _toolCodeImages.back();

    // Simple global threshold for binarization
    //Vision::Image invertedDotRoi = dotRoi.GetNegative();
    //double maxVal = 0, minVal = 0;
    //cv::minMaxIdx(invertedDotRoi.get_CvMat_(), &minVal, &maxVal);
    //invertedDotRoi.Threshold((maxVal + minVal)*0.5);

    // Perform local binarization:
    Vision::Image dotRoi_blurred;
    cv::GaussianBlur(dotRoi.get_CvMat_(), dotRoi_blurred.get_CvMat_(),
                     cv::Size(kBinarizeKernelSize,kBinarizeKernelSize), kBinarizeKernelSigma);
    Vision::Image binarizedDotRoi(dotRoi.GetNumRows(), dotRoi.GetNumCols());
    const u8 roiMean = cv::saturate_cast<u8>(1.5 * cv::mean(dotRoi.get_CvMat_())[0]); // 1.5 = fudge factor
    binarizedDotRoi.get_CvMat_() = ((dotRoi.get_CvMat_() < dotRoi_blurred.get_CvMat_()) &
                                    (dotRoi.get_CvMat_() < roiMean));

    if(false && DRAW_TOOL_CODE_DEBUG) {
      _currentResult.debugImages.push_back({(iDot==0 ? "dotRoi0" : "dotRoi1"), dotRoi});
      _currentResult.debugImages.push_back({(iDot==0 ? "dotRoi0_blurred" : "dotRoi1_blurred"), dotRoi_blurred});
      _currentResult.debugImages.push_back({(iDot==0 ? "InvertedDotROI0" : "InvertedDotRoi1"), binarizedDotRoi});
    }

    // Get connected components in the ROI
    Array2d<s32> labels;
    cv::Mat stats, centroids;
    const s32 numComponents = cv::connectedComponentsWithStats(binarizedDotRoi.get_CvMat_(), labels.get_CvMat_(), stats, centroids);

    s32 dotLabel = -1;

    // Filter out components based on a variety of checks:
    //  area, solidity, existence and size of a hole that is fully surrounded
    Anki::Point2f roiCen(binarizedDotRoi.GetNumCols(), binarizedDotRoi.GetNumRows());
    roiCen *= 0.5f;
    f32 distToCenterSq = FLT_MAX;
    for(s32 iComp=1; iComp < numComponents; ++iComp)
    {
      const s32* compStats = stats.ptr<s32>(iComp);
      const s32 compArea = compStats[cv::CC_STAT_AREA];
      //const s32 bboxArea = compStats[cv::CC_STAT_HEIGHT]*compStats[cv::CC_STAT_WIDTH];
      //const f32 solidity = (f32)compArea/(f32)bboxArea;

      //        // DEBUG!!!
      //        {
      //          Vision::Image temp;
      //          temp.get_CvMat_() = (labels.get_CvMat_() == iComp);
      //          PRINT_NAMED_DEBUG("Component", "iComp: %d, Area: %d, solidity: %.3f",
      //                            iComp, compArea, -1.f);
      //          temp.Display("Component", 0);
      //        }

      if(compArea > kMinDotAreaFrac*binarizedDotRoi.GetNumElements() &&
         compArea < kMaxDotAreaFrac*binarizedDotRoi.GetNumElements()
        /* && solidity > kMinSolidity */)
      {
        const f64* dotCentroid = centroids.ptr<f64>(iComp);
        const f32 distSq = (Anki::Point2f(dotCentroid[0], dotCentroid[1]) - roiCen).LengthSq();
        if(distSq < distToCenterSq)
        {
          // Check to see if center point is "empty" (has background label)
          // Note the x/y vs. row/col switch here
          const s32 centerLabel = labels(std::round(dotCentroid[1]),
                                         std::round(dotCentroid[0]));

          if(centerLabel == 0)
          {
            Anki::Rectangle<s32> compRect(compStats[cv::CC_STAT_LEFT],  compStats[cv::CC_STAT_TOP],
                                          compStats[cv::CC_STAT_WIDTH], compStats[cv::CC_STAT_HEIGHT]);

            Vision::Image compBrightnessROI = dotRoi.GetROI(compRect);
            Array2d<s32> labelROI;
            labels.GetROI(compRect).CopyTo(labelROI); // need copy!

            // Verify if we flood fill from center that we get a hole of
            // reasonable size that doesn't "leak" outside of this component
            cv::floodFill(labelROI.get_CvMat_(), cv::Point(dotCentroid[0]-compRect.GetX(), dotCentroid[1]-compRect.GetY()),
                          numComponents+1);

            //              // DEBUG!!!
            //              //if(iDot == 1){
            //              {
            //                Vision::Image temp;
            //                temp.get_CvMat_() = (labels.get_CvMat_() == iComp);
            //                PRINT_NAMED_DEBUG("Component", "iComp: %d, Area: %d, solidity: %.3f",
            //                                  iComp, compArea, -1.f);
            //                temp.Display("Component");
            //
            //                Vision::Image tempFill;
            //                tempFill.get_CvMat_() = (labelROI.get_CvMat_() == numComponents+1);
            //                tempFill.Display("FloodFill", 0);
            //              }

            // Loop over an even smaller ROI right around the component to
            // compute the hole size, the brightness of that hole vs.
            // the component itself, and whether the hole is completely
            // surrounded by the component or touches the edge of ROI.
            s32 avgDotBrightness = 0;
            s32 avgHoleBrightness = 0;
            s32 holeArea = 0;
            bool touchesEdge = false;
            for(s32 i=0; i<labelROI.GetNumRows() && !touchesEdge; ++i)
            {
              const u8* brightness_i = compBrightnessROI.GetRow(i);
              const s32* label_i = labelROI.GetRow(i);

              for(s32 j=0; j<labelROI.GetNumCols() && !touchesEdge; ++j)
              {
                if(label_i[j] == numComponents+1)
                {
                  ++holeArea;
                  avgHoleBrightness += brightness_i[j];

                  if(i==0 || i==labelROI.GetNumRows()-1 ||
                     j==0 || j==labelROI.GetNumCols()-1)
                  {
                    touchesEdge = true;
                  }
                }
                else if(label_i[j] == iComp)  {
                  avgDotBrightness += brightness_i[j];
                }
              }
            }

            if(!touchesEdge)
            {
              avgHoleBrightness /= holeArea;
              avgDotBrightness  /= compArea;

              // Hole should neither leak to the outside, nor should it be too big,
              // and its brightness should be sufficiently brighter than the dot
              const bool holeSmallEnough = holeArea < compArea * kMaxHoleAreaFrac;
              const bool enoughContrast = (f32)avgHoleBrightness > kCalibDotMinContrastRatio * (f32)avgDotBrightness;
              if(holeSmallEnough && enoughContrast)
              {
                // Yay, passed all checks! Thus "must" be a tool code.
                dotLabel = iComp;
                distToCenterSq = distSq;
              } else if(!enoughContrast) {
                PRINT_CH_INFO(kLogChannelName, "VisionSystem.ReadToolCode.BadContrast",
                              "Dot %lu: Contrast for comp %d = %f",
                              (unsigned long)iDot, iComp, (f32)avgHoleBrightness / (f32)avgDotBrightness);
              } else if(!holeSmallEnough) {
                PRINT_CH_INFO(kLogChannelName, "VisionSystem.ReadToolCode.HoleTooLarge",
                              "Dot %lu: hole too large %d > %f*%d (=%f)",
                              (unsigned long)iDot, holeArea, kMaxHoleAreaFrac, compArea,
                              kMaxHoleAreaFrac*compArea);
              }
            }
          }
        } // dist to center check
      } // area check
    } // for each component iComp

    if(DRAW_TOOL_CODE_DEBUG) {
      Vision::ImageRGB roiImgDisp(binarizedDotRoi);
      // Function to color component with dotLabel green, and white for all others
      std::function<Vision::PixelRGB(const s32&)> fcn = [dotLabel](const s32& label)
      {
        if(label == dotLabel) {
          return Vision::PixelRGB(0,255,0);
        } else if(label == 0) {
          return Vision::PixelRGB(0,0,0);
        } else {
          return Vision::PixelRGB(255,255,255);
        }
      };
      labels.ApplyScalarFunction(fcn, roiImgDisp);
      if(dotLabel != -1) {
        const f64* dotCentroid = centroids.ptr<f64>(dotLabel);
        roiImgDisp.DrawCircle(Anki::Point2f(dotCentroid[0], dotCentroid[1]), NamedColors::RED, 1);

        const s32* compStats = stats.ptr<s32>(dotLabel);
        Anki::Rectangle<f32> compRect(compStats[cv::CC_STAT_LEFT],  compStats[cv::CC_STAT_TOP],
                                      compStats[cv::CC_STAT_WIDTH], compStats[cv::CC_STAT_HEIGHT]);
        roiImgDisp.DrawRect(compRect, NamedColors::RED, 1);
      }
      _currentResult.debugImageRGBs.push_back({(iDot==0 ? "DotROI0withCentroid" : "DotROI1withCentroid"), roiImgDisp});
    } // if(DRAW_TOOL_CODE_DEBUG)

    if(dotLabel == -1) {
      PRINT_NAMED_WARNING("VisionSystem.ReadToolCode.DotsNotFound",
                          "Failed to find valid dot");

      // Continuing to the next dot so that we at least have images
      continue;
    }

    DEV_ASSERT(centroids.type() == CV_64F, "VisionSystem.ReadToolCode.CentroidTypeNotDouble");
    const f64* dotCentroid = centroids.ptr<f64>(dotLabel);
    observedPoints.push_back(Anki::Point2f(dotCentroid[0] + dotRectRoi.GetX(),
                                           dotCentroid[1] + dotRectRoi.GetY()));

#     if DRAW_TOOL_CODE_DEBUG
    dispImg.DrawPoint(observedPoints.back(), NamedColors::ORANGE, 1);
      dispImg.DrawPoint(projectedToolCodeDots[iDot], NamedColors::BLUE,   2);
      dispImg.DrawQuad(dotQuadRoi2d, NamedColors::CYAN, 1);
#     endif
  } // for each tool code dot iDot

  if (observedPoints.size() < 2) {
    PRINT_NAMED_WARNING("VisionSystem.ReadToolCode.WrongNumDotsObserved",
                        "Dots found in %zu images", observedPoints.size());

    // TODO: Return failure instead?
    return RESULT_OK;
  }

  readToolCodeMessage.observedCalibDotLeft_x  = observedPoints[LEFT_DOT].x();
  readToolCodeMessage.observedCalibDotLeft_y  = observedPoints[LEFT_DOT].y();
  readToolCodeMessage.observedCalibDotRight_x = observedPoints[RIGHT_DOT].x();
  readToolCodeMessage.observedCalibDotRight_y = observedPoints[RIGHT_DOT].y();

  // TODO: Actually read the code and put corresponding result in the mailbox (once we have more than one)
  // NOTE: This gets put in the mailbox by the Cleanup object defined at the top of the function
  readToolCodeMessage.code = ToolCode::CubeLiftingTool;

  if(_calibrateFromToolCode)
  {
    // Solve for camera center and focal length as a system of equations
    //
    // Let:
    //   (x_i, y_i, z_i) = 3D location of tool code dot i
    //   (u_i, v_i)      = observed 2D projection tool code dot i
    //   (cx,cy)         = calibration center point
    //   f               = calibration focal length
    //
    // Then:
    //
    //   [z_i  0   x_i] [cx]   [z_i * u_i]
    //   [0   z_i  y_i] [cy] = [z_i * v_i]
    //                  [f ]

    SmallMatrix<4, 3, f32> A;
    Anki::Point<4, f32> b;
    Anki::Point<3, f32> calibParams;

    for(s32 iDot=0; iDot<2; ++iDot)
    {
      A(iDot*2,0)   = toolCodeDotsWrtCam[iDot].z();
      A(iDot*2,1)   = 0.f;
      A(iDot*2,2)   = toolCodeDotsWrtCam[iDot].x();
      b[iDot*2]     = toolCodeDotsWrtCam[iDot].z() * observedPoints[iDot].x();

      A(iDot*2+1,0) = 0.f;
      A(iDot*2+1,1) = toolCodeDotsWrtCam[iDot].z();
      A(iDot*2+1,2) = toolCodeDotsWrtCam[iDot].y();
      b[iDot*2+1]   = toolCodeDotsWrtCam[iDot].z() * observedPoints[iDot].y();
    }

    Result lsqResult = LeastSquares(A,b,calibParams);

    DEV_ASSERT(lsqResult == RESULT_OK, "LeastSquares failed");

    camCen.x()  = calibParams[0];
    camCen.y()  = calibParams[1];
    const f32 f = calibParams[2];

#     if DRAW_TOOL_CODE_DEBUG
    {
        char dispStr[256];
        snprintf(dispStr, 255, "f=%.1f, cen=(%.1f,%.1f)",
                 f, camCen.x(), camCen.y());
        dispImg.DrawText(Anki::Point2f(0, 15), dispStr, NamedColors::RED, 0.6);
        _currentResult.debugImageRGBs.push_back({"ToolCode", dispImg});
      }
#     endif

    if(std::isnan(camCen.x()) || std::isnan(camCen.y())) {
      PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.CamCenNaN", "");
      return RESULT_FAIL;
    } else if(std::isnan(f) || f <= 0.f) {
      PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.BadFocalLength", "");
      return RESULT_FAIL;
    } else {
      // Make sure we're not changing too drastically
      const f32 kMaxChangeFraction = 0.25f;
      const f32 fChangeFrac = f/_camera.GetCalibration()->GetFocalLength_x();
      const f32 xChangeFrac = camCen.x() / _camera.GetCalibration()->GetCenter_x();
      const f32 yChangeFrac = camCen.y() / _camera.GetCalibration()->GetCenter_y();
      if(!NEAR(fChangeFrac, 1.f, kMaxChangeFraction) ||
         !NEAR(xChangeFrac, 1.f, kMaxChangeFraction) ||
         !NEAR(yChangeFrac, 1.f, kMaxChangeFraction))
      {
        PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.ChangeTooLarge",
                          "Calibration change too large from current: f=%f vs %f, "
                              "cen=(%f,%f) vs (%f,%f)",
                          f, _camera.GetCalibration()->GetFocalLength_x(),
                          xChangeFrac, yChangeFrac,
                          _camera.GetCalibration()->GetCenter_x(),
                          _camera.GetCalibration()->GetCenter_y());
        return RESULT_FAIL;
      }


      // Sanity check the new calibration:
      if(true) // TODO: Only in debug?
      {
        Vision::Camera tempCamera;
        auto tempCalib = std::make_shared<Vision::CameraCalibration>(_camera.GetCalibration()->GetNrows(),
                                                                     _camera.GetCalibration()->GetNcols(),
                                                                     _camera.GetCalibration()->GetFocalLength_x(),
                                                                     _camera.GetCalibration()->GetFocalLength_y(),
                                                                     _camera.GetCalibration()->GetCenter_x(),
                                                                     _camera.GetCalibration()->GetCenter_y());
        tempCalib->SetFocalLength(f,f);
        tempCalib->SetCenter(camCen);
        tempCamera.SetCalibration(tempCalib);
        std::vector<Anki::Point2f> sanityCheckPoints;
        tempCamera.Project3dPoints(toolCodeDotsWrtCam, sanityCheckPoints);
        for(s32 i=0; i<2; ++i)
        {
          const f32 reprojErrorSq = (sanityCheckPoints[i] - observedPoints[i]).LengthSq();
          if(reprojErrorSq > 5*5)
          {
            if(DRAW_TOOL_CODE_DEBUG)
            {
              Vision::ImageRGB dispImg(image);
              dispImg.DrawCircle(sanityCheckPoints[0], NamedColors::RED, 1);
              dispImg.DrawCircle(sanityCheckPoints[1], NamedColors::RED, 1);
              dispImg.DrawCircle(observedPoints[0], NamedColors::GREEN, 1);
              dispImg.DrawCircle(observedPoints[1], NamedColors::GREEN, 1);
              _currentResult.debugImageRGBs.push_back({"SanityCheck", dispImg});
            }
            PRINT_NAMED_ERROR("VisionSystem.ReadToolCode.BadProjection",
                              "Reprojection error of point %d = %f",
                              i, std::sqrtf(reprojErrorSq));
            return RESULT_FAIL;
          }
        }
      } // if sanity checking the new calibration

      // Update the camera calibration
      PRINT_CH_INFO(kLogChannelName, "VisionSystem.ReadToolCode.CameraCalibUpdated",
                    "OldCen=(%f,%f), NewCen=(%f,%f), OldF=(%f,%f), NewF=(%f,%f), t=%dms",
                    _camera.GetCalibration()->GetCenter_x(),
                    _camera.GetCalibration()->GetCenter_y(),
                    camCen.x(), camCen.y(),
                    _camera.GetCalibration()->GetFocalLength_x(),
                    _camera.GetCalibration()->GetFocalLength_y(),
                    f, f,
                    image.GetTimestamp());
      _camera.GetCalibration()->SetCenter(camCen);
      _camera.GetCalibration()->SetFocalLength(f, f);

    } // if(new calib values pass sanity / nan checks)
  } // if tool code calibration is enabled

  return RESULT_OK;
} // ReadToolCode()

Result VisionSystem::GetSerializedFaceData(std::vector<u8>& albumData,
                                           std::vector<u8>& enrollData) const
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.GetSerializedFaceData.NullFaceTracker");
  return _faceTracker->GetSerializedData(albumData, enrollData);
}

Result VisionSystem::SetSerializedFaceData(const std::vector<u8>& albumData,
                                           const std::vector<u8>& enrollData,
                                           std::list<Vision::LoadedKnownFace>& loadedFaces)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SetSerializedFaceData.NullFaceTracker");
  return _faceTracker->SetSerializedData(albumData, enrollData, loadedFaces);
}

Result VisionSystem::LoadFaceAlbum(const std::string& albumName, std::list<Vision::LoadedKnownFace> &loadedFaces)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.LoadFaceAlbum.NullFaceTracker");
  return _faceTracker->LoadAlbum(albumName, loadedFaces);
}

Result VisionSystem::SaveFaceAlbum(const std::string& albumName)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SaveFaceAlbum.NullFaceTracker");
  return _faceTracker->SaveAlbum(albumName);
}

void VisionSystem::SetFaceRecognitionIsSynchronous(bool isSynchronous)
{
  DEV_ASSERT(nullptr != _faceTracker, "VisionSystem.SetFaceRecognitionRunMode.NullFaceTracker");
  _faceTracker->SetRecognitionIsSynchronous(isSynchronous);
}

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

} // namespace Cozmo
} // namespace Anki
