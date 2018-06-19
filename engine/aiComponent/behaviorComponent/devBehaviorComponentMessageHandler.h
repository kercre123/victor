/**
* File: devBehaviorComponentMessageHandler.h
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


#ifndef __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__
#define __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"

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
class BehaviorSystemManager;

class DevBehaviorComponentMessageHandler : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override;
  virtual void AdditionalInitAccessibleComponents(BCCompIDSet& components) const override;
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};

public:
  DevBehaviorComponentMessageHandler(Robot& robot);
  virtual ~DevBehaviorComponentMessageHandler();
  
private:
  Robot& _robot;
  std::vector<::Signal::SmartHandle> _eventHandles;
  ICozmoBehaviorPtr _rerunBehavior;

  void SetupUserIntentEvents();
  
  ICozmoBehaviorPtr WrapRequestedBehaviorInDispatcherRerun(BehaviorContainer& bContainer, 
                                                           BehaviorID requestedBehaviorID, 
                                                           const int numRuns);
  
  // subscribes to webviz OnSubscribed and OnData
  void SubscribeToWebViz(BehaviorExternalInterface& bei, const BehaviorSystemManager& bsm);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__
