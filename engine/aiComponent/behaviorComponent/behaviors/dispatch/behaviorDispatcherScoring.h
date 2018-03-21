/**
 * File: behaviorDispatcherScoring.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2017-11-2
 *
 * Description: Simple behavior which takes a json-defined list of other behaviors
 * and dispatches to the behavior in the list that has the highest evaluated score
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherScoring_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherScoring_H__

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/iBehaviorDispatcher.h"

namespace Anki {
namespace Cozmo {

// forward Declaration
class BehaviorScoringWrapper;

class BehaviorDispatcherScoring : public IBehaviorDispatcher
{
public:
  virtual ~BehaviorDispatcherScoring();  

protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherScoring(const Json::Value& config);
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void InitDispatcher() override;  
  virtual void BehaviorDispatcher_OnDeactivated() override;
  
private:
  struct InstanceConfig {
    InstanceConfig();
    // indexes of scoringTracker correspond to the dispatchers stored in IBehaviorDispatcher
    std::vector<BehaviorScoringWrapper> scoringTracker;
  };

  struct DynamicVariables {
    DynamicVariables();
    BehaviorScoringWrapper* currentScoringTracker;  
    ICozmoBehaviorPtr       currentDispatch;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_BehaviorDispatcherScoring_H__
