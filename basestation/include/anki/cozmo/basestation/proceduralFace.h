#ifndef __Anki_Cozmo_ProceduralFace_H__
#define __Anki_Cozmo_ProceduralFace_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
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
    static constexpr s32   NominalEyeHeight      = 40;
    static constexpr s32   NominalEyeWidth       = 30;
    
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
    
    // Get/Set the overall face position
    void SetFacePosition(Point<2,Value> center);
    Point<2,Value> GetFacePosition() const;
    
    // Get/Set the overall face scale
    void SetFaceScale(Value scale);
    Value GetFaceScale() const;
    
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
    
    // Container for the parameters for both eyes
    std::array<std::array<Value, static_cast<size_t>(Parameter::NumParameters)>, 2> _eyeParams;
    
    Value           _faceAngle;
    Value           _faceScale;
    Point<2,Value>  _faceCenter;
    
    static u8 _firstScanLine;
    bool _sentToRobot;
    TimeStamp_t _timestamp;
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFace::SetParameter(WhichEye whichEye, Parameter param, Value value)
  {
    _eyeParams[whichEye][static_cast<size_t>(param)] = value; //std::max(-1.f, std::min(1.f, value));
  }
  
  inline ProceduralFace::Value ProceduralFace::GetParameter(WhichEye whichEye, Parameter param) const
  {
    return _eyeParams[whichEye][static_cast<size_t>(param)];
  }
  
  inline ProceduralFace::Value ProceduralFace::GetFaceAngle() const {
    return _faceAngle;
  }
  
  inline void ProceduralFace::SetFaceAngle(Value angle) {
    _faceAngle = angle; //std::max(-1.f, std::min(1.f, angle));
  }
  
  inline void ProceduralFace::SetFacePosition(Point<2, Value> center) {
    _faceCenter = center;
    //    _faceCenter.x() = std::max(-1.f, std::min(1.f, center.x()));
    //    _faceCenter.y() = std::max(-1.f, std::min(1.f, center.y()));
  }
  
  inline Point<2,ProceduralFace::Value> ProceduralFace::GetFacePosition() const {
    return _faceCenter;
  }
  
  inline void ProceduralFace::SetFaceScale(Value scale) {
    _faceScale = scale;
  }
  
  inline ProceduralFace::Value ProceduralFace::GetFaceScale() const {
    return _faceScale;
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
