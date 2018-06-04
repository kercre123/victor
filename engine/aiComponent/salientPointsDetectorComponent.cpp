/**
 * File: PersonDetectorComponent
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/salientPointsDetectorComponent.h"

#include "coretech/common/engine/math/poseOriginList.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS 0

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SalientPointsDetectorComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsDetectorComponent::SalientPointsDetectorComponent(Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::SalientPointsDetectorComponent)
, _robot(robot)

{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsDetectorComponent::~SalientPointsDetectorComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsDetectorComponent::InitDependent(Cozmo::Robot* robot, const AICompMap& dependentComps)
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
//    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    // TODO Subscribe to the messages we need
//    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
//    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
//    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotOffTreadsStateChanged>();
  }
  else {
    PRINT_NAMED_WARNING("SalientPointsDetectorComponent.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsDetectorComponent::UpdateDependent(const AICompMap& dependentComps)
{
  // TODO: what goes here?
}

bool SalientPointsDetectorComponent::PersonDetected() const
{
  // TODO Here I'm cheating: test if x seconds have passed and if so make the condition true
  float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((currentTickCount - _timeSinceLastObservation) > 3 ) { // 3 seconds
    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.PersonDetected.ConditionTrue", "");
    _timeSinceLastObservation = currentTickCount;
    return true;
  }
  else {
    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.PersonDetected.ConditionFalse", "");
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void SalientPointsDetectorComponent::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void SalientPointsDetectorComponent::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void SalientPointsDetectorComponent::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg)
{

}


} // namespace Cozmo
} // namespace Anki
