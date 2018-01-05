/**
 * File: behaviorRndAudioDemo.cpp
 *
 * Author: Matt Michini
 * Created: 1/4/18
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRndAudioDemo.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/chargerActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRndAudioDemo::BehaviorRndAudioDemo(const Json::Value& config)
  : ICozmoBehavior(config)
{

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRndAudioDemo::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  
  // maybe only return true if we know about all the cubes
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRndAudioDemo::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_NAMED_WARNING("BehaviorRndAudioDemo", "started");

//  auto action = new DriveToAndMountChargerAction(object->GetID());
//  DelegateIfInControl(action);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRndAudioDemo::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{

}

  
} // namespace Cozmo
} // namespace Anki
