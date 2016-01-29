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

#define SHOW_TIMING 1

#if SHOW_TIMING
#  include <chrono>
#  include <unordered_map>
#endif

namespace Anki {

#if SHOW_TIMING
  class Profiler
  {
  public:
    
    // Calls Print() on destruction
    ~Profiler();
    
    // Start named timer (create if first call, un-pause otherwise)
    void Tic(const char* timerName);
    
    // Pause named timer and return time since last tic in milliseconds
    double Toc(const char* timerName);
    
    // Pause named timer and return average time in milliseconds for all tic/toc
    // pairs up to now
    double AverageToc(const char* timerName);
    
    // Print average time per tic/toc pair for each registered timer
    void Print() const;
    
  private:
    
    struct Timer {
      std::chrono::time_point<std::chrono::system_clock> startTime;
      std::chrono::milliseconds currentTime;
      std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
      s32 count = 0;
      
      void Update();
    };
    
    using TimerContainer = std::unordered_map<const char*, Profiler::Timer>;
    
    TimerContainer _timers;
    
  }; // class Profiler
  
  void Profiler::Tic(const char* timerName)
  {
    // Note will construct timer if matching name doesn't already exist
    _timers[timerName].startTime = std::chrono::system_clock::now();
  }
  
  void Profiler::Timer::Update()
  {
    using namespace std::chrono;
    currentTime = duration_cast<milliseconds>(system_clock::now() - startTime);
    totalTime += currentTime;
    ++count;
  }
  
