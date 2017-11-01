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
#include "engine/blockWorld/blockWorld.h"
#include "engine/robot.h"

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
Result BehaviorGoHome::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& robotPose = behaviorExternalInterface.GetRobot().GetPose();
  const auto* object = behaviorExternalInterface.GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_homeFilter);
  
  if (object == nullptr) {
    PRINT_NAMED_WARNING("BehaviorGoHome.OnBehaviorActivated", "No homes found!");
    return Result::RESULT_FAIL;
  }
  
  auto action = new DriveToAndMountChargerAction(behaviorExternalInterface.GetRobot(),
                                                 object->GetID());
  const bool success = DelegateIfInControl(action);
  
  return success ? Result::RESULT_OK : Result::RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGoHome::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{

}

  
} // namespace Cozmo
} // namespace Anki
