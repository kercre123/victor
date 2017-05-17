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

#include "anki/cozmo/basestation/events/ankiEventMgr.h"

#include "clad/types/robotPublicState.h"

#include "util/helpers/noncopyable.h"

#include <memory>

namespace Anki {
namespace Cozmo {
  
class Robot;

class PublicStateBroadcaster : private Util::noncopyable
{
public:
  PublicStateBroadcaster();
  ~PublicStateBroadcaster() {};
  
  using SubscribeFunc = std::function<void(const AnkiEvent<RobotPublicState>&)>;
  Signal::SmartHandle Subscribe(SubscribeFunc messageHandler);
  
  static int GetBehaviorRoundFromMessage(const RobotPublicState& stateEvent);
  
  
  void Update(Robot& robot);
  void UpdateActivity(ActivityID activityID);
  void UpdateBroadcastBehaviorStage(BehaviorStageTag stageType, uint8_t stage);
  
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
