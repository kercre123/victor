/**
 * File: faceTrackerImpl_okao.cpp
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

#if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

#include "faceTrackerImpl_okao.h"

#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/rotation.h"
#include "util/logging/logging.h"
#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Vision {
  
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

    switch(_detectionMode)
    {
      case FaceTracker::DetectionMode::Video:
      {
        _okaoDetectorHandle = OKAO_DT_CreateHandle(_okaoCommonHandle, DETECTION_MODE_MOVIE, MaxFaces);
        if(NULL == _okaoDetectorHandle) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoDetectionHandleAllocFail.VideoMode", "");
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

        okaoResult = OKAO_DT_MV_SetDirectionMask(_okaoDetectorHandle, false);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetDirectionMaskFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        okaoResult = OKAO_DT_MV_SetPoseExtension(_okaoDetectorHandle, true, true);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetPoseExtensionFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        okaoResult = OKAO_DT_MV_SetAccuracy(_okaoDetectorHandle, TRACKING_ACCURACY_HIGH);
        if(OKAO_NORMAL != okaoResult) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAccuracyFailed", "");
          return RESULT_FAIL_INVALID_PARAMETER;
        }
        
        break;
      }
        
      case FaceTracker::DetectionMode::SingleImage:
        _okaoDetectorHandle = OKAO_DT_CreateHandle(_okaoCommonHandle, DETECTION_MODE_STILL, MaxFaces);
        if(NULL == _okaoDetectorHandle) {
          PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoDetectionHandleAllocFail.StillMode", "");
          return RESULT_FAIL_MEMORY;
        }
        break;
        
      default:
        PRINT_NAMED_ERROR("FaceTrackerImpl.Init.UnknownDetectionMode",
                          "Requested mode = %hhu", _detectionMode);
        return RESULT_FAIL;
    }
    
    //okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_HALF_PROFILE,
    //                              ROLL_ANGLE_U45 | ROLL_ANGLE_2 | ROLL_ANGLE_10);
    okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_FRONT, ROLL_ANGLE_U45);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAngleFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_SetSizeRange(_okaoDetectorHandle, 60, 640);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetSizeRangeFailed", "");
      return RESULT_FAIL_INVALID_PARAMETER;
    }
    
    okaoResult = OKAO_DT_SetThreshold(_okaoDetectorHandle, 500);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetThresholdFailed", "");
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

    _okaoPartDetectionResultHandle2 = OKAO_PT_CreateResultHandle(_okaoCommonHandle);
    if(NULL == _okaoPartDetectionResultHandle2) {
      PRINT_NAMED_ERROR("FacetrackerImpl.Init.OkaoPartDetectionResultHandle2AllocFail", "");
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

    Result recognizerInitResult = _recognizer.Init(_okaoCommonHandle);
    
    if(RESULT_OK == recognizerInitResult) {
      
      _isInitialized = true;
      
      PRINT_NAMED_INFO("FaceTrackerImpl.Init.Success",
                       "OKAO Vision handles created successfully.");
    }
    
    return recognizerInitResult;
        
  } // Init()
  
  
  FaceTracker::Impl::~Impl()
  {
    //Util::SafeDeleteArray(_workingMemory);
    //Util::SafeDeleteArray(_backupMemory);

    
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
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectionResultHandle1DeleteFail", "");
      }
    }
    
    if(NULL != _okaoPartDetectionResultHandle2) {
      if(OKAO_NORMAL != OKAO_PT_DeleteResultHandle(_okaoPartDetectionResultHandle2)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoPartDetectionResultHandle2DeleteFail", "");
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
  
  
  bool FaceTracker::Impl::DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                          INT32 detectionIndex,
                                          Vision::TrackedFace& face)
  {
    INT32 okaoResult = OKAO_PT_SetPositionFromHandle(_okaoPartDetectorHandle, _okaoDetectionResultHandle, detectionIndex);
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetPositionFail",
                        "OKAO Result Code=%d", okaoResult);
      return false;
    }
    okaoResult = OKAO_PT_DetectPoint_GRAY(_okaoPartDetectorHandle, dataPtr,
                                          nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3, _okaoPartDetectionResultHandle);
    
    if(OKAO_NORMAL != okaoResult) {
      if(OKAO_ERR_PROCESSCONDITION != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoPartDetectionFail",
                          "OKAO Result Code=%d", okaoResult);
      }
      return false;
    }
    
    okaoResult = OKAO_PT_GetResult(_okaoPartDetectionResultHandle, PT_POINT_KIND_MAX,
                                   _facialParts, _facialPartConfs);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFacePartResultFail",
                        "OKAO Result Code=%d", okaoResult);
      return false;
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
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    //PRINT_NAMED_INFO("FaceTrackerImpl.Update.HeadOrientation",
    //                 "Roll=%ddeg, Pitch=%ddeg, Yaw=%ddeg",
    //                 roll_deg, pitch_deg, yaw_deg);
    
    face.SetHeadOrientation(DEG_TO_RAD(roll_deg),
                            DEG_TO_RAD(pitch_deg),
                            DEG_TO_RAD(yaw_deg));
    
    return true;
  }
  
  Result FaceTracker::Impl::EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                               Vision::TrackedFace& face)
  {
    INT32 okaoResult = OKAO_EX_SetPointFromHandle(_okaoEstimateExpressionHandle, _okaoPartDetectionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetExpressionPointFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_EX_Estimate_GRAY(_okaoEstimateExpressionHandle, dataPtr, nWidth, nHeight,
                                       GRAY_ORDER_Y0Y1Y2Y3, _okaoExpressionResultHandle);
    if(OKAO_NORMAL != okaoResult) {
      if(OKAO_ERR_PROCESSCONDITION == okaoResult) {
        // This might happen, depending on face parts
        PRINT_NAMED_INFO("FaceTrackerImpl.Update.OkaoEstimateExpressionNotPossible", "");
      } else {
        // This should not happen
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoEstimateExpressionFail",
                          "OKAO Result Code=%d", okaoResult);
        return RESULT_FAIL;
      }
    } else {
      
      okaoResult = OKAO_EX_GetResult(_okaoExpressionResultHandle, EX_EXPRESSION_KIND_MAX, _expressionValues);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetExpressionResultFail",
                          "OKAO Result Code=%d", okaoResult);
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

    return RESULT_OK;
  } // EstimateExpression()
  
  
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
    //TIC;
    Tic("FaceDetect");
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
    Toc("FaceDetect");
    
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

      const INT32 trackerID = detectionInfo.nID;
      if(detectionInfo.nDetectionMethod == DET_METHOD_DETECTED_HIGH) {
        // This face was found via detection
        auto iter = _trackingData.find(trackerID);
        if(iter != _trackingData.end()) {
          // The detector seems to be reusing an existing tracking ID we've seen
          // before. Remove it from the recognizer so we don't merge things inappropriately
          _recognizer.RemoveTrackingID(trackerID);
        }
        _trackingData[trackerID].assignedID = Vision::TrackedFace::UnknownFace;
        face.SetIsBeingTracked(false);
      } else {
        // This face was found via tracking: keep its existing recognition ID
        ASSERT_NAMED(_trackingData.find(trackerID) != _trackingData.end(),
                     "A tracked face should already have a recognition ID entry");
        face.SetIsBeingTracked(true);
      }
 
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
      Tic("FacePartDetection");
      bool facePartsFound = DetectFaceParts(nWidth, nHeight, dataPtr, detectionIndex, face);
      Toc("FacePartDetection");
      
      if(_detectEmotion && facePartsFound)
      {
        // Expression detection
        Tic("ExpressionRecognition");
        Result expResult = EstimateExpression(nWidth, nHeight, dataPtr, face);
        Toc("ExpressionRecognition");
        if(RESULT_OK != expResult) {
          PRINT_NAMED_WARNING("FaceTrackerImpl.Update.EstimateExpressiongFailed",
                              "Detection index %d of %d.",
                              detectionIndex, numDetections);
        }
      } // if(_detectEmotion)
      
      // Face Recognition:
      if(facePartsFound)
      {
        bool recognizing = _recognizer.SetNextFaceToRecognize(frameOrig, trackerID,
                                                              detectionInfo,
                                                              _okaoPartDetectionResultHandle);
        if(recognizing) {
          // The FaceRecognizer is now using whatever the partDetectionResultHandle is pointing to.
          // Switch to using the other handle so we don't step on its toes.
          std::swap(_okaoPartDetectionResultHandle, _okaoPartDetectionResultHandle2);
        }
      }
      
      // Get whatever is the latest recognition information for the current tracker ID
      auto recognitionData = _recognizer.GetRecognitionData(trackerID);
      
      if(recognitionData.isNew) {
        face.SetThumbnail(_recognizer.GetEnrollmentImage(recognitionData.faceID));
      }
      
      /*
      TrackingData& trackingData = _trackingData[trackerID];
      Vision::TrackedFace::ID_t faceID = trackingData.assignedID;
      INT32 recognitionScore = 0;
      if(facePartsFound)
      {
       
        // We've now assigned an identity to this tracker ID so we don't have to
        // keep trying to recognize it while we're successfully tracking!
        trackingData.assignedID = faceID;
        
        Toc("FaceRecognition");
      } // if(facePartsFound)
      */
      
      face.SetID(recognitionData.faceID); // could be unknown!
      face.SetScore(recognitionData.score); // could still be zero!
      if(TrackedFace::UnknownFace == recognitionData.faceID) {
        face.SetName("Unknown");
      } else {
        _trackingData[trackerID].assignedID = recognitionData.faceID;
        if(recognitionData.name.empty()) {
          // Known, unnamed face
          face.SetName("KnownFace" + std::to_string(recognitionData.faceID));
        } else {
          face.SetName(recognitionData.name);
        }
      }
      
    } // FOR each face
    
    return RESULT_OK;
  } // Update()
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    return _faces;
  }
  
  void FaceTracker::Impl::AssignNameToID(TrackedFace::ID_t faceID, const std::string& name)
  {
    _recognizer.AssignNameToID(faceID, name);
  }
  
  Result FaceTracker::Impl::SaveAlbum(const std::string& albumName)
  {
    return _recognizer.SaveAlbum(albumName);
  }
  
  Result FaceTracker::Impl::LoadAlbum(const std::string& albumName)
  {
    // Initialize on first use
    if(!_isInitialized) {
      Result initResult = Init();
      
      if(!_isInitialized || initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.InitFailed", "");
        return RESULT_FAIL;
      }
    }
    
    return _recognizer.LoadAlbum(_okaoCommonHandle, albumName);
  }
  
} // namespace Vision
} // namespace Anki

#endif // #if FACE_TRACKER_PROVIDER == FACE_TRACKER_OKAO