  double Profiler::Toc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      return (u32)timer.currentTime.count();
    } else {
      return 0;
    }
  }
  
  double Profiler::AverageToc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      if(timer.count > 0) {
        return (double)(timer.totalTime.count() / (double)timer.count);
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  
  Profiler::~Profiler()
  {
    Print();
  }
  
  void Profiler::Print() const
  {
    for(auto & timerPair : _timers)
    {
      const Timer& timer = timerPair.second;
      PRINT_NAMED_INFO("Profiler", "%s averaged %.2fms over %d calls",
                       timerPair.first,
                       (timer.count > 0 ? (double)timer.totalTime.count() / (double)timer.count : 0),
                       timer.count);
    }
  }
#else 
  class Profiler {
  public:
    void Tic(const char*) { }
    void Toc(const char*) { }
  };
  
#endif // SHOW_TIMING
  
namespace Vision {
  
  class FaceTracker::Impl : public Profiler
  {
  public:
    Impl(const std::string& modelPath, FaceTracker::DetectionMode mode);
    ~Impl();
    
    Result Update(const Vision::Image& frameOrig);
    
    std::list<TrackedFace> GetFaces() const;
    
    void EnableDisplay(bool enabled) { }
    
    static bool IsRecognitionSupported() { return true; }
    
    void EnableNewFaceEnrollment(bool enable) { _enrollNewFaces = enable; }
    bool IsNewFaceEnrollmentEnabled() const   { return _enrollNewFaces; }
    
    void EnableEmotionDetection(bool enable) { _detectEmotion = enable; }
    bool IsEmotionDetectionEnabled() const   { return _detectEmotion;  }
    
    std::vector<u8> GetSerializedAlbum();
    Result SetSerializedAlbum(const std::vector<u8>&serializedAlbum);
    
  private:
    
    Result Init();
    Result RegisterNewUser(HFEATURE& hFeature, TimeStamp_t obsTime,
                           const DETECTION_INFO& detectionInfo);
    
    Result UpdateExistingUser(INT32 userID, HFEATURE& hFeature, TimeStamp_t obsTime,
                              const DETECTION_INFO& detectionInfo);
    
    Result DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                           INT32 detectionIndex, Vision::TrackedFace& face);
    
    Result EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                              Vision::TrackedFace& face);
    
    Result RecognizeFace(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                         const DETECTION_INFO& detectionInfo, TimeStamp_t timestamp,
                         Vision::TrackedFace::ID_t& faceID);
    
    Result MergeFaces(Vision::TrackedFace::ID_t keepID, Vision::TrackedFace::ID_t mergeID);

    Result RemoveUser(INT32 userID);
    
    bool _isInitialized = false;
    FaceTracker::DetectionMode _detectionMode;
    bool _enrollNewFaces = true;
    bool _detectEmotion = true;
    
    static const s32   MaxFaces = 10; // detectable at once
    static const INT32 MaxAlbumDataPerFace = 10; // can't be more than 10
    static const INT32 MaxAlbumFaces = 100; // can't be more than 1000
    static const s32   TimeBetweenEnrollmentUpdates_ms = 2000;
    
    static_assert(MaxAlbumFaces <= 1000 && MaxAlbumFaces > 1,
                  "MaxAlbumFaces should be between 1 and 1000 for OKAO Library.");
    static_assert(MaxAlbumDataPerFace > 1 && MaxAlbumDataPerFace <= 10,
                  "MaxAlbumDataPerFace should be between 1 and 10 for OKAO Library.");
    
    INT32 _lastRegisteredUserID = 0;
    
    std::list<TrackedFace> _faces;
    
    // Mapping from tracking ID to recognition (identity) ID
    struct TrackingData {
      Vision::TrackedFace::ID_t assignedID = Vision::TrackedFace::UnknownFace;
    };
    std::map<INT32, TrackingData> _trackingData;
    
    // Track the index of the oldest data for each registered user, so we can
    // replace the oldest one each time
    struct EnrollmentStatus {
      INT32        oldestData = 0;
      TimeStamp_t  lastEnrollmentTimeStamp = 0;
    };
    std::map<INT32,EnrollmentStatus> _enrollmentStatus;
    
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
    HFEATURE    _okaoRecogMergeFeatureHandle   = NULL;
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
    
    okaoResult = OKAO_DT_SetAngle(_okaoDetectorHandle, POSE_ANGLE_HALF_PROFILE,
                                  ROLL_ANGLE_U45 | ROLL_ANGLE_2 | ROLL_ANGLE_10);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoSetAngleFailed", "");
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
    
    _okaoRecogMergeFeatureHandle = OKAO_FR_CreateFeatureHandle(_okaoCommonHandle);
    if(NULL == _okaoRecognitionFeatureHandle) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Init.OkaoMergeFeatureHandleAllocFail", "");
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
    
    if(NULL != _okaoRecogMergeFeatureHandle) {
      if(OKAO_NORMAL != OKAO_FR_DeleteFeatureHandle(_okaoRecogMergeFeatureHandle)) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.Destructor.OkaoRecognitionMergeFeatureHandleDeleteFail", "");
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
  
  bool IsRecognizable(const DETECTION_INFO& detectionInfo)
  {
    if(detectionInfo.nConfidence > 500 &&
       detectionInfo.nPose == POSE_YAW_FRONT &&
       (detectionInfo.nAngle < 45 || detectionInfo.nAngle > 315))
    {
      return true;
    }
    return false;
  }

  Result FaceTracker::Impl::RegisterNewUser(HFEATURE& hFeature, TimeStamp_t obsTime,
                                            const DETECTION_INFO& detectionInfo)
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
    
    auto & enrollStatus = _enrollmentStatus[_lastRegisteredUserID];
    //enrollStatus.detectionInfo[GetDataIndex(detectionInfo)] = detectionInfo;
    enrollStatus.lastEnrollmentTimeStamp = obsTime;
    
    PRINT_NAMED_INFO("FaceTrackerImpl.RegisterNewUser.Success",
                     "Added user number %d to album", _lastRegisteredUserID);
    
    return RESULT_OK;
  } // RegisterNewUser()

  
  Result FaceTracker::Impl::UpdateExistingUser(INT32 userID, HFEATURE& hFeature, TimeStamp_t obsTime,
                                               const DETECTION_INFO& detectionInfo)
  {
    /*
    const INT32 dataToReplace = GetDataIndex(detectionInfo);

    // TODO: Is clearing necessary if we're about to replace it anyway?
    INT32 okaoResult = OKAO_FR_ClearData(_okaoFaceAlbum, userID, dataToReplace);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.ClearDataFailed",
                        "Failed to clear data %d for user %d. OKAO result=%d",
                        dataToReplace, userID, okaoResult);
      return RESULT_FAIL;
    }
    */

    INT32 numUserData = 0;
    INT32 okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, userID, &numUserData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.GetNumDataFailed",
                        "Failed to get num data for user %d", userID);
      return RESULT_FAIL;
    }
    
    INT32 dataToReplace = numUserData-1;
    if(numUserData >= MaxAlbumDataPerFace) {
      
      
      // Replace the oldest face data
      // TODO: Consider other methods (e.g. lowest/highest score)
      auto enrollStatusIter = _enrollmentStatus.find(userID);
      ASSERT_NAMED(enrollStatusIter != _enrollmentStatus.end(), "Missing enrollment status");
      auto & enrollStatus = enrollStatusIter->second;
      if(obsTime - enrollStatus.lastEnrollmentTimeStamp > TimeBetweenEnrollmentUpdates_ms) {
        dataToReplace = enrollStatus.oldestData;
        enrollStatus.lastEnrollmentTimeStamp = obsTime;
        enrollStatus.oldestData++;
        if(enrollStatus.oldestData == MaxAlbumDataPerFace) {
          enrollStatus.oldestData = 0;
        }
      }
      
      // TODO: Is clearing necessary if we're about to replace it anyway?
      okaoResult = OKAO_FR_ClearData(_okaoFaceAlbum, userID, dataToReplace);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.ClearDataFailed",
                          "Failed to clear data %d for user %d. OKAO result=%d",
                          numUserData, userID, okaoResult);
        return RESULT_FAIL;
      }
    }

    
    okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, hFeature, userID, dataToReplace);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.UpdateExistingUser.RegisterFailed",
                        "Failed to trying to register data %d for existing user %d",
                        dataToReplace, userID);
      return RESULT_FAIL;
    }
    
    /*
    auto enrollStatusIter = _enrollmentStatus.find(userID);
    ASSERT_NAMED(enrollStatusIter != _enrollmentStatus.end(),
                 "Enrollment status should exist for use when updating");
    enrollStatusIter->second.detectionInfo[dataToReplace] = detectionInfo;
    enrollStatusIter->second.lastEnrollmentTimeStamp      = timestamp;
    */
    
    return RESULT_OK;
  } // UpdateExistingUser()
  
  
  Result FaceTracker::Impl::DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                            INT32 detectionIndex,
                                            Vision::TrackedFace& face)
  {
    INT32 okaoResult = OKAO_PT_SetPositionFromHandle(_okaoPartDetectorHandle, _okaoDetectionResultHandle, detectionIndex);
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoSetPositionFail",
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    okaoResult = OKAO_PT_DetectPoint_GRAY(_okaoPartDetectorHandle, dataPtr,
                                          nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3, _okaoPartDetectionResultHandle);
    
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoPartDetectionFail",
                        "OKAO Result Code=%d%s", okaoResult,
                        (OKAO_ERR_PROCESSCONDITION == okaoResult ?
                         " (Unsupported yaw angle)" : ""));
      return RESULT_FAIL;
    }
    
    okaoResult = OKAO_PT_GetResult(_okaoPartDetectionResultHandle, PT_POINT_KIND_MAX,
                                   _facialParts, _facialPartConfs);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.Update.OkaoGetFacePartResultFail",
                        "OKAO Result Code=%d", okaoResult);
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
                        "OKAO Result Code=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    //PRINT_NAMED_INFO("FaceTrackerImpl.Update.HeadOrientation",
    //                 "Roll=%ddeg, Pitch=%ddeg, Yaw=%ddeg",
    //                 roll_deg, pitch_deg, yaw_deg);
    
    face.SetHeadOrientation(DEG_TO_RAD(roll_deg),
                            DEG_TO_RAD(pitch_deg),
                            DEG_TO_RAD(yaw_deg));
    
    return RESULT_OK;
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
  
  Result FaceTracker::Impl::RemoveUser(INT32 userID)
  {
    INT32 okaoResult = OKAO_FR_ClearUser(_okaoFaceAlbum, userID);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RemoveUser.ClearUserFailed",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    _enrollmentStatus.erase(userID);
    
    return RESULT_OK;
  }
  
  Result FaceTracker::Impl::RecognizeFace(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                                          const DETECTION_INFO& detectionInfo, TimeStamp_t timestamp,
                                          Vision::TrackedFace::ID_t& faceID)
  {
    INT32 okaoResult = OKAO_FR_ExtractHandle_GRAY(_okaoRecognitionFeatureHandle,
                                                  dataPtr, nWidth, nHeight, GRAY_ORDER_Y0Y1Y2Y3,
                                                  _okaoPartDetectionResultHandle);
    
    const bool allowAddNewFace = _enrollNewFaces && IsRecognizable(detectionInfo);
    
    INT32 numUsersInAlbum = 0;
    okaoResult = OKAO_FR_GetRegisteredUserNum(_okaoFaceAlbum, &numUsersInAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.RecognizeFace.OkaoGetNumUsersInAlbumFailed", "");
      return RESULT_FAIL;
    }
    
    if(numUsersInAlbum == 0 && allowAddNewFace) {
      // Nobody in album yet, add this person
      PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.AddingFirstUser",
                       "Adding first user to empty album");
      Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle, timestamp, detectionInfo);
      if(RESULT_OK != lastResult) {
        return lastResult;
      }
      faceID = _lastRegisteredUserID;
    } else {
      INT32 resultNum = 0;
      const s32 MaxResults = 2;
      INT32 userIDs[MaxResults], scores[MaxResults];
      okaoResult = OKAO_FR_Identify(_okaoRecognitionFeatureHandle, _okaoFaceAlbum, MaxResults, userIDs, scores, &resultNum);
      
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_WARNING("FaceTrackerImpl.RecognizeFace.OkaoFaceRecognitionIdentifyFailed",
                            "OKAO Result Code=%d", okaoResult);
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
          
          //const INT32 dataToReplace = GetDataIndex(detectionInfo);
          //auto enrollStatusIter = _enrollmentStatus.find(minID);
          
          //if(IsBetterForRecognition(enrollStatusIter->second.detectionInfo[dataToReplace], detectionInfo))
          //{
            Result result = UpdateExistingUser(minID, _okaoRecognitionFeatureHandle, timestamp, detectionInfo);
            if(RESULT_OK != result) {
              return result;
            }
          //}
          
          for(INT32 iResult = 0; iResult < lastResult; ++iResult)
          {
            if(userIDs[iResult] != minID)
            {
              PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.MergingRecords",
                               "Merging face %d with %d b/c its score %d is > %d",
                               userIDs[iResult], minID, scores[iResult], MergeThreshold);
              
              // TODO: check return val
              RemoveUser(userIDs[iResult]);
              
            }
            ++iResult;
          }
          
          faceID = minID;
          
        } else if(allowAddNewFace) {
          // No match found, add new user
          PRINT_NAMED_INFO("FaceTrackerImpl.RecognizeFace.AddingNewUser",
                           "Observed new person. Adding to album.");
          Result lastResult = RegisterNewUser(_okaoRecognitionFeatureHandle, timestamp, detectionInfo);
          if(RESULT_OK != lastResult) {
            return lastResult;
          }
          faceID = _lastRegisteredUserID;
        }
      }
    }

    return RESULT_OK;
  } // RecognizeFace()
  
  Result FaceTracker::Impl::MergeFaces(Vision::TrackedFace::ID_t keepID,
                                       Vision::TrackedFace::ID_t mergeID)
  {
    INT32 okaoResult = OKAO_NORMAL;
    
    INT32 numKeepData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, (INT32)keepID, &numKeepData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetNumKeepDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    INT32 numMergeData = 0;
    okaoResult = OKAO_FR_GetRegisteredUsrDataNum(_okaoFaceAlbum, (INT32)mergeID, &numMergeData);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetNumMergeDataFail",
                        "OKAO result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    // TODO: Better way of deciding which (not just how many) registered data to keep
    
    while((numMergeData+numKeepData) >= MaxAlbumDataPerFace)
    {
      --numMergeData;
      if((numMergeData+numKeepData) < MaxAlbumDataPerFace) {
        break;
      }
      --numKeepData;
    }
    ASSERT_NAMED((numMergeData+numKeepData) < MaxAlbumDataPerFace,
                 "Total merge and keep data should be less than max data per face");
    
    
    for(s32 iMerge=0; iMerge < numMergeData; ++iMerge) {
      
      okaoResult = OKAO_FR_GetFeatureFromAlbum(_okaoFaceAlbum, (INT32)mergeID, iMerge,
                                               _okaoRecogMergeFeatureHandle);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.GetFeatureFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
      
      okaoResult = OKAO_FR_RegisterData(_okaoFaceAlbum, _okaoRecogMergeFeatureHandle,
                                        (INT32)keepID, numKeepData + iMerge);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.MergeFaces.RegisterDataFail",
                          "OKAO result=%d", okaoResult);
        return RESULT_FAIL;
      }
    }
    
    // After merging it, remove the old user that just got merged
    // TODO: check return val
    RemoveUser((INT32)mergeID);
    
    return RESULT_OK;
    
  } // MergeFaces()
  
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
      bool facePartsFound = false;
      Tic("FacePartDetection");
      Result partDetectResult = DetectFaceParts(nWidth, nHeight, dataPtr, detectionIndex, face);
      if(RESULT_OK != partDetectResult) {
        PRINT_NAMED_WARNING("FaceTrackerImpl.Update.FacePartDetectionFailed",
                            "Detection index %d of %d. Will not attempt expression/face recognition",
                            detectionIndex, numDetections);
      } else {
        facePartsFound = true;
      }
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
      TrackingData& trackingData = _trackingData[trackerID];
      Vision::TrackedFace::ID_t faceID = trackingData.assignedID;
      if(facePartsFound)
      {
        Vision::TrackedFace::ID_t recognizedID = Vision::TrackedFace::UnknownFace;
        //INT32 recognitionScore = 0;
        
        Tic("FaceRecognition");
        Result recognitionResult = RecognizeFace(nWidth, nHeight, dataPtr,
                                                 detectionInfo, frameOrig.GetTimestamp(),
                                                 recognizedID);
        if(RESULT_OK != recognitionResult) {
          // Full-on failure of recognition, just leave faceID whatever it was
          PRINT_NAMED_WARNING("FaceTrackerImpl.Update.FaceRecognitionFailed",
                              "Detection index %d of %d.",
                              detectionIndex, numDetections);
        } else if(Vision::TrackedFace::UnknownFace == recognizedID) {
          // We did not recognize the tracked face in its current position, just leave
          // the faceID alone
          
          // (Nothing to do)
          
        } else if(Vision::TrackedFace::UnknownFace == faceID) {
          // We have not yet assigned a recognition ID to this tracker ID. Use the
          // one we just found via recognition.
          faceID = recognizedID;
          
          
        } else if(faceID != recognizedID) {
          // We recognized this face as a different ID than the one currently
          // assigned to the tracking ID. Trust the tracker that they are in
          // fact the same and merge the two, keeping the original (minimum ID)
          
          Result mergeResult = RESULT_OK;
          if(faceID < recognizedID) {
            PRINT_NAMED_INFO("FaceTrackerImpl.Update.MergingFaces",
                             "Tracking %d: merging recognized ID=%llu into existing ID=%llu",
                             trackerID, recognizedID, faceID);
            mergeResult = MergeFaces(faceID, recognizedID);
          } else {
            PRINT_NAMED_INFO("FaceTrackerImpl.Update.MergingFaces",
                             "Tracking %d: merging existing ID=%llu into recognized ID=%llu",
                             trackerID, faceID, recognizedID);
            mergeResult = MergeFaces(recognizedID, faceID);
            faceID = recognizedID;
          }
          
          if(RESULT_OK != mergeResult) {
            PRINT_NAMED_WARNING("FaceTrackerImpl.Update.MergeFail",
                                "Trying to merge %llu with %llu", faceID, recognizedID);
          }
          
          // TODO: Notify the world somehow that an ID we were tracking got merged?
        } else {
          // We recognized this person as the same ID already assigned to its tracking ID
          faceID = recognizedID;
        }
        
        // We've now assigned an identity to this tracker ID so we don't have to
        // keep trying to recognize it while we're successfully tracking!
        trackingData.assignedID = faceID;
        //trackingData.detectionInfo = detectionInfo;
        
        Toc("FaceRecognition");
      } // if(facePartsFound)
      
      face.SetID(faceID); // could be unknown!
      
    } // FOR each face
    
    return RESULT_OK;
  } // Update()
  
  std::list<TrackedFace> FaceTracker::Impl::GetFaces() const
  {
    return _faces;
  }
  
  
  std::vector<u8> FaceTracker::Impl::GetSerializedAlbum()
  {
    std::vector<u8> serializedAlbum;
    
    UINT32 albumSize = 0;
    INT32 okaoResult = OKAO_FR_GetSerializedAlbumSize(_okaoFaceAlbum, &albumSize);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.GetSerializedAlbum.GetSizeFail",
                        "OKAO Result=%d", okaoResult);
    } else {
      serializedAlbum.resize(albumSize);
      okaoResult = OKAO_FR_SerializeAlbum(_okaoFaceAlbum, &(serializedAlbum[0]), albumSize);
      if(OKAO_NORMAL != okaoResult) {
        PRINT_NAMED_ERROR("FaceTrackerImpl.GetSerializedAlbum.SerializeFail",
                          "OKAO Result=%d", okaoResult);
      }
    }
    
    return serializedAlbum;
  } // GetSerializedAlbum()
  
  
  Result FaceTracker::Impl::SetSerializedAlbum(const std::vector<u8>&serializedAlbum)
  {
    FR_ERROR error;
    HALBUM restoredAlbum = OKAO_FR_RestoreAlbum(_okaoCommonHandle, const_cast<UINT8*>(&(serializedAlbum[0])), (UINT32)serializedAlbum.size(), &error);
    if(NULL == restoredAlbum) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.RestoreFail",
                        "OKAO Result=%d", error);
      return RESULT_FAIL;
    }
    
    INT32 okaoResult = OKAO_FR_DeleteAlbumHandle(_okaoFaceAlbum);
    if(OKAO_NORMAL != okaoResult) {
      PRINT_NAMED_ERROR("FaceTrackerImpl.SetSerializedAlbum.DeleteOldAlbumFail",
                        "OKAO Result=%d", okaoResult);
      return RESULT_FAIL;
    }
    
    _okaoFaceAlbum = restoredAlbum;
    
    return RESULT_OK;
  } // SetSerializedAlbum()
  
  
} // namespace Vision
} // namespace Anki

