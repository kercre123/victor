/**
 * File: BehaviorMultiRobotInteractions.h
 *
 * Author: Kevin Yoon
 * Date:   12/18/2017
 *
 * Description: Kevin's R&D behavior for demonstrating interactions
 *              between Victors
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorMultiRobotInteractions_H__
#define __Cozmo_Basestation_Behaviors_BehaviorMultiRobotInteractions_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/touchGestureTypes.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class IStateConceptStrategy;
class MultiRobotComponent;
  
class BehaviorMultiRobotInteractions : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorMultiRobotInteractions() { }
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  friend class BehaviorContainer;
  BehaviorMultiRobotInteractions(const Json::Value& config);
  
  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual BehaviorStatus UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  void HandleMultiRobotSessionStarted(const ExternalInterface::MultiRobotSessionStarted& msg, BehaviorExternalInterface& behaviorExternalInterface);
  
  ICozmoBehaviorPtr _fistBumpMasterBehavior;
  ICozmoBehaviorPtr _fistBumpSlaveBehavior;

  MultiRobotComponent* _multiRobotComponent;

private:
  enum class State {
    Idle,
    RequestInteraction,
    WaitForResponse,
    DoInteraction
  };
  void SetState_internal(State state, const std::string& stateName);
  
  State _state;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorMultiRobotInteractions_H__
