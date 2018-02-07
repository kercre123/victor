/**
 * File: publicStateBroadcaster.h
 *
 * Author: Kevin M. Karol
 * Created: 4/5/2017
 *
 * Description: Tracks state information about the robot/current behavior/game
 * that engine wants to expose to other parts of the system (music, UI, etc).
 * Full state information is broadcast every time any piece of tracked state changes.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_PublicStateBroadcaster_H__
#define __Anki_Cozmo_Basestation_Components_PublicStateBroadcaster_H__

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/events/ankiEventMgr.h"
#include "engine/robotComponents_fwd.h"

#include "clad/types/robotPublicState.h"

#include "util/helpers/noncopyable.h"

#include <memory>

namespace Anki {
namespace Cozmo {
  
class Robot;

class PublicStateBroadcaster : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  PublicStateBroadcaster();
  ~PublicStateBroadcaster() {};

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override {};
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::PetWorld);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  using SubscribeFunc = std::function<void(const AnkiEvent<RobotPublicState>&)>;
  Signal::SmartHandle Subscribe(SubscribeFunc messageHandler);
  
  static int GetBehaviorRoundFromMessage(const RobotPublicState& stateEvent);
  
  
  void Update(Robot& robot);
  void UpdateBroadcastBehaviorStage(BehaviorStageTag stageType, uint8_t stage);
  
  void UpdateRequestingGame(bool isRequesting);
  void NotifyBroadcasterOfConfigurationManagerUpdate(const Robot& robot);
  
private:
  std::unique_ptr<RobotPublicState> _currentState;
  AnkiEventMgr<RobotPublicState> _eventMgr;
  
  void SendUpdatedState();
  static int GetStageForBehaviorStageType(BehaviorStageTag stageType,
                                          const BehaviorStageStruct& stageStruct);
  
  
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Components_PublicStateBroadcaster_H__
