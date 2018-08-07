/**
* File: behaviorComponentMessageHandler.h
*
* Author: Kevin M. Karol
* Created: 10/24/17
*
* Description: Component that receives dev messages and performs actions on
* the behaviorComponent in response
*
* Copyright: Anki, Inc. 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorComponentMessageHandler_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorComponentMessageHandler_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// forward declarations
class Robot;
class BehaviorComponent;
class BehaviorContainer;
class BehaviorsBootLoader;
class BehaviorSystemManager;
class UserIntentComponent;

class BehaviorComponentMessageHandler : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override;
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};

public:
  BehaviorComponentMessageHandler(Robot& robot);
  virtual ~BehaviorComponentMessageHandler();
  
private:
  Robot& _robot;
  std::vector<::Signal::SmartHandle> _eventHandles;
  ICozmoBehaviorPtr _rerunBehavior;
  size_t _tickInfoScreenEnded; // ignored if 0
  
  void OnEnterInfoFace( BehaviorContainer& bContainer, BehaviorSystemManager& bsm );
  
  void OnExitInfoFace( BehaviorSystemManager& bsm, BehaviorsBootLoader& bbl, UserIntentComponent& uic );

  void SetupUserIntentEvents();
  
  ICozmoBehaviorPtr WrapRequestedBehaviorInDispatcherRerun(BehaviorContainer& bContainer, 
                                                           BehaviorID requestedBehaviorID, 
                                                           const int numRuns);
  
  // subscribes to webviz OnSubscribed and OnData
  void SubscribeToWebViz(BehaviorExternalInterface& bei, const BehaviorSystemManager& bsm);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorComponentMessageHandler_H__
