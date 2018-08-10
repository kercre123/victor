/**
 * File: conditionConnectedToCube.h
 *
 * Author: Sam Russell
 * Created: 2018 August 3
 *
 * Description: Condition that checks for a cube connection of the specified type (interactable/background)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionConnectedToCube_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionConnectedToCube_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "clad/types/cubeConnectionTypes.h"

namespace Anki {
namespace Vector {

class ConditionConnectedToCube : public IBEICondition
{
public:
  explicit ConditionConnectedToCube(const Json::Value& config);
  virtual ~ConditionConnectedToCube() = default;

protected:
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;

  CubeConnectionType _requiredConnectionType;
};


} // namespace Vector
} // namespace Anki


#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionConnectedToCube_H__
