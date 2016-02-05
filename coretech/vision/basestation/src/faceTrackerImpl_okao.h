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
#include "anki/vision/basestation/profiler.h"

#include "faceRecognizer_okao.h"

// Omron OKAO Vision
#include <OkaoAPI.h>
#include <OkaoDtAPI.h> // Face Detection
#include <OkaoPtAPI.h> // Face parts detection
#include <OkaoExAPI.h> // Expression recognition
#include <CommonDef.h>
#include <DetectorComDef.h>

#include <list>
#include <ctime>

namespace Anki {
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
    
    void EnableNewFaceEnrollment(bool enable) { _recognizer.EnableNewFaceEnrollment(enable); }
    bool IsNewFaceEnrollmentEnabled() const   { return _recognizer.IsNewFaceEnrollmentEnabled(); }
    
    void EnableEmotionDetection(bool enable) { _detectEmotion = enable; }
    bool IsEmotionDetectionEnabled() const   { return _detectEmotion;  }

    void AssignNameToID(TrackedFace::ID_t faceID, const std::string& name);
    
    Result LoadAlbum(const std::string& albumName);
    Result SaveAlbum(const std::string& albumName);

  private:
    
    Result Init();

    bool   DetectFaceParts(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                           INT32 detectionIndex, Vision::TrackedFace& face);
    
    Result EstimateExpression(INT32 nWidth, INT32 nHeight, RAWIMAGE* dataPtr,
                              Vision::TrackedFace& face);
  
    bool _isInitialized = false;
    FaceTracker::DetectionMode _detectionMode;
    bool _detectEmotion = true;
    
    static const s32   MaxFaces = 10; // detectable at once
    
    std::list<TrackedFace> _faces;
    
    // Mapping from tracking ID to recognition (identity) ID
    struct TrackingData {
      Vision::TrackedFace::ID_t assignedID = Vision::TrackedFace::UnknownFace;
    };
    std::map<INT32, TrackingData> _trackingData;
    
    //u8* _workingMemory = nullptr;
    //u8* _backupMemory  = nullptr;
    
    // Okao Vision Library "Handles"
    // Note that we have two actual handles for part detection so that we can
    // hand one to the FaceRecognizer and let it churn on that while we use the
    // other one for part detection in the mean time.
    HCOMMON     _okaoCommonHandle               = NULL;
    HDETECTION  _okaoDetectorHandle             = NULL;
    HDTRESULT   _okaoDetectionResultHandle      = NULL;
    HPOINTER    _okaoPartDetectorHandle         = NULL;
    HPTRESULT   _okaoPartDetectionResultHandle  = NULL;
    HPTRESULT   _okaoPartDetectionResultHandle2 = NULL;
    HEXPRESSION _okaoEstimateExpressionHandle   = NULL;
    HEXPRESSION _okaoExpressionResultHandle     = NULL;
    
    // Space for detected face parts / expressions
    POINT _facialParts[PT_POINT_KIND_MAX];
    INT32 _facialPartConfs[PT_POINT_KIND_MAX];
    INT32 _expressionValues[EX_EXPRESSION_KIND_MAX];
    
    // Runs on a separate thread
    FaceRecognizer _recognizer;
    
  }; // class FaceTracker::Impl
  
} // namespace Vision
} // namespace Anki

