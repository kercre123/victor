/**
 * File: conditionIsMaintenanceReboot.h
 *
 * Author: Brad Neuman
 * Created: 2018-09-04
 *
 * Description: Checks OS state to see if this is a maintenance reboot
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionIsMaintenanceReboot_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionIsMaintenanceReboot_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionIsMaintenanceReboot : public IBEICondition
{
public:
  
  explicit ConditionIsMaintenanceReboot(const Json::Value& config);

protected:
  
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
};
  
} // namespace
} // namespace


#endif
