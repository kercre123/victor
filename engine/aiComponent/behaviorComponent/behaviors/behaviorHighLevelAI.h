/**
 * File: behaviorHighLevelAI.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the high level AI of victor (similar to Cozmo's
 *              freeplay activities)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/objectIDs.h"
#include "clad/types/visionModes.h"
#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

class BehaviorFeedingEat;

class BehaviorHighLevelAI : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorHighLevelAI(const Json::Value& config);
  
public:

  virtual ~BehaviorHighLevelAI();
  
  virtual bool WantsToBeActivatedBehavior() const override {
    return true; }
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior() override;

  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  using StateID = size_t;
  static const StateID InvalidStateID = 0;
  
  class State;

  enum class TransitionType {
    NonInterrupting,
    Interrupting,
    Exit
  };

  static TransitionType TransitionTypeFromString(const std::string& str);

  // place to put all of the hardcoded / predefined transition strategies
  void CreatePreDefinedStrategies();
  
  // returns the newly created ID
  StateID AddStateName(const std::string& stateName);

  // asserts if not found
  StateID GetStateID(const std::string& stateName) const;

  StateID ParseStateFromJson(const Json::Value& config, const std::string& key);
  IBEIConditionPtr ParseTransitionStrategy(const Json::Value& config);
  
  void AddState( State&& state );
  
  void TransitionToState(StateID targetState);

  bool StateExitCooldownExpired(StateID state, float timeout) const;

  std::map< std::string, StateID > _stateNameToID;

  using StateMap = std::map< StateID, State >;
  std::unique_ptr< StateMap > _states;

  std::map< std::string, IBEIConditionPtr > _preDefinedStrategies;
  
  // hack to turn off all modes when this behavior starts and then back on when it ends
  std::vector< VisionMode > _visionModesToReEnable;

  StateID _currState = InvalidStateID;

  StateID _defaultState = InvalidStateID;

  BackpackLights _currDebugLights;
  bool _debugLightsDirty = false;
  bool _useDebugLights = false;
  float _lastHearbeatLightTime = -1.0f;
};

}
}

#endif
