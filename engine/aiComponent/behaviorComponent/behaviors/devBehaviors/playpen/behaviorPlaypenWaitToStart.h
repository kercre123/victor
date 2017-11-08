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

#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenWaitToStart : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenWaitToStart(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual BehaviorStatus PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void HandleWhileActivatedInternal(const RobotToEngineEvent& event, 
                                            BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  TimeStamp_t _touchStartTime_ms = 0;

  bool _buttonPressed = false;

  bool _needLightUpdate = false;

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

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
