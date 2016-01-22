/**
 * File: faceTrackerImpl_okao.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for OKAO Vision face detection library.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/rotation.h"
#include "util/logging/logging.h"

// Omron OKAO Vision
#include <OkaoAPI.h>
#include <OkaoDtAPI.h> // Face Detection
#include <OkaoPtAPI.h> // Face parts detection
#include <OkaoExAPI.h> // Expression recognition
#include <OkaoFrAPI.h> // Face Recognition
#include <CommonDef.h>
#include <DetectorComDef.h>

#include <list>

#define SHOW_TIMING 0

#if SHOW_TIMING
#  include <chrono>
static std::chrono::time_point<std::chrono::system_clock> __TICTOC__;
#  define TIC __TICTOC__ = std::chrono::system_clock::now()
#  define TOC(__MSG__) PRINT_NAMED_INFO(__MSG__, "%llums", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()-__TICTOC__).count())
#else
#  define TIC
#  define TOC(__MSG__)
#endif

namespace Anki {
namespace Vision {
  
  class FaceTracker::Impl
  {
  public:
    Impl(const std::string& modelPath, FaceTracker::DetectionMode mode);
    ~Impl();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled) { }
    
    static bool IsRecognitionSupported() { return true; }
    
  private:
    
    Result Init();
    Result RegisterNewUser(HFEATURE& hFeature);
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature);
    
    bool _isInitialized = false;
    FaceTracker::DetectionMode _detectionMode;
    
    static const s32 MaxFaces = 10; // detectable at once
    static const INT32 MaxAlbumDataPerFace = 10; // can't be more than 10
    static const INT32 MaxAlbumFaces = 100; // can't be more than 1000
    
    static_assert(MaxAlbumFaces <= 1000 && MaxAlbumFaces > 1,
                  "MaxAlbumFaces should be between 1 and 1000 for OKAO Library.");
    static_assert(MaxAlbumDataPerFace > 1 && MaxAlbumDataPerFace <= 10,
                  "MaxAlbumDataPerFace should be between 1 and 10 for OKAO Library.");
    
    INT32 _lastRegisteredUserID = 0;
    
    std::list<TrackedFace> _faces;
    
    //u8* _workingMemory = nullptr;
    //u8* _backupMemory  = nullptr;
    
    // Okao Vision Library "Handles"
    HCOMMON     _okaoCommonHandle              = NULL;
    HDETECTION  _okaoDetectorHandle            = NULL;
    HDTRESULT   _okaoDetectionResultHandle     = NULL;
    HPOINTER    _okaoPartDetectorHandle        = NULL;
    HPTRESULT   _okaoPartDetectionResultHandle = NULL;
    HEXPRESSION _okaoEstimateExpressionHandle  = NULL;
    HEXPRESSION _okaoExpressionResultHandle    = NULL;
    HFEATURE    _okaoRecognitionFeatureHandle  = NULL;
    HALBUM      _okaoFaceAlbum                 = NULL;
    
    // Space for detected face parts / expressions
    POINT _facialParts[PT_POINT_KIND_MAX];
    INT32 _facialPartConfs[PT_POINT_KIND_MAX];
    INT32 _expressionValues[EX_EXPRESSION_KIND_MAX];
    
  }; // class FaceTracker::Impl
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath, FaceTracker::DetectionMode mode)
  : _detectionMode(mode)
  {
    
  } // Impl Constructor()
  
  Result FaceTracker::Impl::Init()
  {
    // Get and pring Okao library version as a sanity check that we can even
    // talk to the library
    UINT8 okaoVersionMajor=0, okaoVersionMinor = 0;
    INT32 okaoResult = OKAO_CO_GetVersion(&okaoVersionMajor, &okaoVersionMinor);
    if(okaoResult != OKAO_NORMAL) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoVersionFail", "");
      return RESULT_FAIL;
    }
    PRINT_NAMED_INFO("FaceTrackerImpl.Init.OkaoVersion",
                     "Initializing with OkaoVision version %d.%d",
                     okaoVersionMajor, okaoVersionMinor);
    
    /*
    // Allocate memory for the Okao libraries
    _workingMemory = new u8[MaxFaces*800 + 60*1024]; // 0.7KB × [max detection count] + 60KB
    _backupMemory  = new u8[MaxFaces*4096 + (250+70)*1024];
    
    if(_workingMemory == nullptr || _backupMemory == nullptr) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.MemoryAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    */
    
    _okaoCommonHandle = OKAO_CO_CreateHandle();
    if(NULL == _okaoCommonHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoCommonHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }

    auto okaoDetectionMode = DETECTION_MODE_DEFAULT;
    switch(_detectionMode)
    {
      case FaceTracker::DetectionMode::Video:
        okaoDetectionMode = DETECTION_MODE_MOVIE;
        break;
      case FaceTracker::DetectionMode::SingleImage:
        okaoDetectionMode = DETECTION_MODE_STILL;
        break;
      default:
        PRINT_NAMED_ERROR("FaceTrackerImpl.Init.UnknownDetectionMode",
                          "Requested mode = %hhu", _detectionMode);
        return RESULT_FAIL;
    }
    _okaoDetectorHandle = OKAO_DT_CreateHandle(_okaoCommonHandle, okaoDetectionMode, MaxFaces);
        
    if(NULL == _okaoDetectorHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoDetectionHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    // Adjust some detection parameters
    // TODO: Expose these for setting at runtime
    okaoResult = OKAO_DT_MV_SetDelayCount(_okaoDetectorHandle, 1); // have to see faces for more than one frame
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetDelayCountFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_MV_SetSearchCycle(_okaoDetectorHandle, 2, 2, 5);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetSearchCycleFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_HALF_PROFILE, ROLL_ANGLE_ULR45);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAngleFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    _okaoDetectionResultHandle = OKAO_DT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoDetectionResultHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoDetectionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoPartDetectorHandle = OKAO_PT_CreateHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectorHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectorHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoPartDetectionResultHandle = OKAO_PT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectionResultHandle) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoEstimateExpressionHandle = OKAO_EX_CreateHandle(_okaoCommonHandle);
    if(NULL == _okaoEstimateExpressionHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoEstimateExpressionHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoExpressionResultHandle = OKAO_EX_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoExpressionResultHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoExpressionResultHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoRecognitionFeatureHandle = OKAO_FR_CreateFeatureHandle(_okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoFeatureHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _okaoFaceAlbum = OKAO_FR_CreateAlbumHandle(_okaoCommonHandle, MaxAlbumFaces, MaxAlbumDataPerFace);
    if(NULL == _okaoFaceAlbum) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoAlbumHandleAllocFail", "");
      return RESULT_FAIL_MEMORY;
    }
    
    _isInitialized = true;
    
    PRINT_NAMED_INFO("FaceTrackerImpl.Init.Success",
                     "OKAO Vision handles created successfully.");
    return RESULT_OK;
  } // Init()
  
  FaceTracker::Impl::~Impl()
  {
    //Util::SafeDeleteArray(_workingMemory);
    //Util::SafeDeleteArray(_backupMemory);

    if(NULL != _okaoFaceAlbum) {
      
      // TODO: Save album first!
      
      if(OKAO_NORMAL != OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoAlbumHandleDeleteFail", "");
      }
    }
    if(NULL != _okaoRecognitionFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecognitionFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoRecognitionFeatureHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoExpressionResultHandle) {
      if(OKAO_NORMAL != OKAO_EX_DeleteResultHandle(_okaoExpressionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoExpressionResultHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoEstimateExpressionHandle) {
      if(OKAO_NORMAL != OKAO_EX_DeleteHandle(_okaoEstimateExpressionHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoEstimateExpressionHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectionResultHandle) {
      if(OKAO_NORMAL != OKAO_PT_DeleteResultHandle(_okaoPartDetectionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectionResultHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectorHandle) {
      if(OKAO_NORMAL != OKAO_PT_DeleteHandle(_okaoPartDetectorHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectorHandleDeleteFail", "");
      }
    }

    if(NULL != _okaoDetectionResultHandle) {
      if(OKAO_NORMAL != OKAO_DT_DeleteResultHandle(_okaoDetectionResultHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoDetectionResultHandleDeleteFail", "");
      }
    }
    
    if(NULL != _okaoDetectorHandle) {
      if(OKAO_NORMAL != OKAO_DT_DeleteHandle(_okaoDetectorHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoDetectorHandleDeleteFail", "");
      }
      _okaoDetectorHandle = NULL;
    }
    
    if(NULL != _okaoCommonHandle) {
      if(OKAO_NORMAL != OKAO_CO_DeleteHandle(_okaoCommonHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoCommonHandleDeleteFail", "");
      }
      _okaoCommonHandle = NULL;
    }
    
    _isInitialized = false;
  } // ~Impl()
  
  static inline void SetFeatureHelper(const POINT* faceParts, std::vector<s32>&& indices,
                                      TrackedFace::FeatureName whichFeature,
                                      TrackedFace& face)
  {
    TrackedFace::Feature feature;
    bool allPointsPresent = true;
    for(auto index : indices) {
      if(faceParts[index].x == FEATURE_NO_POINT ||
         faceParts[index].y == FEATURE_NO_POINT)
      {
        allPointsPresent = false;
        break;
      }
      feature.emplace_back(faceParts[index].x, faceParts[index].y);
    }
    
    if(allPointsPresent) {
      face.SetFeature(whichFeature, std::move(feature));
    }
  } // SetFeatureHelper()
  
  Result FaceTracker::Impl::RegisterNewUser(HFEATURE& hFeature)
  {
    INT32 numUsersInAlbum = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    if(numUsersInAlbum >= MaxAlbumFaces) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.TooManyUsers",
                        "Already have %d users, could not add another", MaxAlbumFaces);
      return RESULT_FAIL;
    }

    // Find unused ID
    BOOL isRegistered = true;
    while(isRegistered)
    {
      ++_lastRegisteredUserID;
      
      okaoResult = OKAO_FR_IsRegistered(_okaoFaceAlbum, _lastRegisteredUserID, 0, &isRegistered);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.IsRegisteredCheckFailed",
                          "Failed to determine if user ID %d is already registered",
                          numUsersInAlbum);
        return RESULT_FAIL;
      }
    }
    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, _lastRegisteredUserID, 0);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RegisterNewUser.RegisterFailed",
                        "Failed trying to register user %d", _lastRegisteredUserID);
      return RESULT_FAIL;
    }
    
    PRINT_NAMED_INFO("FaceTrackerImpl.RegisterNewUser.Success",
                     "Added user number %d to album", _lastRegisteredUserID);
    
    return RESULT_OK;
  } // RegisterNewUser()
  
  Result FaceTracker::Impl::UpdateExistingUser(INT32 userID, HFEATURE& hFeature)
  {
    INT32 numUserData = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, userID, &numUserData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.GetNumDataFailed",
                        "Failed to get num data for user %d", userID);
      return RESULT_FAIL;
    }
    
    if(numUserData >= MaxAlbumDataPerFace) {
      //PRINT_NAMED_INFO("FaceTrackerImpl.UpdateExistingUser.MaxNumDataPerUser",
      //                 "Reached maximum of %d data for user %d. Replacing last.",
      //                 MaxAlbumDataPerFace, userID);
      numUserData = MaxAlbumDataPerFace - 1;
      
      // TODO: Is clearing necessary if we're about to replace it anyway?
      okaoResult = OKAO_FR_ClearData(_okaoFaceAlbum, userID, numUserData);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.ClearDataFailed",
                          "Failed to clear data %d for user %d",
                          numUserData, userID);
        return RESULT_FAIL;
      }
    }
    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, userID, numUserData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.RegisterFailed",
                        "Failed to trying to register data %d for existing user %d",
                        numUserData, userID);
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // UpdateExistingUser()
  
  Result FaceTracker::Impl::Update(const Vision::Image& frameOrig)
  {
    // Initialize on first use
    if(!_isInitialized) {
      Result initResult = Init();
      
      if(!_isInitialized || initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.InitFailed", "");
        return RESULT_FAIL;
      }
    }
    
    ASSERT_NAMED(frameOrig.IsContinuous(),
                 "Image must be continuous to pass straight into OKAO Detector");
    
    INT32 okaoResult = OKAO_NORMAL;
    TIC;
    const INT32 nWidth  = frameOrig.GetNumCols();
    const INT32 nHeight = frameOrig.GetNumRows();
    RAWIMAGE* dataPtr = const_cast<UINT8*>(frameOrig.GetDataPointer());
    okaoResult = OKAO_DT_Detect_GRAY(_okaoDetectorHandle, dataPtr, nWidth, nHeight,
                                     GRAY_ORDER_Y0Y1Y2Y3, _okaoDetectionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoDetectFail", "OKAO Result Code=%d",
                        okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numDetections = 0;
    okaoResult = OKAO_DT_GetResultCount(_okaoDetectionResultHandle, &numDetections);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetResultCountFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    TOC("FaceDetectTime");
    
    _faces.clear();
    
    for(INT32 detectionIndex=0; detectionIndex<numDetections; ++detectionIndex)
    {
      DETECTION_INFO detectionInfo;
      okaoResult = OKAO_DT_GetRawResultInfo(_okaoDetectionResultHandle, detectionIndex,
                                            &detectionInfo);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetResultInfoFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      // Add a new face to the list
      _faces.emplace_back();
      
      TrackedFace& face = _faces.back();
      face.SetID(detectionInfo.nID);

      POINT ptLeftTop, ptRightTop, ptLeftBottom, ptRightBottom;
      okaoResult = OKAO_CO_ConvertCenterToSquare(detectionInfo.ptCenter,
                                                 detectionInfo.nHeight,
                                                 0, &ptLeftTop, &ptRightTop,
                                                 &ptLeftBottom, &ptRightBottom);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoCenterToSquareFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      face.SetRect(Rectangle<f32>(ptLeftTop.x, ptLeftTop.y,
                                  ptRightBottom.x-ptLeftTop.x,
                                  ptRightBottom.y-ptLeftTop.y));
      
      face.SetTimeStamp(frameOrig.GetTimestamp());
      
      // Try finding face parts
      TIC;
      okaoResult = OKAO_PT_SetPositionFromHandle(_okaoPartDetectorHandle, _okaoDetectionResultHandle, detectionIndex);

      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetPositionFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      okaoResult = OKAO_PT_DetectPoint_GRAY(_okaoPartDetectorHandle, dataPtr,
                                            nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3, _okaoPartDetectionResultHandle);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoPartDetectionFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_PT_GetResult(_okaoPartDetectionResultHandle, PT_POINT_KIND_MAX,
                                     _facialParts, _facialPartConfs);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFacePartResultFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }

      // Set eye centers
      face.SetLeftEyeCenter(Point2f(_facialParts[PT_POINT_LEFT_EYE].x,
                                    _facialParts[PT_POINT_LEFT_EYE].y));
      
      face.SetRightEyeCenter(Point2f(_facialParts[PT_POINT_RIGHT_EYE].x,
                                     _facialParts[PT_POINT_RIGHT_EYE].y));

      // Set other facial features
      SetFeatureHelper(_facialParts, {
        PT_POINT_LEFT_EYE_OUT, PT_POINT_LEFT_EYE, PT_POINT_LEFT_EYE_IN
      }, TrackedFace::FeatureName::LeftEye, face);
      
      SetFeatureHelper(_facialParts, {
        PT_POINT_RIGHT_EYE_IN, PT_POINT_RIGHT_EYE, PT_POINT_RIGHT_EYE_OUT
      }, TrackedFace::FeatureName::RightEye, face);

      SetFeatureHelper(_facialParts, {
        PT_POINT_NOSE_LEFT, PT_POINT_NOSE_RIGHT
      }, TrackedFace::FeatureName::Nose, face);
      
      SetFeatureHelper(_facialParts, {
        PT_POINT_MOUTH_LEFT, PT_POINT_MOUTH_UP, PT_POINT_MOUTH_RIGHT,
        PT_POINT_MOUTH, PT_POINT_MOUTH_LEFT,
      }, TrackedFace::FeatureName::UpperLip, face);

      
      // Fill in head orientation
      INT32 roll_deg=0, pitch_deg=0, yaw_deg=0;
      okaoResult = OKAO_PT_GetFaceDirection(_okaoPartDetectionResultHandle, &pitch_deg, &yaw_deg, &roll_deg);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFaceDirectionFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      //PRINT_NAMED_INFO("FaceTrackerImpl.Update.HeadOrientation",
      //                 "Roll=%ddeg, Pitch=%ddeg, Yaw=%ddeg",
      //                 roll_deg, pitch_deg, yaw_deg);
      
      face.SetHeadOrientation(DEG_TO_RAD(roll_deg),
                              DEG_TO_RAD(pitch_deg),
                              DEG_TO_RAD(yaw_deg));
      
      TOC("FacePartDetection");
      
      // Expression detection
      TIC;
      okaoResult = OKAO_EX_SetPointFromHandle(_okaoEstimateExpressionHandle, _okaoPartDetectionResultHandle);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetExpressionPointFail",
                          "Detection index %d of %d. OKAO Result Code=%d",
                          detectionIndex, numDetections, okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_EX_Estimate_GRAY(_okaoEstimateExpressionHandle, dataPtr, nWidth, nHeight,
                                         GRAY_ORDER_Y0Y1Y2Y3, _okaoExpressionResultHandle);
      if(OKAO_NORMAL != okaoResult) {
        if(OKAO_ERR_PROCESSCONDITION == okaoResult) {
          // This might happen, depending on face parts
          PRINT_NAMED_INFO("FaceTrackerImpl.Update.OkaoEstimateExpression",
                           "Could not estimate expression for face %d of %d using detected parts.",
                           detectionIndex, numDetections);
        } else {
          // This should not happen
          PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoEstimateExpressionFail",
                            "Detection index %d of %d. OKAO Result Code=%d",
                            detectionIndex, numDetections, okaoResult);
          return RESULT_FAIL;
        }
      } else {
        
        okaoResult = OKAO_EX_GetResult(_okaoExpressionResultHandle, EX_EXPRESSION_KIND_MAX, _expressionValues);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetExpressionResultFail",
                            "Detection index %d of %d. OKAO Result Code=%d",
                            detectionIndex, numDetections, okaoResult);
          return RESULT_FAIL;
        }
        
        static const TrackedFace::Expression TrackedFaceExpressionLUT[EX_EXPRESSION_KIND_MAX] = {
          TrackedFace::Neutral,
          TrackedFace::Happiness,
          TrackedFace::Surprise,
          TrackedFace::Anger,
          TrackedFace::Sadness
        };
        
        for(INT32 okaoExpressionVal = 0; okaoExpressionVal < EX_EXPRESSION_KIND_MAX; ++okaoExpressionVal) {
          face.SetExpressionValue(TrackedFaceExpressionLUT[okaoExpressionVal],
                                  _expressionValues[okaoExpressionVal]);
        }
        
      }
      TOC("ExpressionRecognitionTime");
      
      // Face Recognition:
      INT32 numUsersInAlbum = 0;
      okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetNumUsersInAlbumFailed", "");
        return RESULT_FAIL;
      }
      
      TIC;
      okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                              dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                              _okaoPartDetectionResultHandle);
      
      if(numUsersInAlbum == 0) {
        // Nobody in album yet, add this person
        Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle);
        if(RESULT_OK != lastResult) {
          return lastResult;
        }
      } else {
        INT32 resultNum = 0;
        const s32 MaxResults = 2;
        INT32 userIDs[MaxResults], scores[MaxResults];
        okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, MaxResults, userIDs, scores, &resultNum);
        
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_WARNING("FaceTrackerImpl.Update.OkaoFaceRecognitionIdentifyFailed",
                              "Detection index %d of %d. OKAO Result Code=%d",
                              detectionIndex, numDetections, okaoResult);
        } else {
          const INT32 RecognitionThreshold = 750; // TODO: Expose this parameter
          const INT32 MergeThreshold       = 500; // TODO: Expose this parameter
          //const f32   RelativeRecogntionThreshold = 1.5; // Score of top result must be this times the score of the second best result
          
          if(resultNum > 0 && scores[0] > RecognitionThreshold)
          {
            INT32 minID = userIDs[0];

            // Merge all results that are above threshold together, keeping the lowest ID
            INT32 lastResult = 1;
            while(lastResult < resultNum && scores[lastResult] > MergeThreshold) {
              minID = std::min(userIDs[lastResult], minID);
              ++lastResult;
            }
            
            for(INT32 iResult = 0; iResult < lastResult; ++iResult)
            {
              if(userIDs[iResult] != minID)
              {
                PRINT_NAMED_INFO("FaceTrackerImpl.Update.MergingRecords",
                                 "Merging face %d with %d b/c its score %d is > %d",
                                 userIDs[iResult], minID, scores[iResult], MergeThreshold);
                
                Result lastResult = UpdateExistingUser(minID, _okaoRecognitionFeatureHandle);
                
                if(RESULT_OK != lastResult) {
                  return lastResult;
                }
                
                okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, userIDs[iResult]);
                if(OKAO_NORMAL != okaoResult) {
                  PRINT_NAMED_ERROR("FaceTrackerImpl.Update.ClearUserFailed",
                                    "Failed to clear user %d after merging it with %d",
                                    userIDs[iResult], minID);
                  return RESULT_FAIL;
                }
              }
              ++iResult;
            }
            
            face.SetID(minID);
            face.SetName("KnownFace" + std::to_string(face.GetID()));

          } else {
            // No match found, add new user
            Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle);
            if(RESULT_OK != lastResult) {
              return lastResult;
            }
          }
          
        }
      }
      TOC("FaceRecTime");
      
    } // FOR each face
    
    return RESULT_OK;
  } // Update()
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    return _faces;
  }
  
} // namespace Vision
} // namespace Anki

