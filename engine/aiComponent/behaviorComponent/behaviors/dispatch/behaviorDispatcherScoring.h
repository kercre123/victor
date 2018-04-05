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
  

  virtual ICozmoBehaviorPtr GetDesiredBehavior() override;
  virtual void InitDispatcher() override;  
  virtual void BehaviorDispatcher_OnDeactivated() override;
  
private:
  // indexes of scoringTracker correspond to the dispatchers stored in IBehaviorDispatcher
  std::vector<BehaviorScoringWrapper> _scoringTracker;
  ICozmoBehaviorPtr _currentDispatch;
  BehaviorScoringWrapper* _currentScoringTracker = nullptr;  
};

}
}

#endif
