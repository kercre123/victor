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

#include "coretech/common/shared/math/point_fwd.h"
#include "coretech/common/shared/math/rect.h"
#include "coretech/common/shared/math/rect_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/math/radians.h"

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
    
    // If this face has a name, it will be returned. Otherwise, the best guess
    // will be returned (which could still be none/empty). The best guess need
    // not be as confident a match as the "regular" name above, but may be useful
    // in some situations when we're willing to be less sure about someone's identity.
    const std::string& GetBestGuessName() const;
    void SetBestGuessName(const std::string& name);
    
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
    using FeatureConfidence = std::vector<int>;
    
    const Feature& GetFeature(FeatureName whichFeature) const;
    const FeatureConfidence& GetFeatureConfidence(FeatureName whichFeature) const;
    void  ClearFature(FeatureName whichFeature);

    // Shift both the detection rectangle and features
    void Shift(const Point2f& shift);
    
    void AddPointToFeature(FeatureName whichFeature, Point2f&& point);
    void SetFeature(FeatureName whichFeature, Feature&& points, FeatureConfidence&& confidences);
    
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

    // This is derived from gaze similar to how head
    // pose is derived from head yaw, pitch and roll.
    // The eye pose is independent of the head pose.
    const Pose3d& GetEyePose() const;
    void SetEyePose(Pose3d& pose);

    // This is the gaze direction computed using the head pose rotation matrix, and then
    // projected onto the ground plane
    const Pose3d& GetGazeDirectionPose() const;
    void SetGazeDirectionPose(const Pose3d& gazeDirectionPose);

    // Returns true if face was roughly facing the camera when it was observed
    bool IsFacingCamera() const;
    void SetIsFacingCamera(bool tf);
    
    // Return the histogram over all expressions (sums to 100)
    using ExpressionValue = u8;
    using ExpressionValues = std::array<ExpressionValue, (size_t)FacialExpression::Count>;
    const ExpressionValues& GetExpressionValues() const;
    
    // Return the expression with highest value (and optionally, its score if valuePtr != nullptr)
    // (If the returned expression is Unknown, the returned value will be -1) 
    FacialExpression GetMaxExpression(ExpressionValue* valuePtr = nullptr) const;
    
    // Set a particular expression value
    void SetExpressionValue(FacialExpression whichExpression, ExpressionValue newValue);
    
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

    // Is gaze direction stable
    bool IsGazeDirectionStable() const { return _isGazeDirectionStable; }
    void SetGazeDirectionStable(const bool gazeDirecitonStable);

    // Has the translation of this face been set
    bool IsTranslationSet() const { return _isTranslationSet; }

    void SetRecognitionDebugInfo(const std::list<FaceRecognitionMatch>& info);
    const std::list<FaceRecognitionMatch>& GetRecognitionDebugInfo() const;
    
  private:
    
    FaceID_t       _id                    = UnknownFaceID;
    float          _score                 = 0.f;
    TimeStamp_t    _timestamp             = 0;
    s32            _numEnrollments        = 0;
    bool           _isBeingTracked        = false;
    bool           _isFacingCamera        = false;
    bool           _isMakingEyeContact    = false;
    bool           _isGazeDirectionStable = false;
    bool           _isTranslationSet   = false;
    Point2f        _eyeGazeAverage;

    Pose3d _gazeDirectionPose;

    std::string    _name;
    std::string    _bestGuessName;
    
    Rectangle<f32> _rect;
    
    bool    _eyesDetected = false;
    Point2f _leftEyeCen, _rightEyeCen;
    
    std::array<Feature, NumFeatures> _features;
    std::array<FeatureConfidence, NumFeatures> _featureConfidences;
    ExpressionValues _expression{};
    
    // "Metadata" about the face
    SmileAmount _smileAmount;
    Gaze        _gaze;
    BlinkAmount _blinkAmount;
    
    Radians _roll, _pitch, _yaw;
    
    Pose3d _headPose;
    Pose3d _eyePose;
    
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
  
  inline void TrackedFace::SetFeature(FeatureName whichFeature, Feature&& points, FeatureConfidence&& confs) {
    DEV_ASSERT(points.size() == confs.size(), "TrackedFace.SetFeature.MisMatchedPointsAndConfidences");
    _features[whichFeature] = points;
    _featureConfidences[whichFeature] = confs;
  }
  
  inline const TrackedFace::Feature& TrackedFace::GetFeature(TrackedFace::FeatureName whichFeature) const {
    return _features[whichFeature];
  }
  
  inline const TrackedFace::FeatureConfidence& TrackedFace::GetFeatureConfidence(FeatureName whichFeature) const {
    return _featureConfidences[whichFeature];
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

  inline const Pose3d& TrackedFace::GetEyePose() const {
    return _eyePose;
  }

  inline void TrackedFace::SetEyePose(Pose3d &pose) {
    _eyePose = pose;
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
  
  inline void TrackedFace::SetBestGuessName(const std::string& name) {
    _bestGuessName = name;
  }
  
  inline const std::string& TrackedFace::GetBestGuessName() const {
    return (HasName() ? GetName() : _bestGuessName);
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

  inline void TrackedFace::SetGazeDirectionStable(const bool isGazeDirectionStable) {
    _isGazeDirectionStable = isGazeDirectionStable;
  }

  inline const Pose3d& TrackedFace::GetGazeDirectionPose() const {
    return _gazeDirectionPose;
  }

  inline void TrackedFace::SetGazeDirectionPose(const Pose3d& gazeDirectionPose) {
    _gazeDirectionPose = gazeDirectionPose;
  }

} // namespace Vision
} // namespace Anki

#endif
