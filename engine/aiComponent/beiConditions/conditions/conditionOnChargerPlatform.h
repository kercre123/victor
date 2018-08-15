/**
 * File: conditionOnChargerPlatform.h
 *
 * Author: Brad Neuman
 * Created: 2018-04-26
 *
 * Description: True if on the platform (not the contacts)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionOnChargerPlatform_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionOnChargerPlatform_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

class ConditionOnChargerPlatform : public IBEICondition
{
public:
  explicit ConditionOnChargerPlatform(const Json::Value& config);

  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& bei) const override;
};

}
}


#endif
