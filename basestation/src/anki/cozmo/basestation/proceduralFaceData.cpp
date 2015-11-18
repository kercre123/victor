/**
 * File: proceduralFaceData.cpp
 *
 * Author: Lee Crippen
 * Created: 11/17/15
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#include "anki/cozmo/basestation/proceduralFaceData.h"
#include "anki/common/basestation/jsonTools.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
ProceduralFaceData::ProceduralFaceData()
{
  Reset();
}

void ProceduralFaceData::Reset()
{
  _faceAngle = 0;
  _faceCenter = {0,0};
  _faceScale = {1.f,1.f};
  
  _eyeParams[Left].fill(0);
  _eyeParams[Right].fill(0);
  
  for(auto whichEye : {Left, Right}) {
    SetParameter(whichEye, Parameter::EyeScaleX, whichEye == Left ? 1.01f : 1.19f);
    SetParameter(whichEye, Parameter::EyeScaleY, whichEye == Left ? 0.87f : 1.0f);
    SetParameter(whichEye, Parameter::EyeCenterX, whichEye == Left ? 42 : 83);
    SetParameter(whichEye, Parameter::EyeCenterY, whichEye == Left ? 36 : 35);
    
    for(auto radius : {Parameter::UpperInnerRadiusX, Parameter::UpperInnerRadiusY,
      Parameter::UpperOuterRadiusX, Parameter::UpperOuterRadiusY,
      Parameter::LowerInnerRadiusX, Parameter::LowerInnerRadiusY,
      Parameter::LowerOuterRadiusX, Parameter::LowerOuterRadiusY})
    {
      SetParameter(whichEye, radius, 0.61f);
    }
  }
}
  
static const char* kFaceAngleKey = "faceAngle";
static const char* kFaceCenterXKey = "faceCenterX";
static const char* kFaceCenterYKey = "faceCenterY";
static const char* kFaceScaleXKey = "faceScaleX";
static const char* kFaceScaleYKey = "faceScaleY";
static const char* kLeftEyeKey = "leftEye";
static const char* kRightEyeKey = "rightEye";

void ProceduralFaceData::SetEyeArrayHelper(WhichEye eye, const std::vector<Value>& eyeArray)
{
  const char* eyeStr = (eye == WhichEye::Left) ? kLeftEyeKey : kRightEyeKey;

  const size_t N = static_cast<size_t>(Parameter::NumParameters);
  if(eyeArray.size() != N) {
    PRINT_NAMED_WARNING("ProceduralFaceData.SetEyeArrayHelper.WrongNumParams",
                        "Unexpected number of parameters for %s array (%lu vs. %lu)",
                        eyeStr, eyeArray.size(), N);
    return;
  }
  
  for(s32 i=0; i<std::min(eyeArray.size(), N); ++i)
  {
    SetParameter(eye, static_cast<ProceduralFaceData::Parameter>(i),eyeArray[i]);
  }
}
  
void ProceduralFaceData::SetFromJson(const Json::Value &jsonRoot)
{
  Value tempValue;
  if (JsonTools::GetValueOptional(jsonRoot, kFaceAngleKey, tempValue))
  {
    _faceAngle = tempValue;
  }
  
  if (JsonTools::GetValueOptional(jsonRoot, kFaceCenterXKey, tempValue))
  {
    _faceCenter.x() = tempValue;
  }
  
  if (JsonTools::GetValueOptional(jsonRoot, kFaceCenterYKey, tempValue))
  {
    _faceCenter.y() = tempValue;
  }
  
  if (JsonTools::GetValueOptional(jsonRoot, kFaceScaleXKey, tempValue))
  {
    _faceScale.x() = tempValue;
  }
  
  if (JsonTools::GetValueOptional(jsonRoot, kFaceScaleYKey, tempValue))
  {
    _faceScale.y() = tempValue;
  }
  
  std::vector<Value> eyeArray;
  if (JsonTools::GetVectorOptional(jsonRoot, kLeftEyeKey, eyeArray))
  {
    SetEyeArrayHelper(WhichEye::Left, eyeArray);
  }
  
  eyeArray.clear();
  if (JsonTools::GetVectorOptional(jsonRoot, kRightEyeKey, eyeArray))
  {
    SetEyeArrayHelper(WhichEye::Right, eyeArray);
  }
}
  
void ProceduralFaceData::SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg)
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

inline static ProceduralFaceData::Value BlendAngleHelper(const ProceduralFaceData::Value angle1,
                                                         const ProceduralFaceData::Value angle2,
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
  
  return static_cast<ProceduralFaceData::Value>(RAD_TO_DEG(std::atan2(y,x)));
}


void ProceduralFaceData::Interpolate(const ProceduralFaceData& face1, const ProceduralFaceData& face2,
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


} // namespace Cozmo
} // namespace Anki