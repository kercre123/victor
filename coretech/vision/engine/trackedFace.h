/**
 * File: trackedFace.h
 *
 * Author: Andrew Stein
 * Date:   8/20/2015
 *
 * Description: A container for a tracked face and any features (e.g. eyes, mouth, ...)
 *              related to it.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Vision_TrackedFace_H__
#define __Anki_Vision_TrackedFace_H__

#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/rect.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/radians.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/facialExpressions.h"
#include "clad/types/faceDetectionMetaData.h"

namespace Anki {
namespace Vision {
  
  class TrackedFace
  {
  public:
    
    // Constructor:
    TrackedFace();
    
    TimeStamp_t GetTimeStamp()   const;
    void        SetTimeStamp(TimeStamp_t timestamp);
    
    // Recognition related getters / setters
    void        SetScore(float score);
    void        SetID(FaceID_t newID);
    void        SetNumEnrollments(s32 N);
    
    float       GetScore()          const;
    FaceID_t    GetID()             const;
    s32         GetNumEnrollments() const;
    
    const std::string& GetName() const;
    void SetName(const std::string& newName);
    bool HasName() const;
    
    // Returns true if tracking is happening vs. false if face was just detected
    bool IsBeingTracked() const;
    void SetIsBeingTracked(bool tf);
    
    const Rectangle<f32>& GetRect() const;
    
    // NOTE: Left/right are from viewer's perspective! (i.e. as seen in the image)
    
    enum FeatureName {
      LeftEye = 0,
      RightEye,
      LeftEyebrow,
      RightEyebrow,
      UpperLip,
      LowerLip,
      NoseBridge,
      Nose,
      Contour,
      
      NumFeatures
    };
    
    using Feature = std::vector<Point2f>;
    
    const Feature& GetFeature(FeatureName whichFeature) const;
    void  ClearFature(FeatureName whichFeature);

    // These methods will shift the detected features and rectangles.
    // They are intended for use when we are cropping the image during
    // face detection.
    void HorizontallyShiftFeatures(const s32 horizontalShift);
    void HorizontallyShiftRect(const s32 horizontalShift);
    
    void AddPointToFeature(FeatureName whichFeature, Point2f&& point);
    void SetFeature(FeatureName whichFeature, Feature&& points);
    
    void SetEyeCenters(Point2f&& leftCen, Point2f&& rightCen);
    
    // Returns true if eye centers have been specified via SetEyeCenters, false otherwise
    bool HasEyes() const { return _eyesDetected; }
    
    // Returns true if the face has eyes set and populates the left and right centers
    // Returns false and does not change leftCen/rightCen if eyes were never set
    // GetIntraEyeDistance will undistort the eye locations according to the camera's calibration (if available)
    //  to compute a more accurate distance.
    bool GetEyeCenters(Point2f& leftCen, Point2f& rightCen) const;
    
    void SetRect(Rectangle<f32>&& rect);
    
    // These are w.r.t. the original observer (i.e. the camera at observation time)
    Radians GetHeadYaw()   const;
    Radians GetHeadPitch() const;
    Radians GetHeadRoll()  const;
    
    void SetHeadOrientation(Radians roll, Radians pitch, Radians yaw);
    
    const Pose3d& GetHeadPose() const;
    void SetHeadPose(Pose3d& pose);
    
    // Returns true if face was roughly facing the camera when it was observed
    bool IsFacingCamera() const;
    void SetIsFacingCamera(bool tf);
    
    // Return the histogram over all expressions (sums to 100)
    using FacialExpressionValues = std::array<u8, (size_t)FacialExpression::Count>;
    const FacialExpressionValues& GetExpressionValues() const;
    
    // Return the expression with highest value (and optionally, its score if valuePtr != nullptr)
    // (If the returned expression is Unknown, the returned value will be -1.f) 
    FacialExpression GetMaxExpression(s32* valuePtr = nullptr) const;
    
    // Set a particular expression value
    void SetExpressionValue(FacialExpression whichExpression, f32 newValue);
    
    // Smile information, if available
    const SmileAmount& GetSmileAmount() const { return _smileAmount; }
    void  SetSmileAmount(f32 degree, f32 confidence);
    
    // Gaze direction, if available
    const Gaze& GetGaze() const { return _gaze; }
    void  SetGaze(f32 leftRight_deg, f32 upDown_deg);
    
    // Blink detection, if known.
    const BlinkAmount& GetBlinkAmount() const { return _blinkAmount; }
    void  SetBlinkAmount(f32 leftAmount, f32 rightAmount);
    
    // Eye contact detection, if available
    bool IsMakingEyeContact() const { return _isMakingEyeContact; }
    void SetEyeContact(const bool eyeContact);

    // Has the translation of this face been set
    bool IsTranslationSet() const { return _isTranslationSet; }

    void SetRecognitionDebugInfo(const std::list<FaceRecognitionMatch>& info);
    const std::list<FaceRecognitionMatch>& GetRecognitionDebugInfo() const;
    
  private:
    
    FaceID_t       _id                 = UnknownFaceID;
    float          _score              = 0.f;
    TimeStamp_t    _timestamp          = 0;
    s32            _numEnrollments     = 0;
    bool           _isBeingTracked     = false;
    bool           _isFacingCamera     = false;
    bool           _isMakingEyeContact = false;
    bool           _isTranslationSet   = false;

    std::string    _name;
    
    Rectangle<f32> _rect;
    
    bool    _eyesDetected = false;
    Point2f _leftEyeCen, _rightEyeCen;
    
    std::array<Feature, NumFeatures> _features;
    FacialExpressionValues _expression{};
    
    // "Metadata" about the face
    SmileAmount _smileAmount;
    Gaze        _gaze;
    BlinkAmount _blinkAmount;
    
    Radians _roll, _pitch, _yaw;
    
    Pose3d _headPose;
    
    std::list<FaceRecognitionMatch> _debugRecognitionInfo;
    
  }; // class TrackedFace
  
  //
  // Inlined implementations
  //
  
  inline bool TrackedFace::IsBeingTracked() const  {
    return _isBeingTracked;
  }
  
  inline void TrackedFace::SetIsBeingTracked(bool tf) {
    _isBeingTracked = tf;
  }
  
  inline TimeStamp_t TrackedFace::GetTimeStamp() const {
    return _timestamp;
  }
  
  inline void TrackedFace::SetTimeStamp(TimeStamp_t timestamp) {
    _timestamp = timestamp;
  }
  
  inline const Rectangle<f32>& TrackedFace::GetRect() const {
    return _rect;
  }
  
  inline float TrackedFace::GetScore() const {
    return _score;
  }
  
  inline void TrackedFace::SetScore(float score) {
    _score = score;
  }
  
  inline FaceID_t TrackedFace::GetID() const {
    return _id;
  }
  
  inline void TrackedFace::SetID(FaceID_t newID) {
    _id = newID;
  }
  
  inline s32 TrackedFace::GetNumEnrollments() const {
    return _numEnrollments;
  }
  
  inline void TrackedFace::SetNumEnrollments(s32 N) {
    _numEnrollments = N;
  }
  
  inline void TrackedFace::ClearFature(FeatureName whichFeature) {
    _features[whichFeature].clear();
  }
  
  inline void TrackedFace::SetFeature(FeatureName whichFeature, Feature&& points) {
    _features[whichFeature] = points;
  }
  
  inline const TrackedFace::Feature& TrackedFace::GetFeature(TrackedFace::FeatureName whichFeature) const {
    return _features[whichFeature];
  }
  
  inline void TrackedFace::AddPointToFeature(FeatureName whichFeature, Point2f &&point)
  {
    _features[whichFeature].emplace_back(point);
  }
  
  inline void TrackedFace::SetEyeCenters(Point2f&& leftCen, Point2f&& rightCen)
  {
    _leftEyeCen = leftCen;
    _rightEyeCen = rightCen;
    _eyesDetected = true;
  }
  
  inline bool TrackedFace::GetEyeCenters(Point2f& leftCen, Point2f& rightCen) const
  {
    if(HasEyes()) {
      leftCen = _leftEyeCen;
      rightCen = _rightEyeCen;
    }
    return HasEyes();
  }
  
  inline void TrackedFace::SetRect(Rectangle<f32> &&rect)
  {
    _rect = rect;
  }
  
  inline Radians TrackedFace::GetHeadYaw()   const {
    return _yaw;
  }
  
  inline Radians TrackedFace::GetHeadPitch() const {
    return _pitch;
  }
  
  inline Radians TrackedFace::GetHeadRoll()  const {
    return _roll;
  }

  inline void TrackedFace::SetHeadOrientation(Radians roll, Radians pitch, Radians yaw) {
    _roll = roll;
    _pitch = pitch;
    _yaw = yaw;
    //PRINT_NAMED_INFO("TrackedFace.SetHeadOrientation", "Roll=%.1fdeg, Pitch=%.1fdeg, Yaw=%.1fdeg",
    //                 _roll.getDegrees(), _pitch.getDegrees(), _yaw.getDegrees());
  }
  
  inline const Pose3d& TrackedFace::GetHeadPose() const {
    return _headPose;
  }
  
  inline void TrackedFace::SetHeadPose(Pose3d &pose) {
    _headPose = pose;
    _isTranslationSet = true;
  }
  
  
  inline const std::string& TrackedFace::GetName() const {
    return _name;
  }

  inline bool TrackedFace::HasName() const {
    return !_name.empty();
  }
  
  inline bool TrackedFace::IsFacingCamera() const {
    return _isFacingCamera;
  }
  
  inline void TrackedFace::SetIsFacingCamera(bool tf) {
    _isFacingCamera = tf;
  }
  
  inline void TrackedFace::SetName(const std::string& newName) {
    _name = newName;
  }
  
  inline void TrackedFace::SetRecognitionDebugInfo(const std::list<FaceRecognitionMatch>& info) {
    _debugRecognitionInfo = info;
  }
  
  inline const std::list<FaceRecognitionMatch>& TrackedFace::GetRecognitionDebugInfo() const {
    return _debugRecognitionInfo;
  }
  
  inline void TrackedFace::SetSmileAmount(f32 amount, f32 confidence) {
    _smileAmount.wasChecked = true;
    _smileAmount.amount     = amount;
    _smileAmount.confidence = confidence;
  }
  
  inline void TrackedFace::SetGaze(f32 leftRight_deg, f32 upDown_deg) {
    _gaze.wasChecked = true;
    _gaze.leftRight_deg = leftRight_deg;
    _gaze.upDown_deg = upDown_deg;
  }
  
  inline void TrackedFace::SetBlinkAmount(f32 leftAmount, f32 rightAmount) {
    _blinkAmount.wasChecked = true;
    _blinkAmount.blinkAmountLeft  = leftAmount;
    _blinkAmount.blinkAmountRight = rightAmount;
  }

  inline void TrackedFace::SetEyeContact(const bool eyeContact) {
    _isMakingEyeContact = eyeContact;
  }
  
} // namespace Vision
} // namespace Anki

#endif
