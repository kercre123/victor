/**
 * File: BehaviorShowText.cpp
 *
 * Author: Brad
 * Created: 2019-01-07
 *
 * Description: Display fixed (or programatically set) text for a given time
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorShowText.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {
  

namespace {
const char* kTextKey = "text";
const char* kTimeoutKey = "timeout_s";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorShowText::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorShowText::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorShowText::BehaviorShowText(const Json::Value& config)
 : ICozmoBehavior(config)
{
  for( const auto& line : config[kTextKey] ) {
    _iConfig.text.emplace_back( line.asString() );
  }

  _iConfig.timeout_s = config.get(kTimeoutKey, -1.0f).asFloat();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorShowText::~BehaviorShowText()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorShowText::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorShowText::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorShowText::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kTextKey,
    kTimeoutKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorShowText::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( _iConfig.timeout_s > 0.0f ) {
    _dVars.cancelTime_s = currTime_s + _iConfig.timeout_s;
  }
  else {
    // if set negative, cancel immediately after activation
    _dVars.cancelTime_s = currTime_s;
  }

  SmartDisableKeepFaceAlive();
  
  // TODO:(bn) move this somewhere more reasonable
  IBehaviorSelfTest::DrawTextOnScreen(GetBEI(), _iConfig.text);

  // TODO:(bn) do I need to "undo" this somehow if this behavior is interrupted?
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorShowText::BehaviorUpdate() 
{
  if( IsActivated() &&
      _dVars.cancelTime_s > 0.0f &&
      BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() >= _dVars.cancelTime_s ) {
    CancelSelf();
  }
}

}
}
