/**
 * File: conditionUnitTest.h
 *
 * Author: ross
 * Created: 2018 Apr 10
 *
 * Description: Condition used exclusively for unit tests where you can set the value manually
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionUnitTest_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionUnitTest_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionUnitTest : public IBEICondition
{
public:
  explicit ConditionUnitTest( bool val )
    : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::UnitTestCondition))
    , _val( val )
  {
    SetOwnerDebugLabel("tests");
  }
  ConditionUnitTest( const Json::Value& config )
    : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::UnitTestCondition))
    , _val( config.get("value", false).asBool() )
  {
  }

  virtual void InitInternal( BehaviorExternalInterface& behaviorExternalInterface ) override {
    _initCount++;
  }

  virtual void SetActiveInternal( BehaviorExternalInterface& behaviorExternalInterface, bool setActive ) override {
    if(setActive){
      _setActiveCount++;
    }
  }

  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& behaviorExternalInterface ) const override {
    _areMetCount++;
    return _val;
  }
  
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override {}

  bool _val = false;

  int _initCount = 0;
  int _setActiveCount = 0;
  mutable int _areMetCount = 0;
};

}
}


#endif
