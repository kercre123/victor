/**
 * File: ISimpleFaceBehavior.h
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: Base for behaviors (usually voice commands) that do some simple face-related action after another
 *              behavior finds a face and passes it to SetTargetFace().
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_ISimpleFaceBehavior__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_ISimpleFaceBehavior__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Vector {

class ISimpleFaceBehavior : public ICozmoBehavior
{
public:
  virtual ~ISimpleFaceBehavior(){}
  
  void SetTargetFace( SmartFaceID faceID ) { _targetFaceID = faceID; }

protected:
  ISimpleFaceBehavior(const Json::Value& config) : ICozmoBehavior(config) {}
  virtual void OnBehaviorDeactivated() override { _targetFaceID.Reset(); }

  
  SmartFaceID GetTargetFace() const { return _targetFaceID; }
  
private:

  SmartFaceID _targetFaceID;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_ISimpleFaceBehavior__
