/**
 * File: BehaviorPRDemo.cpp
 *
 * Author: Brad
 * Created: 2018-07-05
 *
 * Description: coordinator for the PR demo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/prDemo/behaviorPRDemo.h"

#include "engine/aiComponent/behaviorComponent/userIntents.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemo::BehaviorPRDemo(const Json::Value& config)
 : InternalStatesBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemo::~BehaviorPRDemo()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemo::Reset()
{
  _resumeOverride = InternalStatesBehavior::InvalidStateID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemo::OverrideResumeState( StateID& resumeState )
{
  if( InternalStatesBehavior::InvalidStateID != _resumeOverride ) {
    resumeState = _resumeOverride;
    _resumeOverride = InternalStatesBehavior::InvalidStateID;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemo::InformOfVoiceIntent( const UserIntentTag& intentTag )
{
  // special case for demo #1 set timer enters exploring
  if( intentTag == USER_INTENT(set_timer) && 
      GetCurrentStateID() == GetStateID("Socialize") ) {
    SetOverideState(GetStateID("Exploring"));
  }
  else if( intentTag == USER_INTENT(imperative_come) ) {
    SetOverideState(GetStateID("Socialize"));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemo::InformTimerIsRinging()
{
  if( GetCurrentStateID() == GetStateID("Exploring") ) {
    SetOverideState(GetStateID("Observing"));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemo::SetOverideState(StateID state)
{
  if( _resumeOverride != InternalStatesBehavior::InvalidStateID &&
      _resumeOverride != state ) {
    PRINT_NAMED_WARNING("BehaviorPRDemo.SetOverrideState.Repalcing",
                        "Wiping out old override state of '%s' with new state '%s'",
                        GetStateName(_resumeOverride).c_str(),
                        GetStateName(state).c_str());
  }

  PRINT_CH_INFO("Demo", "BehaviorPRDemo.SetOverrideState", "%s", GetStateName(state).c_str());
  
  _resumeOverride = state;
}


}
}
