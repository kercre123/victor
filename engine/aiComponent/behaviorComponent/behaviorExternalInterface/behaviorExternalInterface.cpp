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
BehaviorExternalInterface::BehaviorExternalInterface(Robot& robot,
                                                     AIComponent& aiComponent,
                                                     const BehaviorContainer& behaviorContainer,
                                                     BlockWorld& blockWorld,
                                                     FaceWorld& faceWorld,
                                                     StateChangeComponent& stateChangeComponent)
: _robot(robot)
, _aiComponent(aiComponent)
, _behaviorContainer(behaviorContainer)
, _faceWorld(faceWorld)
, _blockWorld(blockWorld)
,_stateChangeComponent(stateChangeComponent)
{
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
  // This is kindof a dirty hack and deserves an explanation
  // Robot doesn't store what the BehaviorExternalInterface considers
  // optional interfaces in a consistent fashion - some are pointers, some are
  // references, none are shared ptrs - however using shared and weak ptrs is
  // a great way to force parts of the behavior system to check whether a component
  // is currently included/allowed to be used - to allow this shared pointers
  // are created/stored in this class - problem is, when those shared pointers
  // are reset, part of robot gets deleted because the last refernce fell out of scope
  // Solution: pass null deleters into our fake shared pointers so that when they
  // are destroyed robot is unaffected
  
  auto delegationNullDeleter = [](DelegationComponent *) {};
  auto moodNullDeleter = [](MoodManager *) {};
  auto needsNullDeleter = [](NeedsManager *) {};
  auto progNullDeleter = [](ProgressionUnlockComponent *) {};
  auto pubStateNullDeleter = [](PublicStateBroadcaster *) {};

  _delegationComponent        = std::shared_ptr<DelegationComponent>(delegationComponent, delegationNullDeleter);
  _moodManager                = std::shared_ptr<MoodManager>(moodManager, moodNullDeleter);
  _needsManager               = std::shared_ptr<NeedsManager>(needsManager, needsNullDeleter);
  _progressionUnlockComponent = std::shared_ptr<ProgressionUnlockComponent>(progressionUnlockComponent, progNullDeleter);
  _publicStateBroadcaster     = std::shared_ptr<PublicStateBroadcaster>(publicStateBroadcaster, pubStateNullDeleter);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BehaviorExternalInterface::GetOffTreadsState() const
{
  return _robot.GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BehaviorExternalInterface::GetRNG()
{
  return _robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
