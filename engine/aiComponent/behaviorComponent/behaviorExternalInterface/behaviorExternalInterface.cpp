/**
* File: behaviorExternalInterface.cpp
*
* Author: Kevin M. Karol
* Created: 08/30/17
*
* Description: Interface that behaviors use to interact with the rest of
* the Cozmo system
*
* Copyright: Anki, Inc. 2017
*
**/


#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::BehaviorExternalInterface()
: _delegationComponent(nullptr)
, _moodManager(nullptr)
, _needsManager(nullptr)
, _progressionUnlockComponent(nullptr)
, _publicStateBroadcaster(nullptr)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::Init(Robot& robot,
        AIComponent& aiComponent,
        const BehaviorContainer& behaviorContainer,
        BlockWorld& blockWorld,
        FaceWorld& faceWorld,
        BehaviorEventComponent& behaviorEventComponent)
{
  _beiComponents = std::make_unique<ComponentWrappers::BehaviorExternalInterfaceComponents>(
                                    robot, aiComponent, behaviorContainer,
                                    blockWorld, faceWorld, behaviorEventComponent);
  
  SetOptionalInterfaces(nullptr,
                        &robot.GetMoodManager(),
                        robot.GetContext()->GetNeedsManager(),
                        &robot.GetProgressionUnlockComponent(),
                        &robot.GetPublicStateBroadcaster());
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::SetOptionalInterfaces(DelegationComponent* delegationComponent,
                                                      MoodManager* moodManager,
                                                      NeedsManager* needsManager,
                                                      ProgressionUnlockComponent* progressionUnlockComponent,
                                                      PublicStateBroadcaster* publicStateBroadcaster)
{


  _delegationComponent        = delegationComponent;
  _moodManager                = moodManager;
  _needsManager               = needsManager;
  _progressionUnlockComponent = progressionUnlockComponent;
  _publicStateBroadcaster     = publicStateBroadcaster;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BehaviorExternalInterface::GetOffTreadsState() const
{
  assert(_beiComponents);
  return _beiComponents->_robot.GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BehaviorExternalInterface::GetRNG()
{
  assert(_beiComponents);
  return _beiComponents->_robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
