/**
 * File: conditionAlexaEnabled.cpp
 *
 * Author: Jarrod
 * Created: 2018 november 11th
 *
 * Description: True if a Alexa is enabled
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionAlexaEnabled_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionAlexaEnabled_H__
#pragma once

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ConditionAlexaEnabled : public IBEICondition
{
public:
  explicit ConditionAlexaEnabled( const Json::Value& config );

  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& bei ) const override;


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool _expected;
};

}
}

#endif
