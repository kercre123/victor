/**
 * File: BehaviorDispatcherAdaptive.h
 *
 * Author: Andrew Stout
 * Created: 2019-01-08
 *
 * Description:
 * 2019 R&D Sprint project: Adaptive Behavior Coordinator.
 * Chooses delegates according to a state-action-value function which is updated through very simple reinforcement
 * learning.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDispatcherAdaptive__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDispatcherAdaptive__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

#include "coretech/vision/engine/faceIdTypes.h"

namespace Anki {
namespace Vector {

// should I move all these typedefs inside the class? "Action" seems likely to be overloaded...

// State needs to be something hashable
// for now, just the type of FaceID
using State = Vision::FaceID_t;
// we just use the behaviorID to represent the action for bookkeeping purposes
using Action = std::string;
using SAValue = float;

using StateActionValueTable = std::map< State, std::map< Action, SAValue> >;

// I think we should have a StateActionValueTable class, to provide a little bit of protection
// eh. it's just a map of maps. Be a little careful using them, that's good enough for now.
/*
template<class T, class V>
class ValueTable {
public:
  ValueTable();


private:
  std::map< T, V > _V;


};
*/



class BehaviorDispatcherAdaptive : public IBehaviorDispatcher
{
public: 
  virtual ~BehaviorDispatcherAdaptive();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDispatcherAdaptive(const Json::Value& config);  

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void InitDispatcher() override;
  virtual void BehaviorDispatcher_OnActivated() override;
  virtual void BehaviorDispatcher_OnDeactivated() override;
  virtual void DispatcherUpdate() override;


private:

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
    std::vector<std::string> actionSpace;
    std::string defaultBehaviorName;
    ICozmoBehaviorPtr defaultBehavior;
    StateActionValueTable SAVTable;

    // I think we'll want to store the state-action value function here
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
    ICozmoBehaviorPtr lastSelectedBehavior;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  State EvaluateState();
  SAValue GetStateActionValue(State s, Action a);
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDispatcherAdaptive__
