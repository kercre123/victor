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
    static constexpr Value NominalEyeCenY        = 0.6f*static_cast<f32>(HEIGHT);
    static constexpr Value EyebrowThickness      = 2.f;
    static constexpr s32   MinEyeHeightPix       = 0;
    static constexpr s32   MaxEyeHeightPix       = 3*HEIGHT/4;
    static constexpr s32   MinEyeWidthPix        = WIDTH/5;
    static constexpr s32   MaxEyeWidthPix        = 2*WIDTH/5;
    static constexpr s32   EyebrowHalfLength     = (MaxEyeWidthPix + MinEyeWidthPix)/4; // Half the average eye width
    static constexpr s32   MaxBrowAngle          = 15; // Degrees (symmtric, also used for -ve angle)
    static constexpr s32   MaxFaceAngle          = 25; //   "

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
    
    // Get/Set the overall angle of the whole face (still using parameter on interval [-1,1]
    void SetFaceAngle(Value value);
    Value GetFaceAngle() const;
    
    // Set this face's parameters to values interpolated from two other faces.
    //   When BlendFraction == 0.0, the parameters will be equal to face1's.
    //   When BlendFraction == 1.0, the parameters will be equal to face2's.
    //   TODO: Support other types of interpolation besides simple linear
    //   Note: 0.0 <= BlendFraction <= 1.0!
    // If usePupilSaccades==true, pupil positions don't interpolate smoothly but
    //   instead jump when fraction crossed 0.5.
    void Interpolate(const ProceduralFace& face1,
                     const ProceduralFace& face2,
                     float fraction,
                     bool usePupilSaccades = false);
    
    // Closes eyes and switches interlacing
    void Blink();
    
    // Actually draw the face with the current parameters
    cv::Mat_<u8> GetFace() const;
    
    void MimicHumanFace(const Vision::TrackedFace& trackedFace);

    bool HasBeenSentToRobot() const;
    void MarkAsSentToRobot(bool tf);
    
    TimeStamp_t GetTimeStamp() const;
    void SetTimeStamp(TimeStamp_t t);
    
    // To avoid burn-in this switches which scanlines to use (odd or even), e.g.
    // to be called each time we blink.
    static void SwitchInterlacing();
    
  private:

    static const cv::Rect imgRect;
    
    void DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const;
    s32 GetBrowHeight(const s32 eyeHeightPix, const Value browCenY) const;
    
    // Container for the parameters for both eyes
    std::array<std::array<Value, static_cast<size_t>(Parameter::NumParameters)>, 2> _eyeParams;
    
    Value _faceAngle;
    
    static u8 _firstScanLine;
    bool _sentToRobot;
    TimeStamp_t _timestamp;
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFace::SetParameter(WhichEye whichEye, Parameter param, Value value)
  {
    _eyeParams[whichEye][static_cast<size_t>(param)] = std::max(-1.f, std::min(1.f, value));
  }
  
  inline ProceduralFace::Value ProceduralFace::GetParameter(WhichEye whichEye, Parameter param) const
  {
    return _eyeParams[whichEye][static_cast<size_t>(param)];
  }
  
  inline ProceduralFace::Value ProceduralFace::GetFaceAngle() const {
    return _faceAngle;
  }
  
  inline void ProceduralFace::SetFaceAngle(Value angle) {
    _faceAngle = std::max(-1.f, std::min(1.f, angle));
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
  
  inline void ProceduralFace::SwitchInterlacing() {
    _firstScanLine = 1 - _firstScanLine;
  }
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFace_H__
