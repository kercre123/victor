/**
 * File: behaviorPlaypenWaitToStart.h
 *
 * Author: Al Chaussee
 * Created: 10/12/17
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/engineTimeStamp.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenWaitToStart : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenWaitToStart(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  
  virtual Result        OnBehaviorActivatedInternal() override;
  virtual PlaypenStatus PlaypenUpdateInternal() override;
  virtual void          OnBehaviorDeactivated() override;
    
private:
  
  EngineTimeStamp_t _touchStartTime_ms = 0;

  bool _buttonPressed = false;

  bool _needLightUpdate = false;

  BackpackLightAnimation::BackpackAnimation _lights = {
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

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
