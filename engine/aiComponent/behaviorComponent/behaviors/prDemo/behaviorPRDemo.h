/**
 * File: BehaviorPRDemo.h
 *
 * Author: Brad
 * Created: 2018-07-05
 *
 * Description: coordinator for the PR demo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemo__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemo__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent_fwd.h"

// TODO:(bn) make a _fwd for this
#include "clad/types/behaviorComponent/activeFeatures.h"

namespace Anki {
namespace Vector {

class BehaviorPRDemo : public InternalStatesBehavior
{
public: 
  virtual ~BehaviorPRDemo();

  // let this behavior know about things it needs for the demo
  void InformOfVoiceIntent( const UserIntentTag& intentTag );
  void InformTimerIsRinging();
  
  void Reset();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPRDemo(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

  virtual void OverrideResumeState( StateID& resumeState ) override;
  
private:

  void SetOverideState(StateID state);

  StateID _resumeOverride = InternalStatesBehavior::InvalidStateID;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPRDemo__
