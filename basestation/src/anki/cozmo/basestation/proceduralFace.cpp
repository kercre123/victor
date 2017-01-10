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
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/point_impl.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"
#include "cozmo_anim_generated.h"

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
                        eyeStr, (unsigned long)eyeArray.size(), (unsigned long)N);
    return;
  }
  
  for(s32 i=0; i<std::min(eyeArray.size(), N); ++i)
  {
    SetParameter(eye, static_cast<ProceduralFace::Parameter>(i),eyeArray[i]);
  }
}

void ProceduralFace::SetFromFlatBuf(const CozmoAnim::ProceduralFace* procFaceKeyframe)
{
  std::vector<Value> eyeParams;

  auto leftEyeData = procFaceKeyframe->leftEye();
  for (int leIdx=0; leIdx < leftEyeData->size(); leIdx++) {
    auto leftEyeVal = leftEyeData->Get(leIdx);
    eyeParams.push_back(leftEyeVal);
  }
  SetEyeArrayHelper(WhichEye::Left, eyeParams);

  eyeParams.clear();

  auto rightEyeData = procFaceKeyframe->rightEye();
  for (int reIdx=0; reIdx < rightEyeData->size(); reIdx++) {
    auto rightEyeVal = rightEyeData->Get(reIdx);
    eyeParams.push_back(rightEyeVal);
  }
  SetEyeArrayHelper(WhichEye::Right, eyeParams);
 
  f32 jsonFaceAngle = procFaceKeyframe->faceAngle();
  SetFaceAngle(jsonFaceAngle);
 
  f32 fbFaceCenterX = procFaceKeyframe->faceCenterX();
  f32 fbFaceCenterY = procFaceKeyframe->faceCenterY();
  SetFacePosition({fbFaceCenterX, fbFaceCenterY});
 
  f32 fbFaceScaleX = procFaceKeyframe->faceScaleX();
  f32 fbFaceScaleY = procFaceKeyframe->faceScaleY();
  SetFaceScale({fbFaceScaleX, fbFaceScaleY});
}

void ProceduralFace::SetFromJson(const Json::Value &jsonRoot)
{
  std::vector<Value> eyeParams;
  if(JsonTools::GetVectorOptional(jsonRoot, kLeftEyeKey, eyeParams)) {
    SetEyeArrayHelper(WhichEye::Left, eyeParams);
  }
  eyeParams.clear();
  if(JsonTools::GetVectorOptional(jsonRoot, kRightEyeKey, eyeParams)) {
    SetEyeArrayHelper(WhichEye::Right, eyeParams);
  }
  
  f32 jsonFaceAngle=0;
  if(JsonTools::GetValueOptional(jsonRoot, kFaceAngleKey, jsonFaceAngle)) {
    SetFaceAngle(jsonFaceAngle);
  }
  
  f32 jsonFaceCenX=0, jsonFaceCenY=0;
  if(JsonTools::GetValueOptional(jsonRoot, kFaceCenterXKey, jsonFaceCenX) &&
     JsonTools::GetValueOptional(jsonRoot, kFaceCenterYKey, jsonFaceCenY))
  {
    SetFacePosition({jsonFaceCenX, jsonFaceCenY});
  }
  
  f32 jsonFaceScaleX=1.f,jsonFaceScaleY=1.f;
  if(JsonTools::GetValueOptional(jsonRoot, kFaceScaleXKey, jsonFaceScaleX) &&
     JsonTools::GetValueOptional(jsonRoot, kFaceScaleYKey, jsonFaceScaleY))
  {
    SetFaceScale({jsonFaceScaleX, jsonFaceScaleY});
  }
}
  
void ProceduralFace::SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg)
{
  SetFaceAngle(msg.faceAngle_deg);
  SetFacePosition({msg.faceCenX, msg.faceCenY});
  SetFaceScale({msg.faceScaleX, msg.faceScaleY});
  SetEyeArrayHelper(WhichEye::Left, msg.leftEye);
  SetEyeArrayHelper(WhichEye::Right, msg.rightEye);
}
  
