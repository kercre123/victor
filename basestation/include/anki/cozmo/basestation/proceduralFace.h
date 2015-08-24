#ifndef __Anki_Cozmo_ProceduralFace_H__
#define __Anki_Cozmo_ProceduralFace_H__

#include <opencv2/core/core.hpp>

namespace Anki {
namespace Cozmo {

  class ProceduralFace
  {
  public:
    using Value = int;
    
    static const int WIDTH = 128;
    static const int HEIGHT = 64;
    
    enum Parameter {
      BrowAngle = 0, // Degrees
      BrowShiftX, // From nominal position
      BrowShiftY, //    "
      EyeHeight,
      PupilHeight,
      PupilShiftX, // From nominal position
      PupilShiftY, //   "
      
      NumParameters
    };
    
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
    
    cv::Mat_<u8> GetFace() const;
    
    
  private:
    
    // Nominal positions/sizes for everything (these are things that aren't
    // parameterized at dynamically, but could be if we want)
    static const Value NominalLeftEyeCenX  = WIDTH/4;
    static const Value NominalRightEyeCenX = 3*WIDTH/4;
    static const Value NominalEyebrowHeight = HEIGHT/4;
    static const Value NominalEyeCenY = HEIGHT/2;
    static const Value NominalEyeHeight = HEIGHT/2;
    static const Value EyeWidth  = WIDTH/4;
    static const Value EyebrowHalfLength = EyeWidth/2;
    static const Value PupilWidth = EyeWidth/3;
    static const Value NominalPupilHeight = NominalEyeHeight/2;
    
    // Container for the parameters for both eyes
    std::array<std::array<Value, NumParameters>, 2> _eyeParams;
    
    Value _faceAngle_deg;
    
  }; // class ProceduralFace
  
  
#pragma mark Inlined Methods
  
  inline void ProceduralFace::SetParameter(WhichEye whichEye, Parameter param, Value value)
  {
    _eyeParams[whichEye][param] = value;
  }
  
  inline ProceduralFace::Value ProceduralFace::GetParameter(WhichEye whichEye, Parameter param) const
  {
    return _eyeParams[whichEye][param];
  }
  
  inline void ProceduralFace::SetFaceAngle(Value angle_deg) {
    _faceAngle_deg = angle_deg;
  }
  
  inline Value ProceduralFace::GetFaceAngle() const {
    return _faceAngle_deg;
  }
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFace_H__
