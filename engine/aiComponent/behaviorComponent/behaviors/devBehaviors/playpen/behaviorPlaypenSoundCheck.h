/**
 * File: behaviorPlaypenSoundCheck.h
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks speaker and mics work
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenSoundCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenSoundCheck_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Vector {

class BehaviorPlaypenSoundCheck : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenSoundCheck(const Json::Value& config);
  
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

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
