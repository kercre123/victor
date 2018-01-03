/**
 * File: behaviorGoHome.cpp
 *
 * Author: Matt Michini
 * Created: 11/1/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorGoHome.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/chargerActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGoHome::BehaviorGoHome(const Json::Value& config)
  : ICozmoBehavior(config)
  , _homeFilter(std::make_unique<BlockWorldFilter>())
{
  // Set up block world filter for finding Home object
  _homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGoHome::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // If we have a located Home/charger, then we can be activated.
  
  std::vector<const ObservableObject*> locatedHomes;
  behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(*_homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  return hasAHome;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
  const auto* object = behaviorExternalInterface.GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_WARNING("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return;
  }
  
  auto action = new DriveToAndMountChargerAction(object->GetID());
  DelegateIfInControl(action);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{

}

  
} // namespace Cozmo
} // namespace Anki
