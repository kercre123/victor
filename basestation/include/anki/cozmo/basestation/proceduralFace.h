/**
 * File: ProceduralFace.h
 *
 * Author: Lee Crippen
 * Created: 11/17/15
 *
 * Description: Holds and sets the face rig data used by ProceduralFace.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_ProceduralFace_H__
#define __Anki_Cozmo_ProceduralFace_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "clad/types/proceduralEyeParameters.h"
#include "util/logging/logging.h"
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
  
class ProceduralFace
{
public:
  static const int WIDTH  = FaceAnimationManager::IMAGE_WIDTH;
  static const int HEIGHT = FaceAnimationManager::IMAGE_HEIGHT;
  
  using Value = f32;
  using Parameter = ProceduralEyeParameter;
  
  // Container for the parameters for both eyes
  using EyeParamArray = std::array<Value, static_cast<size_t>(Parameter::NumParameters)>;
  
  // Note: SCREEN Left and Right, not Cozmo's left and right!!!!
  enum WhichEye {
    Left,
    Right
  };
  
  ProceduralFace();
  
  // Allows setting an instance of ProceduralFace to be used as reset values
  static void SetResetData(const ProceduralFace& newResetData);
  
  // Reset parameters to their nominal values
  void Reset();
  
  // Read in available parameters from Json
  void SetFromJson(const Json::Value &jsonRoot);
  
  void SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg);
  
  // Get/Set each of the above procedural parameters, for each eye
  void  SetParameter(WhichEye whichEye, Parameter param, Value value);
  Value GetParameter(WhichEye whichEye, Parameter param) const;
  const EyeParamArray& GetParameters(WhichEye whichEye) const;
  
  // Set the same value to a parameter for both eyes:
  void SetParameterBothEyes(Parameter param, Value value);
  
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
  void Interpolate(const ProceduralFace& face1,
                   const ProceduralFace& face2,
                   float fraction,
                   bool usePupilSaccades = false);
  
  // Combine the input params with those from our instance
  ProceduralFace& Combine(const ProceduralFace& otherFace);
  
  // E.g. for unit tests
  static void EnableClippingWarning(bool enable);
  
private:
  
  std::array<EyeParamArray, 2> _eyeParams{{}};
  
  Value           _faceAngle = 0.0f;
  Point<2,Value>  _faceScale = 1.0f;
  Point<2,Value>  _faceCenter = 0.0f;
  
  void SetEyeArrayHelper(WhichEye eye, const std::vector<Value>& eyeArray);
  void CombineEyeParams(EyeParamArray& eyeArray0, const EyeParamArray& eyeArray1);
  
  Value Clip(WhichEye eye, Parameter whichParam, Value value) const;
                                                          
  static ProceduralFace* _resetData;
  static std::function<void(const char*,Value,Value,Value)> ClipWarnFcn;
  
}; // class ProceduralFace
  
#pragma mark Inlined Methods
  
inline void ProceduralFace::SetParameter(WhichEye whichEye, Parameter param, Value value)
{
  _eyeParams[whichEye][static_cast<size_t>(param)] = Clip(whichEye, param, value);
}

inline ProceduralFace::Value ProceduralFace::GetParameter(WhichEye whichEye, Parameter param) const
{
  return _eyeParams[whichEye][static_cast<size_t>(param)];
}
  
inline const ProceduralFace::EyeParamArray& ProceduralFace::GetParameters(WhichEye whichEye) const
{
  return _eyeParams[whichEye];
}

inline void ProceduralFace::SetParameterBothEyes(Parameter param, Value value)
{
  SetParameter(WhichEye::Left,  param, value);
  SetParameter(WhichEye::Right, param, value);
}
  
inline ProceduralFace::Value ProceduralFace::GetFaceAngle() const {
  return _faceAngle;
}

inline void ProceduralFace::SetFaceAngle(Value angle) {
  // TODO: Define face angle limits?
  _faceAngle = angle;
}

inline void ProceduralFace::SetFacePosition(Point<2, Value> center) {
  _faceCenter = center;
}

inline Point<2,ProceduralFace::Value> const& ProceduralFace::GetFacePosition() const {
  return _faceCenter;
}

inline void ProceduralFace::SetFaceScale(Point<2,Value> scale) {
  if(scale.x() < 0) {
    ClipWarnFcn("FaceScaleX", scale.x(), 0, std::numeric_limits<Value>::max());
    scale.x() = 0;
  }
  if(scale.y() < 0) {
    ClipWarnFcn("FaceScaleY", scale.y(), 0, std::numeric_limits<Value>::max());
    scale.y() = 0;
  }
  _faceScale = scale;
}

inline Point<2,ProceduralFace::Value> const& ProceduralFace::GetFaceScale() const {
  return _faceScale;
}
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ProceduralFace_H__