/**
 * File: behaviorThinkAboutBeacons
 *
 * Author: Raul
 * Created: 07/25/16
 *
 * Decription: behavior for when Cozmo needs to make decisions about AIbeacons. This allows playing animations or
 *  showing intent rather than if making the decision was just a module somewhere else in the AI.
 *
 * Copyright: Anki, Inc. 2016
 *
**/
#include "behaviorThinkAboutBeacons.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

static const char* kConfigParamsKey = "params";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorThinkAboutBeacons::BehaviorThinkAboutBeacons(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorThinkAboutBeacons");
  
  // load parameters from json
  LoadConfig(config[kConfigParamsKey]);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorThinkAboutBeacons::~BehaviorThinkAboutBeacons()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorThinkAboutBeacons::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // we want to think about beacons if we don't have any or we have finished the current one
  bool needsBeacon = true;
  
  // check current beacon
  const AIWhiteboard& whiteboard = preReqData.GetRobot().GetAIComponent().GetWhiteboard();
  const AIBeacon* activeBeacon = whiteboard.GetActiveBeacon();
  if ( nullptr != activeBeacon )
  {
    // TODO Check if done with this beacon, or maybe if we got far, or ...
    needsBeacon = false;
  }
  
  return needsBeacon;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorThinkAboutBeacons::InitInternal(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".InitInternal").c_str(), "Selecting new beacon");
  
  // select new beacon
  SelectNewBeacon(robot);
  
  // play animation since we have discovered a new area
  const std::string& animGroupName = _configParams.newAreaAnimTrigger;
  AnimationTrigger trigger = animGroupName.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animGroupName.c_str());
  if ( trigger != AnimationTrigger::Count )
  {
    IAction* animNewArea = new TriggerAnimationAction(robot,trigger);
    StartActing( animNewArea );
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetName() + ".BehaviorThinkAboutBeacons.LoadConfig";

  _configParams.newAreaAnimTrigger = ParseString(config, "newAreaAnimTrigger", debugName);
  _configParams.beaconRadius_mm = ParseFloat(config, "beaconRadius_mm", debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::SelectNewBeacon(Robot& robot)
{
  // TODO implement the real deal
   AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
   whiteboard.AddBeacon( robot.GetPose().GetWithRespectToOrigin(), _configParams.beaconRadius_mm );
}

} // namespace Cozmo
} // namespace Anki
