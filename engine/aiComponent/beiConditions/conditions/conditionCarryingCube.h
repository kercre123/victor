/**
 * File: conditionCarryingCube
 *
 * Author: Matt Michini
 * Created: 05/17/2018
 *
 * Description: Condition that is true when the robot is carrying a cube (or at least _thinks_ he is).
 *
 * Copyright: Anki, Inc. 2018
 *
 *
 **/

#ifndef __Engine_BeiConditions_ConditionCarryingCube_H__
#define __Engine_BeiConditions_ConditionCarryingCube_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionCarryingCube : public IBEICondition{
public:
  explicit ConditionCarryingCube(const Json::Value& config);

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};


} // namespace Vector
} // namespace Anki

#endif // __Engine_BeiConditions_ConditionCarryingCube_H__
