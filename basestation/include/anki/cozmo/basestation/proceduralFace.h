#ifndef __Anki_Cozmo_ProceduralFace_H__
#define __Anki_Cozmo_ProceduralFace_H__

#include "anki/common/types.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "clad/types/proceduralEyeParameters.h"

#include <opencv2/core/core.hpp>

#include <array>

namespace Anki {
  
  // Forward declaration:
  namespace Vision {
    class TrackedFace;
  }
  
namespace Cozmo {

  class ProceduralFace
  {
  public:
    using Value = f32;
    
    static const int WIDTH  = FaceAnimationManager::IMAGE_WIDTH;
    static const int HEIGHT = FaceAnimationManager::IMAGE_HEIGHT;
 
    // Nominal positions/sizes for everything (these are things that aren't
    // parameterized at dynamically, but could be if we want)
    static constexpr Value NominalLeftEyeCenX    = 0.25f*static_cast<f32>(WIDTH);
    static constexpr Value NominalRightEyeCenX   = 0.75f*static_cast<f32>(WIDTH);
    static constexpr Value NominalEyeCenY        = 0.667f*static_cast<f32>(HEIGHT);
    static constexpr Value NominalEyeHeight      = 0.5f*static_cast<f32>(HEIGHT);
    static constexpr Value NominalEyebrowHeight  = 0.6f*(NominalEyeCenY - 0.5f*NominalEyeHeight);
    static constexpr Value NominalEyeWidth       = 0.333f*static_cast<f32>(WIDTH);
    static constexpr Value EyebrowHalfLength     = 0.5f*static_cast<f32>(NominalEyeWidth);
    static constexpr Value NominalPupilWidthFrac = 0.33f;
    static constexpr Value NominalPupilHeightFrac= 0.5f;
    static constexpr Value EyebrowThickness      = 2.f;
    
    static const     Value MaxFaceAngle;  // Not sure why I can't set this one as constexpr here??
    
    static const bool ScanlinesAsPostProcess = true;
    
    using Parameter = ProceduralEyeParameter;
    
    enum WhichEye {
      Left,
      Right
    };
    
    ProceduralFace();
    
    // Reset parameters to their nominal values
    void Reset();
    
    // Get/Set each of the above procedural parameters, for each eye
    void  SetParameter(WhichEye whichEye, Parameter param, Value value);
    Value GetParameter(WhichEye whichEye, Parameter param) const;
    
    // Get/Set the overall angle of the whole face
    void SetFaceAngle(Value angle_deg);
    Value GetFaceAngle() const;
    
    // Set this face's parameters to values interpolated from two other faces.
    // When BlendFraction == 0.0, the parameters will be equal to face1's.
    // When BlendFraction == 1.0, the parameters will be equal to face2's.
    // TODO: Support other types of interpolation besides simple linear
    // Note: 0.0 <= BlendFraction <= 1.0!
    void Interpolate(const ProceduralFace& face1,
                     const ProceduralFace& face2,
                     float fraction);
    
    void Blink(const ProceduralFace& face, float fraction);
    
    // Actually draw the face with the current parameters
    cv::Mat_<u8> GetFace() const;
    
    void MimicHumanFace(const Vision::TrackedFace& trackedFace);

    bool HasBeenSentToRobot() const;
    void MarkAsSentToRobot(bool tf);
    
    TimeStamp_t GetTimeStamp() const;
    void SetTimeStamp(TimeStamp_t t);
    
  private:
    
    using ValueLimits = std::pair<Value,Value>;
    static const ValueLimits& GetLimits(Parameter param);
    
    void DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const;
    void DrawEyeBrow(WhichEye whichEye, cv::Mat_<u8>& faceImg) const;
    
    // Container for the parameters for both eyes
    std::array<std::array<Value, static_cast<size_t>(Parameter::NumParameters)>, 2> _eyeParams;
    
    Value _faceAngle_deg;
    
    bool _sentToRobot;
    TimeStamp_t _timestamp;
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFace::SetParameter(WhichEye whichEye, Parameter param, Value value)
  {
    const ValueLimits& lims = GetLimits(param);
    _eyeParams[whichEye][static_cast<size_t>(param)] = std::max(lims.first, std::min(lims.second, value));
  }
  
  inline ProceduralFace::Value ProceduralFace::GetParameter(WhichEye whichEye, Parameter param) const
  {
    return _eyeParams[whichEye][static_cast<size_t>(param)];
  }
  
  inline ProceduralFace::Value ProceduralFace::GetFaceAngle() const {
    return _faceAngle_deg;
  }
  
  inline bool ProceduralFace::HasBeenSentToRobot() const {
    return _sentToRobot;
  }
  
  inline void ProceduralFace::MarkAsSentToRobot(bool tf) {
    _sentToRobot = tf;
  }
  
  inline TimeStamp_t ProceduralFace::GetTimeStamp() const {
    return _timestamp;
  }
  
  inline void ProceduralFace::SetTimeStamp(TimeStamp_t t) {
    _timestamp = t;
  }
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFace_H__
