/**
 * File: behaviorPlaypenDriveForwards.h
 *
 * Author: Al Chaussee
 * Created: 07/28/17
 *
 * Description: Drives Cozmo fowards over a narrow cliff checking that all 4 cliff sensors are working
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Vector {

class BehaviorPlaypenDriveForwards : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenDriveForwards(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }

  virtual Result         OnBehaviorActivatedInternal()   override;
  virtual PlaypenStatus PlaypenUpdateInternal() override;
  virtual void           OnBehaviorDeactivated()   override;
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event) override;
    
private:

  void TransitionToWaitingForBackCliffs();
  void TransitionToWaitingForBackCliffUndetected();
  
  // Driving over the narrow cliff in playpen should trigger four cliff events
  // First the front cliffs should be detected and then undetected, then the back cliffs should
  // be detected and finally undetected
  enum State {
    NONE,
    WAITING_FOR_FRONT_CLIFFS,
    WAITING_FOR_FRONT_CLIFFS_UNDETECTED,
    WAITING_FOR_BACK_CLIFFS,
    WAITING_FOR_BACK_CLIFFS_UNDETECTED
  };
  State _waitingForCliffsState = NONE;

  bool _frontCliffsDetected = false;
  bool _backCliffsDetected = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__
