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
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorThinkAboutBeacons.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/events/animationTriggerHelpers.h"

#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* const kNewAreaAnimTriggerKey = "newAreaAnimTrigger";
const char* const kBeaconRadiusKey = "beaconRadius_mm";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorThinkAboutBeacons::BehaviorThinkAboutBeacons(const Json::Value& config)
: ICozmoBehavior(config)
{  
  // load parameters from json
  LoadConfig(config);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorThinkAboutBeacons::~BehaviorThinkAboutBeacons()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kNewAreaAnimTriggerKey,
    kBeaconRadiusKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorThinkAboutBeacons::WantsToBeActivatedBehavior() const
{
  // we want to think about beacons if we don't have any or we have finished the current one
  bool needsBeacon = true;
  
  // check current beacon
  const AIWhiteboard& whiteboard = GetAIComp<AIWhiteboard>();
  const AIBeacon* activeBeacon = whiteboard.GetActiveBeacon();
  if ( nullptr != activeBeacon )
  {
    // TODO Check if done with this beacon, or maybe if we got far, or ...
    needsBeacon = false;
  }
  
  return needsBeacon;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::OnBehaviorActivated()
{
  PRINT_CH_INFO("Behaviors", (std::string(GetDebugLabel()) + ".InitInternal").c_str(), "Selecting new beacon");
  
  // select new beacon
  SelectNewBeacon();
  
  // play animation since we have discovered a new area
  AnimationTrigger trigger = _configParams.newAreaAnimTrigger;
  if ( trigger != AnimationTrigger::Count )
  {    
    IAction* animNewArea = new TriggerAnimationAction(trigger);
    DelegateIfInControl( animNewArea );
  }  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = std::string(GetDebugLabel()) + ".BehaviorThinkAboutBeacons.LoadConfig";

  _configParams.newAreaAnimTrigger = ParseAnimationTrigger(config, kNewAreaAnimTriggerKey, debugName);
  _configParams.beaconRadius_mm = ParseFloat(config, kBeaconRadiusKey, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorThinkAboutBeacons::SelectNewBeacon()
{
  // TODO implement the real deal
   AIWhiteboard& whiteboard = GetAIComp<AIWhiteboard>();
  whiteboard.AddBeacon( GetBEI().GetRobotInfo().GetPose().GetWithRespectToRoot(), _configParams.beaconRadius_mm );
}

} // namespace Cozmo
} // namespace Anki
