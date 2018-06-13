/**
 * File: petTracker.cpp
 *
 * Author: Andrew Stein
 * Date:   05/09/2016
 *
 * Description: Wrapper for Omron Computer Vision (OMCV) pet detection library.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/rotation.h"

#include "coretech/vision/engine/petTracker.h"

#include "clad/types/petTypes.h"

#include "util/logging/logging.h"
#include "util/console/consoleInterface.h"

// Omron Computer Vision
#include "OmcvPdAPI.h"
#include "DetectionInfo.h"
#include "DetectorComDef.h"

#if !defined(ANDROID) && !defined(VICOS)
extern "C"
{
  // These two functions must exist to avoid linker errors but until we need to use the
  // timeout functionality of the OMCV Pet Detector library, they can just be dummy functions
  // See also page 30 of OMCVPetDetectionV2.0SoftwareSpecification_E.pdf
  UINT32 OkaoExtraInitTime(void)
  {
    return 0;
  }
  
  UINT32 OkaoExtraGetTime(UINT32 unInitTime)
  {
    return 0;
  }
}
#endif

// If < 0, use value from JSON config. Otherwise use this one (for dev adjustment live)
CONSOLE_VAR_RANGED(s32, kRuntimePetDetectionThreshold, "Vision.PetDetection", -1, -1, 1000);


namespace Anki {
namespace Vision {
  
namespace JsonKey {
  static const char * const PetTracker          = "PetTracker";
  static const char * const MaxPets             = "MaxPets";
  static const char * const MinFaceSize         = "MinFaceSize";
  static const char * const MaxFaceSize         = "MaxFaceSize";
  static const char * const DetectionThreshold  = "DetectionThreshold";
  static const char * const InitialSearchCycle  = "InitialSearchCycle";
  static const char * const NewSearchCycle      = "NewSearchCycle";
  static const char * const TrackLostCount      = "TrackLostCount";
  static const char * const TrackSteadiness     = "TrackSteadiness";
};
  
struct PetTracker::Handles
{
  // Okao Vision Library "Handles"
  HPET        omcvPetDetector         = NULL;
  HPDRESULT   omcvDetectionResult     = NULL;
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PetTracker::PetTracker()
: _handles(new Handles())
{
  Profiler::SetProfileGroupName("PetTracker");
} // PetTracker Constructor()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PetTracker::Init(const Json::Value& config)
{
  _isInitialized = false;
  
  // Get and print Okao library version as a sanity check that we can even
  // talk to the library
  UINT8 omcvVersionMajor=0, omcvVersionMinor = 0;
  INT32 omcvResult = OMCV_PD_GetVersion(&omcvVersionMajor, &omcvVersionMinor);
  if(omcvResult != OMCV_NORMAL) {
    PRINT_NAMED_ERROR("PetTracker.Init.OmcvVersionFail", "");
    return RESULT_FAIL;
  }
  PRINT_NAMED_INFO("PetTracker.OmcvVersion",
                   "Initializing with FaceLibVision version %d.%d",
                   omcvVersionMajor, omcvVersionMinor);
  
  //
  // Read parameters from Json
  //
  if(!config.isMember(JsonKey::PetTracker))
  {
    PRINT_NAMED_ERROR("PetTracker.Init.MissingConfigGroup", "Expecting '%s'", JsonKey::PetTracker);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  const Json::Value& petConfig = config[JsonKey::PetTracker];
  
  std::map<const char*, s32> parameters{
    {JsonKey::MaxPets,              -1},
    {JsonKey::MinFaceSize,          -1},
    {JsonKey::MaxFaceSize,          -1},
    {JsonKey::InitialSearchCycle,   -1},
    {JsonKey::NewSearchCycle,       -1},
    {JsonKey::DetectionThreshold,   -1},
    {JsonKey::TrackLostCount,       -1},
    {JsonKey::TrackSteadiness,      -1},
  };
  
  for(auto & param : parameters)
  {
    if(!JsonTools::GetValueOptional(petConfig, param.first, param.second))
    {
      PRINT_NAMED_ERROR("PetTracker.Init.MissingParameter", "%s.%s",
                        JsonKey::PetTracker, param.first);
      return RESULT_FAIL_INVALID_PARAMETER;
    }
  }
  
  //
  // Set OMCV parameters
  //
  
  _handles->omcvPetDetector = OMCV_PD_CreateHandle(DETECTION_MODE_MOVIE, parameters[JsonKey::MaxPets]);
  if(NULL == _handles->omcvPetDetector) {
    PRINT_NAMED_ERROR("PetTracker.Init.FaceLibCommonHandleAllocFail", "");
    return RESULT_FAIL_MEMORY;
  }
  
  _handles->omcvDetectionResult = OMCV_PD_CreateResultHandle();
  if(NULL == _handles->omcvDetectionResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.OmcvCreateResultHandleFail", "");
    return RESULT_FAIL_MEMORY;
  }
  
  const UINT32 searchAngles = ROLL_ANGLE_U45; // (ROLL_ANGLE_U45 | ROLL_ANGLE_2 | ROLL_ANGLE_10);
  omcvResult = OMCV_PD_SetAngle(_handles->omcvPetDetector, POSE_YAW_FRONT, searchAngles);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.OmcvSetAngleFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  omcvResult = OMCV_PD_MV_SetSearchCycle(_handles->omcvPetDetector,
                                         parameters[JsonKey::InitialSearchCycle],
                                         parameters[JsonKey::NewSearchCycle]);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.OmcvSetSearchCycleFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  omcvResult = OMCV_PD_SetSizeRange(_handles->omcvPetDetector,
                                    parameters[JsonKey::MinFaceSize],
                                    parameters[JsonKey::MaxFaceSize]);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.OmcvSetSizeRangeFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  omcvResult = OMCV_PD_SetThreshold(_handles->omcvPetDetector, parameters[JsonKey::DetectionThreshold]);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.FaceLibSetThresholdFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  omcvResult = OMCV_PD_MV_SetLostParam(_handles->omcvPetDetector,
                                       parameters[JsonKey::TrackLostCount],
                                       parameters[JsonKey::TrackLostCount]);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.FaceLibSetLostParamFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  // NOTE: using same steadiness parameter for position and size
  omcvResult = OMCV_PD_MV_SetSteadinessParam(_handles->omcvPetDetector,
                                             parameters[JsonKey::TrackSteadiness],
                                             parameters[JsonKey::TrackSteadiness]);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_ERROR("PetTracker.Init.FaceLibSetSteadinessFailed", "OMCV Result=%d", omcvResult);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  _isInitialized = true;
  return RESULT_OK;
      
} // Init()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PetTracker::~PetTracker()
{
  if(NULL != _handles->omcvDetectionResult) {
    if(OMCV_NORMAL != OMCV_PD_DeleteResultHandle(_handles->omcvDetectionResult)) {
      PRINT_NAMED_ERROR("PetTracker.Destructor.OmcvDeleteResultHandleFail", "");
    }
  }
  
  if(NULL != _handles->omcvPetDetector) {
    if(OMCV_NORMAL != OMCV_PD_DeleteHandle(_handles->omcvPetDetector)) {
      PRINT_NAMED_ERROR("PetTracker.Destructor.OmcvDeleteHandleFail", "");
    }
  }
  
  _isInitialized = false;
} // ~PetTracker()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PetTracker::Update(const Vision::Image&       frameOrig,
                          std::list<TrackedPet>&     pets)
{
  // Initialize on first use
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("PetTracker.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
  
  if(kRuntimePetDetectionThreshold >= 0 && _runtimeDetectionThreshold != kRuntimePetDetectionThreshold)
  {
    PRINT_NAMED_DEBUG("PetTracker.Update.ThresholdUpdated",
                      "Updating detection threshold from %d to %d",
                      _runtimeDetectionThreshold,
                      kRuntimePetDetectionThreshold);
    
    _runtimeDetectionThreshold = kRuntimePetDetectionThreshold;
    
    INT32 omcvResult = OMCV_PD_SetThreshold(_handles->omcvPetDetector, _runtimeDetectionThreshold);
    if(OMCV_NORMAL != omcvResult) {
      PRINT_NAMED_ERROR("PetTracker.Update.FaceLibSetThresholdFailed", "OMCV Result=%d", omcvResult);
      return RESULT_FAIL_INVALID_PARAMETER;
    }
  }
  
  DEV_ASSERT(frameOrig.IsContinuous(), "PetTracker.Update.NonContinuousImage");
  
  INT32 omcvResult = OMCV_NORMAL;
  Tic("PetDetect");
  const INT32 nWidth  = frameOrig.GetNumCols();
  const INT32 nHeight = frameOrig.GetNumRows();
  RAWIMAGE* dataPtr = const_cast<UINT8*>(frameOrig.GetDataPointer());
  omcvResult = OMCV_PD_Detect(_handles->omcvPetDetector, dataPtr, nWidth, nHeight, _handles->omcvDetectionResult);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_WARNING("PetTracker.Update.OmcvDetectFail", "OMCV Result Code=%d, dataPtr=%p, nWidth=%d, nHeight=%d",
                        omcvResult, dataPtr, nWidth, nHeight);
    return RESULT_FAIL;
  }
  
  INT32 numDetections = 0;
  omcvResult = OMCV_PD_GetResultCount(_handles->omcvDetectionResult, OBJ_TYPE_DOG|OBJ_TYPE_CAT, &numDetections);
  if(OMCV_NORMAL != omcvResult) {
    PRINT_NAMED_WARNING("PetTracker.Update.OmcvGetResultCountFail",
                        "OMCV Result Code=%d", omcvResult);
    return RESULT_FAIL;
  }
  Toc("PetDetect");
  
  for(INT32 detectionIndex=0; detectionIndex<numDetections; ++detectionIndex)
  {
    DETECTION_INFO detectionInfo;
    omcvResult = OMCV_PD_GetResultInfo(_handles->omcvDetectionResult, (OBJ_TYPE_DOG|OBJ_TYPE_CAT), detectionIndex, &detectionInfo);

    if(OMCV_NORMAL != omcvResult) {
      PRINT_NAMED_WARNING("PetTracker.Update.OmcvGetResultInfoFail",
                          "Detection index %d of %d. OMCV Result Code=%d",
                          detectionIndex, numDetections, omcvResult);
      return RESULT_FAIL;
    }
    
    TrackedPet pet(detectionInfo.nID);
    
    switch(detectionInfo.nObjectType) 
    { 
      case OBJ_TYPE_DOG: 
        pet.SetType(Vision::PetType::Dog);
        break;
        
      case OBJ_TYPE_CAT:
        pet.SetType(Vision::PetType::Cat);
        break;
        
      default:
        PRINT_NAMED_WARNING("PetTracker.Update.InvalidPetType", 
                            "ObjectType=%d", detectionInfo.nObjectType);
        continue;
    } // switch(detectiONInfo.nObjectType                        
    
    pet.SetHeadRoll(DEG_TO_RAD((f32)detectionInfo.nAngle));
    
    pet.SetRect(Rectangle<f32>(detectionInfo.ptCenter.x-detectionInfo.nWidth/2,
                               detectionInfo.ptCenter.y-detectionInfo.nHeight/2,
                               detectionInfo.nWidth, detectionInfo.nHeight));
                            
    pet.SetScore(detectionInfo.nConfidence);
    
    pet.SetTimeStamp(frameOrig.GetTimestamp());
    
    pet.SetIsBeingTracked(detectionInfo.nDetectionMethod == DET_METHOD_TRACKED_MIDDLE);
    
    pets.emplace_back(std::move(pet));

  } // FOR each detection
  
  return RESULT_OK;
} // Update()
  
} // namespace Vision
} // namespace Anki


