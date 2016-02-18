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

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/rect.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/vision/basestation/image.h"

namespace Anki {
namespace Vision {
  
  // Forward declaration
  class Camera;
  
  class TrackedFace
  {
  public:
    
    using ID_t = int32_t;
    
    static const ID_t UnknownFace = -1;
    
    // Constructor:
    TrackedFace();
    
    float       GetScore()       const;
    ID_t        GetID()          const;
    TimeStamp_t GetTimeStamp()   const;
    
    void SetScore(float score);
    void SetID(ID_t newID);
    void SetTimeStamp(TimeStamp_t timestamp);
    
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
    
    // TODO: Cladify this enum
    // (There's no clad in coretech-internal/vision yet, so for now, just do it here)
    enum Expression {
      Neutral = 0,
      Happiness,
      Surprise,
      Anger,
      Sadness,
      
      NumExpressions
    };
    
    using Feature = std::vector<Point2f>;
    
    const Feature& GetFeature(FeatureName whichFeature) const;
    void  ClearFature(FeatureName whichFeature);
    
    void AddPointToFeature(FeatureName whichFeature, Point2f&& point);
    void SetFeature(FeatureName whichFeature, Feature&& points);
    
    void SetEyeCenters(Point2f&& leftCen, Point2f&& rightCen);
    
    // Returns true if eye centers have been specified via SetEyeCenters, false otherwise
    bool HasEyes() const { return _eyesDetected; }
    
    // Returns true if the face has eyes set and populates the left and right centers
    // Returns false and does not change leftCen/rightCen if eyes were never set
    bool GetEyeCenters(Point2f& leftCen, Point2f& rightCen) const;
    f32  GetIntraEyeDistance() const;
    
    void SetRect(Rectangle<f32>&& rect);
    
    void SetThumbnail(const Image& image) { _thumbnailImage = image; }
    const Image& GetThumbnail() { return _thumbnailImage; }
    
    // These are w.r.t. the original observer (i.e. the camera at observation time)
    Radians GetHeadYaw()   const;
    Radians GetHeadPitch() const;
    Radians GetHeadRoll()  const;
    
    void SetHeadOrientation(Radians roll, Radians pitch, Radians yaw);
    
    const Pose3d& GetHeadPose() const;
    void SetHeadPose(Pose3d& pose);
    
    // Returns true if real eye detections were used to update the translation, false
    // if fake eye centers (based on detection rectangle) were used.
    bool UpdateTranslation(const Vision::Camera& camera);
    
    // Return the histogram over all expressions
    std::array<f32, NumExpressions> GetExpressionValues() const;
    
    // Return the expression with highest value
    Expression GetMaxExpression() const;
    
    // Set a particular expression value
    void SetExpressionValue(Expression whichExpression, f32 newValue);
    
    static const char* GetExpressionName(Expression whichExpression);
    
  private:
    
    ID_t           _id;
    
    float          _score;
    bool           _isBeingTracked;
    TimeStamp_t    _timestamp;
    
    Rectangle<f32> _rect;
    
    bool    _eyesDetected = false;
    Point2f _leftEyeCen, _rightEyeCen;
    
    std::array<Feature, NumFeatures> _features;
    std::array<f32, NumExpressions> _expression{};
    
    Radians _roll, _pitch, _yaw;
    
    Pose3d _headPose;
    
    Image _thumbnailImage;
    
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
  
  inline TrackedFace::ID_t TrackedFace::GetID() const {
    return _id;
  }
  
  inline void TrackedFace::SetID(ID_t newID) {
    _id = newID;
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
    return _eyesDetected;
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
  }
  
  
} // namespace Vision
} // namespace Anki

#endif