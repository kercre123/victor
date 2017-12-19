/**
 * File: behaviorRockPaperScissors.h
 *
 * Author: Andrew Stein
 * Created: 2017-12-18
 *
 * Description: R&D Behavior to play rock, paper, scissors.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorRockPaperScissors : public ICozmoBehavior
{
  friend class BehaviorContainer;
  explicit BehaviorRockPaperScissors(const Json::Value& config);

public:

  virtual ~BehaviorRockPaperScissors();

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override
  {
    return true;
  }

protected:

  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override { return false; }

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual BehaviorStatus UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) override;  
  
  void DisplaySelection(BehaviorExternalInterface& behaviorExternalInterface);
  
private:

  enum State {
    WaitForButton,
    PlayCadence,
    WaitForResult,
    React
  };

  State _state = WaitForButton;

  enum Selection {
    Unknown  = -1,
    Rock     = 0,
    Paper    = 1,
    Scissors = 2
  };
  
  Selection _robotSelection;
  Selection _humanSelection;
  
};

}
}


#endif /* __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__ */
