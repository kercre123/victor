/**
 * File: salientPointDetectorComponent.cpp
 *
 * Author: Lorenzo Riano
 * Created: 06/04/18
 *
 * Description: A component for salient points. Listens to messages and categorize them based on their type
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/salientPointsDetectorComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"


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
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedSalientPoint>();
    // TODO what happens when helper goes out of scope here?
  }
  else {
    PRINT_NAMED_WARNING("SalientPointsDetectorComponent.Init", "Initialized component with no external interface. "
        "Will miss events.");
  }
}

template<>
void SalientPointsDetectorComponent::HandleMessage(const ExternalInterface::RobotObservedSalientPoint& msg)
{

    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.HandleMessage.NewMessageReceived", "");
    const auto& salientPoint = msg.salientPoint;

    if (salientPoint.salientType == Vision::SalientPointType::Person ) {
      _lastPersonDetected = salientPoint;
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

    _lastPersonDetected.timestamp = _robot.GetLastImageTimeStamp();

    // Random image coordinates
    _lastPersonDetected.y_img = float(_robot.GetContext()->GetRandom()->RandDbl());
    _lastPersonDetected.x_img = float(_robot.GetContext()->GetRandom()->RandDbl());

    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.PersonDetected.ConditionTrue", "");
    _timeSinceLastObservation = currentTickCount;
    return true;
  }
  else {
    PRINT_CH_DEBUG("Behaviors", "SalientPointsDetectorComponent.PersonDetected.ConditionFalse", "");
    return false;
  }

  // TODO Here just check if a person has been detected, e.g. check that _lastPersonDetected is not empty
}

void SalientPointsDetectorComponent::GetLastPersonDetectedData(Vision::SalientPoint& salientPoint) const
{
  salientPoint = _lastPersonDetected;

  // reset the person data, it's been "consumed"
  _lastPersonDetected = Vision::SalientPoint();

}



} // namespace Cozmo
} // namespace Anki
