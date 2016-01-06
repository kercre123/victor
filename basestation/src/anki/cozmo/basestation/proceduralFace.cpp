/**
 * File: ProceduralFace.cpp
 *
 * Author: Lee Crippen
 * Created: 11/17/15
 *
 * Description: Holds and sets the face rig data used by ProceduralFace.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#include "anki/cozmo/basestation/proceduralFaceParams.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/point_impl.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
ProceduralFace* ProceduralFace::_resetData = nullptr;
  
void ProceduralFace::SetResetData(const ProceduralFace& newResetData)
{
  ProceduralFace* oldPointer = _resetData;
  _resetData = new ProceduralFace(newResetData);
  Util::SafeDelete(oldPointer);
}
  
void ProceduralFace::Reset()
{
  if (nullptr != _resetData)
  {
    *this = *_resetData;
  }
}
  
ProceduralFace::ProceduralFace()
{
  static const auto eyeScaleParamsList =
  {
    Parameter::EyeScaleX,
    Parameter::EyeScaleY
  };
  
  for (auto param : eyeScaleParamsList)
  {
    _eyeParams[WhichEye::Left][(int)param] = 1.0f;
  }
  _eyeParams[WhichEye::Right] = _eyeParams[WhichEye::Left];
}
  
static const char* kFaceAngleKey = "faceAngle";
static const char* kFaceCenterXKey = "faceCenterX";
static const char* kFaceCenterYKey = "faceCenterY";
static const char* kFaceScaleXKey = "faceScaleX";
static const char* kFaceScaleYKey = "faceScaleY";
static const char* kLeftEyeKey = "leftEye";
static const char* kRightEyeKey = "rightEye";

void ProceduralFace::SetEyeArrayHelper(WhichEye eye, const std::vector<Value>& eyeArray)
{
  const char* eyeStr = (eye == WhichEye::Left) ? kLeftEyeKey : kRightEyeKey;

  const size_t N = static_cast<size_t>(Parameter::NumParameters);
  if(eyeArray.size() != N) {
    PRINT_NAMED_WARNING("ProceduralFace.SetEyeArrayHelper.WrongNumParams",
                        "Unexpected number of parameters for %s array (%lu vs. %lu)",
                        eyeStr, eyeArray.size(), N);
    return;
  }
  
  for(s32 i=0; i<std::min(eyeArray.size(), N); ++i)
  {
    SetParameter(eye, static_cast<ProceduralFace::Parameter>(i),eyeArray[i]);
  }
}
  
void ProceduralFace::SetFromJson(const Json::Value &jsonRoot)
{
  JsonTools::GetValueOptional(jsonRoot, kFaceAngleKey, _faceAngle);
  JsonTools::GetValueOptional(jsonRoot, kFaceCenterXKey, _faceCenter.x());
  JsonTools::GetValueOptional(jsonRoot, kFaceCenterYKey, _faceCenter.y());
  JsonTools::GetValueOptional(jsonRoot, kFaceScaleXKey, _faceScale.x());
  JsonTools::GetValueOptional(jsonRoot, kFaceScaleYKey, _faceScale.y());
  JsonTools::GetArrayOptional(jsonRoot, kLeftEyeKey, _eyeParams[WhichEye::Left]);
  JsonTools::GetArrayOptional(jsonRoot, kRightEyeKey, _eyeParams[WhichEye::Right]);
}
  
void ProceduralFace::SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg)
{
  SetFaceAngle(msg.faceAngle);
  SetFacePosition({msg.faceCenX, msg.faceCenY});
  SetFaceScale({msg.faceScaleX, msg.faceScaleY});
  SetEyeArrayHelper(WhichEye::Left, msg.leftEye);
  SetEyeArrayHelper(WhichEye::Right, msg.rightEye);
}
  
template<typename T>
inline static T LinearBlendHelper(const T value1, const T value2, const float blendFraction)
{
  if(value1 == value2) {
    // Special case, no math needed
    return value1;
  }
  
  T blendValue = static_cast<T>((1.f - blendFraction)*static_cast<float>(value1) +
                                blendFraction*static_cast<float>(value2));
  return blendValue;
}

inline static ProceduralFace::Value BlendAngleHelper(const ProceduralFace::Value angle1,
                                                         const ProceduralFace::Value angle2,
                                                         const float blendFraction)
{
  if(angle1 == angle2) {
    // Special case, no math needed
    return angle1;
  }
  
  const float angle1_rad = DEG_TO_RAD(static_cast<float>(angle1));
  const float angle2_rad = DEG_TO_RAD(static_cast<float>(angle2));
  
  const float x = LinearBlendHelper(std::cos(angle1_rad), std::cos(angle2_rad), blendFraction);
  const float y = LinearBlendHelper(std::sin(angle1_rad), std::sin(angle2_rad), blendFraction);
  
  return static_cast<ProceduralFace::Value>(RAD_TO_DEG(std::atan2(y,x)));
}


void ProceduralFace::Interpolate(const ProceduralFace& face1, const ProceduralFace& face2,
                                     float blendFraction, bool usePupilSaccades)
{
  assert(blendFraction >= 0.f && blendFraction <= 1.f);
  
  // Special cases, no blending required:
  if(blendFraction == 0.f) {
    *this = face1;
    return;
  } else if(blendFraction == 1.f) {
    *this = face2;
    return;
  }
  
  for(int iWhichEye=0; iWhichEye < 2; ++iWhichEye)
  {
    WhichEye whichEye = static_cast<WhichEye>(iWhichEye);
    
    for(int iParam=0; iParam < static_cast<int>(Parameter::NumParameters); ++iParam)
    {
      Parameter param = static_cast<Parameter>(iParam);
      
      if(Parameter::EyeAngle == param) {
        SetParameter(whichEye, param, BlendAngleHelper(face1.GetParameter(whichEye, param),
                                                       face2.GetParameter(whichEye, param),
                                                       blendFraction));
      } else {
        SetParameter(whichEye, param, LinearBlendHelper(face1.GetParameter(whichEye, param),
                                                        face2.GetParameter(whichEye, param),
                                                        blendFraction));
      }

    } // for each parameter
  } // for each eye
  
  SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(), face2.GetFaceAngle(), blendFraction));
  SetFacePosition({LinearBlendHelper(face1.GetFacePosition().x(), face2.GetFacePosition().x(), blendFraction),
    LinearBlendHelper(face1.GetFacePosition().y(), face2.GetFacePosition().y(), blendFraction)});
  SetFaceScale({LinearBlendHelper(face1.GetFaceScale().x(), face2.GetFaceScale().x(), blendFraction),
    LinearBlendHelper(face1.GetFaceScale().y(), face2.GetFaceScale().y(), blendFraction)});
  
} // Interpolate()
  
void ProceduralFace::CombineEyeParams(EyeParamArray& eyeArray0, const EyeParamArray& eyeArray1)
{
  static const auto addParamList =
  {
    Parameter::EyeCenterX,
    Parameter::EyeCenterY,
    Parameter::EyeAngle,
    Parameter::UpperLidAngle,
    Parameter::LowerLidAngle,
  };
  for (auto param : addParamList)
  {
    eyeArray0[(int)param] += eyeArray1[(int)param];
  }
  
  static const auto multiplyParamList =
  {
    Parameter::EyeScaleX,
    Parameter::EyeScaleY,
  };
  for (auto param : multiplyParamList)
  {
    eyeArray0[(int)param] *= eyeArray1[(int)param];
  }
}
  
ProceduralFace& ProceduralFace::Combine(const ProceduralFace& otherFace)
{
  CombineEyeParams(_eyeParams[(int)WhichEye::Left], otherFace.GetParameters(WhichEye::Left));
  CombineEyeParams(_eyeParams[(int)WhichEye::Right], otherFace.GetParameters(WhichEye::Right));
  
  _faceAngle += otherFace.GetFaceAngle();
  _faceScale *= otherFace.GetFaceScale();
  _faceCenter += otherFace.GetFacePosition();

  return *this;
}

  
ProceduralFace::Value ProceduralFace::Clip(WhichEye eye, Parameter param, Value newValue) const
{
# define POS_INF std::numeric_limits<Value>::max()
# define NEG_INF std::numeric_limits<Value>::lowest()
  
  static const std::map<Parameter, std::pair<Value,Value>> LimitsLUT = {
    {Parameter::LowerLidAngle,      {-45,   45}},
    {Parameter::UpperLidAngle,      {-45,   45}},
    {Parameter::EyeScaleX,          {  0,   POS_INF}},
    {Parameter::EyeScaleY,          {  0,   POS_INF}},
    {Parameter::LowerInnerRadiusX , {  0,   1}},
    {Parameter::LowerInnerRadiusY , {  0,   1}},
    {Parameter::UpperInnerRadiusX , {  0,   1}},
    {Parameter::UpperInnerRadiusY , {  0,   1}},
    {Parameter::LowerOuterRadiusX , {  0,   1}},
    {Parameter::LowerOuterRadiusY , {  0,   1}},
    {Parameter::UpperOuterRadiusX , {  0,   1}},
    {Parameter::UpperOuterRadiusY , {  0,   1}},
    {Parameter::LowerLidY,          {  0,   1}},
    {Parameter::UpperLidY,          {  0,   1}},
    {Parameter::LowerLidBend,       {  0,   1}},
    {Parameter::UpperLidBend,       {  0,   1}},
  };
  
  auto limitsIter = LimitsLUT.find(param);
  if(limitsIter != LimitsLUT.end())
  {
    const Value MinVal = limitsIter->second.first;
    const Value MaxVal = limitsIter->second.second;
    if(newValue < MinVal) {
      ClipWarnFcn(EnumToString(param), newValue, MinVal, MaxVal);
      newValue = MinVal;
    } else if(newValue > MaxVal) {
      ClipWarnFcn(EnumToString(param), newValue, MinVal, MaxVal);
      newValue = MaxVal;
    }
  }
  
  if(std::isnan(newValue)) {
    PRINT_NAMED_WARNING("ProceduralFace.Clip.NaN",
                        "Returning original value instead of NaN for %s",
                        EnumToString(param));
    newValue = GetParameter(eye, param);
  }
  
  return newValue;
  
# undef POS_INF
# undef NEG_INF
} // Clip()
  
static void ClipWarning(const char* paramName,
                        ProceduralFace::Value value,
                        ProceduralFace::Value minVal,
                        ProceduralFace::Value maxVal)
{
  PRINT_NAMED_WARNING("ProceduralFace.Clip.OutOfRange",
                      "Value of %f out of range [%f,%f] for parameter %s. Clipping.",
                      value, minVal, maxVal, paramName);
}

static void NoClipWarning(const char* paramName,
                          ProceduralFace::Value value,
                          ProceduralFace::Value minVal,
                          ProceduralFace::Value maxVal)
{
  // Do nothing
}

// Start out with warnings enabled:
std::function<void(const char *,
                   ProceduralFace::Value,
                   ProceduralFace::Value,
                   ProceduralFace::Value)> ProceduralFace::ClipWarnFcn = &ClipWarning;

void ProceduralFace::EnableClippingWarning(bool enable)
{
  if(enable) {
    ClipWarnFcn = ClipWarning;
  } else {
    ClipWarnFcn = NoClipWarning;
  }
}

} // namespace Cozmo
} // namespace Anki