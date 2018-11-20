/**
 * File: behaviorSelfTestWaitToStart.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestWaitToStart_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestWaitToStart_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

class BehaviorSelfTestWaitToStart : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestWaitToStart(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;

  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;
    
private:
  
  bool _startTest = false;

  BackpackLights _lights = {
    .onColors               = {{NamedColors::BLUE,NamedColors::GREEN,NamedColors::RED}},
    .offColors              = {{NamedColors::BLUE,NamedColors::GREEN,NamedColors::RED}},
    .onPeriod_ms            = {{1000,1000,1000}},
    .offPeriod_ms           = {{100,100,100}},
    .transitionOnPeriod_ms  = {{450,450,450}},
    .transitionOffPeriod_ms = {{450,450,450}},
    .offset                 = {{0,0,0}}
  };

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestWaitToStart_H__
