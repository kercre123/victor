/**
 * File: ProceduralFaceParams.cpp
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
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
ProceduralFaceParams* ProceduralFaceParams::_resetData = nullptr;
  
void ProceduralFaceParams::SetResetData(const ProceduralFaceParams& newResetData)
{
  ProceduralFaceParams* oldPointer = _resetData;
  _resetData = new ProceduralFaceParams(newResetData);
  Util::SafeDelete(oldPointer);
}
  
void ProceduralFaceParams::Reset()
{
  if (nullptr != _resetData)
  {
    *this = *_resetData;
  }
}
  
static const char* kFaceAngleKey = "faceAngle";
static const char* kFaceCenterXKey = "faceCenterX";
static const char* kFaceCenterYKey = "faceCenterY";
static const char* kFaceScaleXKey = "faceScaleX";
static const char* kFaceScaleYKey = "faceScaleY";
static const char* kLeftEyeKey = "leftEye";
static const char* kRightEyeKey = "rightEye";

void ProceduralFaceParams::SetEyeArrayHelper(WhichEye eye, const std::vector<Value>& eyeArray)
{
  const char* eyeStr = (eye == WhichEye::Left) ? kLeftEyeKey : kRightEyeKey;

  const size_t N = static_cast<size_t>(Parameter::NumParameters);
  if(eyeArray.size() != N) {
    PRINT_NAMED_WARNING("ProceduralFaceParams.SetEyeArrayHelper.WrongNumParams",
                        "Unexpected number of parameters for %s array (%lu vs. %lu)",
                        eyeStr, eyeArray.size(), N);
    return;
  }
  
  for(s32 i=0; i<std::min(eyeArray.size(), N); ++i)
  {
    SetParameter(eye, static_cast<ProceduralFaceParams::Parameter>(i),eyeArray[i]);
  }
}
  
void ProceduralFaceParams::SetFromJson(const Json::Value &jsonRoot)
{
  JsonTools::GetValueOptional(jsonRoot, kFaceAngleKey, _faceAngle);
  JsonTools::GetValueOptional(jsonRoot, kFaceCenterXKey, _faceCenter.x());
  JsonTools::GetValueOptional(jsonRoot, kFaceCenterYKey, _faceCenter.y());
  JsonTools::GetValueOptional(jsonRoot, kFaceScaleXKey, _faceScale.x());
  JsonTools::GetValueOptional(jsonRoot, kFaceScaleYKey, _faceScale.y());
  
  std::vector<Value> eyeArray;
  if (JsonTools::GetVectorOptional(jsonRoot, kLeftEyeKey, eyeArray))
  {
    SetEyeArrayHelper(WhichEye::Left, eyeArray);
    eyeArray.clear();
  }
  
  if (JsonTools::GetVectorOptional(jsonRoot, kRightEyeKey, eyeArray))
  {
    SetEyeArrayHelper(WhichEye::Right, eyeArray);
  }
}
  
void ProceduralFaceParams::SetFromMessage(const ExternalInterface::DisplayProceduralFace& msg)
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

inline static ProceduralFaceParams::Value BlendAngleHelper(const ProceduralFaceParams::Value angle1,
                                                         const ProceduralFaceParams::Value angle2,
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
  
  return static_cast<ProceduralFaceParams::Value>(RAD_TO_DEG(std::atan2(y,x)));
}


void ProceduralFaceParams::Interpolate(const ProceduralFaceParams& face1, const ProceduralFaceParams& face2,
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