/**
* File: AsyncMessageGateComponent.h
*
* Author: Kevin Karol
* Date:   10/07/2017
*
* Description: Caches messages that come in asyncronously over the tick
* until they are requested syncronously
*
* Copyright: Anki, Inc. 2017
**/

#ifndef COZMO_ASYNC_MESSAGE_GATE_COMPONENT
#define COZMO_ASYNC_MESSAGE_GATE_COMPONENT


#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"

#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

class IBehavior;
class IExternalInterface;
class IGatewayInterface;
  
class AsyncMessageGateComponent : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  AsyncMessageGateComponent(IExternalInterface* externalInterface,
                            IGatewayInterface* gatewayInterface,
                            RobotInterface::MessageHandler* robotInterface);
  virtual ~AsyncMessageGateComponent() {};

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {}
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  virtual void InitDependent(Cozmo::Robot* robot, const BCCompMap& dependentComps) override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags);
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags);
  void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags);
  void SubscribeToTags(IBehavior* subscriber, std::set<AppToEngineTag>&& tags);
  
  // Function which sets up the cache of all messages that have come in since
  // the last time the cache was prepared
  void PrepareCache();
  
  void GetEventsForBehavior(IBehavior* subscriber, std::vector<const GameToEngineEvent>& events);
  void GetEventsForBehavior(IBehavior* subscriber, std::vector<const EngineToGameEvent>& events);
  void GetEventsForBehavior(IBehavior* subscriber, std::vector<const RobotToEngineEvent>& events);
  void GetEventsForBehavior(IBehavior* subscriber, std::vector<const AppToEngineEvent>& events);
  
  // Clear the messages out of the cache so that a new updates can be loaded in
  void ClearCache();
  
private:
  IExternalInterface* _externalInterface;
  IGatewayInterface* _gatewayInterface;
  RobotInterface::MessageHandler* _robotInterface;
  bool _isCacheValid;
  
  
  // Properties for tracking behavior message subscriptions
  // and the messages that come in each tick
  struct EventListWrapper{
    std::vector<int> _gameToEngineIdxs;
    std::vector<int> _engineToGameIdxs;
    std::vector<int> _robotToEngineIdxs;
    std::vector<int> _appToEngineIdxs;
  };
  
  struct EventTracker{
    // Track events that come in within the tick
    std::vector<const GameToEngineEvent>  _gameToEngineEvents;
    std::mutex _gameToEngineMutex;
    std::vector<const EngineToGameEvent>  _engineToGameEvents;
    std::mutex _engineToGameMutex;
    std::vector<const RobotToEngineEvent> _robotToEngineEvents;
    std::mutex _robotToEngineMutex;
    std::vector<const AppToEngineEvent> _appToEngineEvents;
    std::mutex _appToEngineMutex;
    
    // Only used by the cache instance of the tracker - allows faster access to events
    // as behavior indexed
    std::unordered_map<IBehavior*, EventListWrapper> _cacheMap;
  };
  
  
  std::unique_ptr<EventTracker> _cachedTracker;
  std::unique_ptr<EventTracker> _activeTracker;
  
  /////////
  // Tracking overall subscription maping/event handles
  ////////

  // Map tags to the behaviors that subscribe to them
  std::unordered_map<GameToEngineTag, std::set<IBehavior*>>                  _gameToEngineSubscribers;
  std::unordered_map<EngineToGameTag, std::set<IBehavior*>>                  _engineToGameSubscribers;
  std::unordered_map<RobotInterface::RobotToEngineTag, std::set<IBehavior*>> _robotToEngineSubscribers;
  std::unordered_map<AppToEngineTag, std::set<IBehavior*>>                   _appToEngineSubscribers;
  
  std::vector<Signal::SmartHandle> _eventHandles;
  
}; // class AsyncMessageGateComponent

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_ASYNC_MESSAGE_GATE_COMPONENT
