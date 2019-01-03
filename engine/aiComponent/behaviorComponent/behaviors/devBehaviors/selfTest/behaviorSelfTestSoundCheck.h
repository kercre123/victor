/**
 * File: behaviorSelfTestSoundCheck.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Checks speaker and mics work
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestSoundCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestSoundCheck_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestSoundCheck : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestSoundCheck(const Json::Value& config);
  
protected:
  virtual void InitBehaviorInternal() override;
  
  virtual Result OnBehaviorActivatedInternal() override;
  virtual void   OnBehaviorDeactivated() override;

  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;
  
private:
  
  void TransitionToPlayingSound();
  
  // Whether or not the sound animation has completed
  bool _soundComplete = false;

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestDriftCheck_H__