void ProceduralFace::LookAt(f32 xShift, f32 yShift, f32 xmax, f32 ymax,
                                  f32 lookUpMaxScale, f32 lookDownMinScale, f32 outerEyeScaleIncrease)
{
  SetFacePosition({xShift, yShift});
  
  // Amount "outer" eye will increase in scale depending on how far left/right we look
  const f32 yscaleLR = 1.f + outerEyeScaleIncrease * std::min(1.f, std::abs(xShift)/xmax);
  
  // Amount both eyes will increase/decrease in size depending on how far we look
  // up or down
  const f32 yscaleUD = (lookUpMaxScale-lookDownMinScale)*std::min(1.f, (1.f - (yShift + ymax)/(2.f*ymax))) + lookDownMinScale;
  
  if(xShift < 0) {
    SetParameter(WhichEye::Left,  ProceduralEyeParameter::EyeScaleY, yscaleLR*yscaleUD);
    SetParameter(WhichEye::Right, ProceduralEyeParameter::EyeScaleY, (2.f-yscaleLR)*yscaleUD);
  } else {
    SetParameter(WhichEye::Left,  ProceduralEyeParameter::EyeScaleY, (2.f-yscaleLR)*yscaleUD);
    SetParameter(WhichEye::Right, ProceduralEyeParameter::EyeScaleY, yscaleLR*yscaleUD);
  }
  
  DEV_ASSERT_MSG(FLT_GT(GetParameter(WhichEye::Left,  ProceduralEyeParameter::EyeScaleY), 0.f),
                 "ProceduralFace.LookAt.NegativeLeftEyeScaleY",
                 "yShift=%f yscaleLR=%f yscaleUD=%f ymax=%f",
                 yShift, yscaleLR, yscaleUD, ymax);
  DEV_ASSERT_MSG(FLT_GT(GetParameter(WhichEye::Right, ProceduralEyeParameter::EyeScaleY), 0.f),
                 "ProceduralFace.LookAt.NegativeRightEyeScaleY",
                 "yShift=%f yscaleLR=%f yscaleUD=%f ymax=%f",
                 yShift, yscaleLR, yscaleUD, ymax);
  
  //SetParameterBothEyes(ProceduralEyeParameter::EyeScaleX, xscale);
  
  // If looking down (positive y), push eyes together (IOD=interocular distance)
  const f32 MaxIOD = 2.f;
  f32 reduceIOD = 0.f;
  if(yShift > 0) {
    reduceIOD = MaxIOD*std::min(1.f, yShift/ymax);
  }
  SetParameter(WhichEye::Left,  ProceduralEyeParameter::EyeCenterX,  reduceIOD);
  SetParameter(WhichEye::Right, ProceduralEyeParameter::EyeCenterX, -reduceIOD);
  
  //PRINT_NAMED_DEBUG("ProceduralFace.LookAt",
  //                  "shift=(%.1f,%.1f), up/down scale=%.3f, left/right scale=%.3f), reduceIOD=%.3f",
  //                  xShift, yShift, yscaleUD, yscaleLR, reduceIOD);
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
  
void ProceduralFace::GetEyeBoundingBox(Value& xmin, Value& xmax, Value& ymin, Value& ymax)
{
  // Left edge of left eye
  const Value leftHalfWidth = GetParameter(WhichEye::Left, Parameter::EyeScaleX) * NominalEyeWidth/2;
  const Value rightHalfWidth = GetParameter(WhichEye::Right, Parameter::EyeScaleX) * NominalEyeWidth/2;
  xmin = (ProceduralFace::NominalLeftEyeX +
          _faceScale.x() * (GetParameter(WhichEye::Left, Parameter::EyeCenterX) - leftHalfWidth));
  
  // Right edge of right eye
  xmax = (ProceduralFace::NominalRightEyeX +
          _faceScale.x()*(GetParameter(WhichEye::Right, Parameter::EyeCenterX) + rightHalfWidth));
  
  
  // Min of the top edges of the two eyes
  const Value leftHalfHeight = GetParameter(WhichEye::Left, Parameter::EyeScaleY) * NominalEyeHeight/2;
  const Value rightHalfHeight = GetParameter(WhichEye::Right, Parameter::EyeScaleY) * NominalEyeHeight/2;
  ymin = (NominalEyeY + _faceScale.y() * (std::min(GetParameter(WhichEye::Left, Parameter::EyeCenterY) - leftHalfHeight,
                                                   GetParameter(WhichEye::Right, Parameter::EyeCenterY) - rightHalfHeight)));
  
  // Max of the bottom edges of the two eyes
  ymax = (NominalEyeY + _faceScale.y() * (std::max(GetParameter(WhichEye::Left, Parameter::EyeCenterY) + leftHalfHeight,
                                                   GetParameter(WhichEye::Right, Parameter::EyeCenterY) + rightHalfHeight)));
  
} // GetEyeBoundingBox()
  
void ProceduralFace::SetFacePosition(Point<2, Value> center)
{
  // Try not to let the eyes drift off the face
  // NOTE: (1) if you set center and *then* change eye centers/scales, you could still go off screen
  //       (2) this also doesn't take lid height into account, so if the top lid is half closed and
  //           you move the eyes way down, it could look like they disappeared, for example
  
  Value xmin=0, xmax=0, ymin=0, ymax=0;
  GetEyeBoundingBox(xmin, xmax, ymin, ymax);
  
  // The most we can move left is the distance b/w left edge of left eye and the
  // left edge of the screen. The most we can move right is the distance b/w the
  // right edge of the right eye and the right edge of the screen
  _faceCenter.x() = CLIP(center.x(), -xmin, ProceduralFace::WIDTH-xmax);
  _faceCenter.y() = CLIP(center.y(), -ymin, ProceduralFace::HEIGHT-ymax);
}
  
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
  
  _faceAngle_deg += otherFace.GetFaceAngle();
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
