/**
 * File: proceduralFaceParams.h
 *
 * Author: Lee Crippen
 * Created: 11/17/15
 *
 * Description: Holds and sets the face rig data used by ProceduralFace.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_ProceduralFaceParams_H__
#define __Anki_Cozmo_ProceduralFaceParams_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "clad/types/proceduralEyeParameters.h"
#include <array>
#include <vector>

namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {
  
// Forward declarations
namespace ExternalInterface {
  struct DisplayProceduralFace;
}
  
class ProceduralFaceParams
{
public:
  using Value = f32;
  using Parameter = ProceduralEyeParameter;
  
  enum WhichEye {
    Left,
    Right
  };
  
  // Allows setting an instance of ProceduralFaceParams to be used as reset values
  static void SetResetData(const ProceduralFaceParams& newResetData);
  
  // Reset parameters to their nominal values
  void Reset();
  
  // Read in available parameters from Json
  void SetFromJson(const Json::Value &jsonRoot);
  
  void SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg);
  
  // Get/Set each of the above procedural parameters, for each eye
  void  SetParameter(WhichEye whichEye, Parameter param, Value value);
  Value GetParameter(WhichEye whichEye, Parameter param) const;
  
  // Get/Set the overall angle of the whole face (still using parameter on interval [-1,1]
  void SetFaceAngle(Value value);
  Value GetFaceAngle() const;
  
  // Get/Set the overall face position
  void SetFacePosition(Point<2,Value> center);
  Point<2,Value> const& GetFacePosition() const;
  
  // Get/Set the overall face scale
  void SetFaceScale(Point<2,Value> scale);
  Point<2,Value> const& GetFaceScale() const;
  
  // Set this face's parameters to values interpolated from two other faces.
  //   When BlendFraction == 0.0, the parameters will be equal to face1's.
  //   When BlendFraction == 1.0, the parameters will be equal to face2's.
  //   TODO: Support other types of interpolation besides simple linear
  //   Note: 0.0 <= BlendFraction <= 1.0!
  // If usePupilSaccades==true, pupil positions don't interpolate smoothly but
  //   instead jump when fraction crossed 0.5.
  void Interpolate(const ProceduralFaceParams& face1,
                   const ProceduralFaceParams& face2,
                   float fraction,
                   bool usePupilSaccades = false);
  
private:
  // Container for the parameters for both eyes
  std::array<std::array<Value, static_cast<size_t>(Parameter::NumParameters)>, 2> _eyeParams{{}};
  
  Value           _faceAngle = 0.0f;
  Point<2,Value>  _faceScale;
  Point<2,Value>  _faceCenter;
  
  void SetEyeArrayHelper(WhichEye eye, const std::vector<Value>& eyeArray);
  
  static ProceduralFaceParams* _resetData;
  
}; // class ProceduralFaceParams
  
#pragma mark Inlined Methods
  
inline void ProceduralFaceParams::SetParameter(WhichEye whichEye, Parameter param, Value value)
{
  _eyeParams[whichEye][static_cast<size_t>(param)] = value;
}

inline ProceduralFaceParams::Value ProceduralFaceParams::GetParameter(WhichEye whichEye, Parameter param) const
{
  return _eyeParams[whichEye][static_cast<size_t>(param)];
}

inline ProceduralFaceParams::Value ProceduralFaceParams::GetFaceAngle() const {
  return _faceAngle;
}

inline void ProceduralFaceParams::SetFaceAngle(Value angle) {
  _faceAngle = angle; //std::max(-1.f, std::min(1.f, angle));
}

inline void ProceduralFaceParams::SetFacePosition(Point<2, Value> center) {
  _faceCenter = center;
}

inline Point<2,ProceduralFaceParams::Value> const& ProceduralFaceParams::GetFacePosition() const {
  return _faceCenter;
}

inline void ProceduralFaceParams::SetFaceScale(Point<2,Value> scale) {
  _faceScale = scale;
}

inline Point<2,ProceduralFaceParams::Value> const& ProceduralFaceParams::GetFaceScale() const {
  return _faceScale;
}

  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFaceParams_H__