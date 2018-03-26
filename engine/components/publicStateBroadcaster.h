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
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override {
    _robot = robot;
  };
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    // probably not ALL necessary, but theoretically public state could be broadcast about any/all of these
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::FaceWorld);
    dependencies.insert(RobotComponentID::PetWorld);
    dependencies.insert(RobotComponentID::EngineAudioClient);
    dependencies.insert(RobotComponentID::PathPlanning);
    dependencies.insert(RobotComponentID::DrivingAnimationHandler);
    dependencies.insert(RobotComponentID::ActionList);
    dependencies.insert(RobotComponentID::Movement);
    dependencies.insert(RobotComponentID::Vision);
    dependencies.insert(RobotComponentID::VisionScheduleMediator);
    dependencies.insert(RobotComponentID::Map);
    dependencies.insert(RobotComponentID::NVStorage);
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::ObjectPoseConfirmer);
    dependencies.insert(RobotComponentID::CubeLights);
    dependencies.insert(RobotComponentID::BodyLights);
    dependencies.insert(RobotComponentID::CubeAccel);
    dependencies.insert(RobotComponentID::CubeComms);
    dependencies.insert(RobotComponentID::GyroDriftDetector);
    dependencies.insert(RobotComponentID::Docking);
    dependencies.insert(RobotComponentID::Carrying);
    dependencies.insert(RobotComponentID::CliffSensor);
    dependencies.insert(RobotComponentID::ProxSensor);
    dependencies.insert(RobotComponentID::Animation);
    dependencies.insert(RobotComponentID::TouchSensor);
    dependencies.insert(RobotComponentID::StateHistory);
    dependencies.insert(RobotComponentID::MoodManager);
    dependencies.insert(RobotComponentID::Inventory);
    dependencies.insert(RobotComponentID::ProgressionUnlock);
    dependencies.insert(RobotComponentID::BlockTapFilter);
    dependencies.insert(RobotComponentID::RobotToEngineImplMessaging);
    dependencies.insert(RobotComponentID::RobotIdleTimeout);
    dependencies.insert(RobotComponentID::MicDirectionHistory);
  };
  
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  //////
  // end IDependencyManagedComponent functions
  //////
  
  using SubscribeFunc = std::function<void(const AnkiEvent<RobotPublicState>&)>;
  Signal::SmartHandle Subscribe(SubscribeFunc messageHandler);
  
  static int GetBehaviorRoundFromMessage(const RobotPublicState& stateEvent);
  
  
  void Update(Robot& robot);
  void UpdateBroadcastBehaviorStage(BehaviorStageTag stageType, uint8_t stage);
  
  void UpdateRequestingGame(bool isRequesting);
  
private:
  Robot* _robot = nullptr;
  std::unique_ptr<RobotPublicState> _currentState;
  AnkiEventMgr<RobotPublicState> _eventMgr;
  
  void SendUpdatedState();
  static int GetStageForBehaviorStageType(BehaviorStageTag stageType,
                                          const BehaviorStageStruct& stageStruct);
  
  
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Components_PublicStateBroadcaster_H__
