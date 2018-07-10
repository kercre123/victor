/**
 * File: salientPointDetectorComponent.cpp
 *
 * Author: Lorenzo Riano
 * Created: 06/04/18
 *
 * Description: A component for salient points. Listens to messages and categorizes them based on their type
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/salientPointsDetectorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/random/randomGenerator.h"


namespace Anki {
namespace Cozmo {

namespace {
const bool kRandomPersonDetection = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SalientPointsDetectorComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsDetectorComponent::SalientPointsDetectorComponent()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::SalientPointsDetectorComponent)
{
  _robot = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsDetectorComponent::~SalientPointsDetectorComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsDetectorComponent::InitDependent(Robot* robot, const AICompMap& dependentComps)
{
  _robot = robot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsDetectorComponent::UpdateDependent(const AICompMap& dependentComps)
{
  // TODO: what goes here?
}

bool SalientPointsDetectorComponent::PersonDetected() const
{
  if (kRandomPersonDetection) {
    // TODO Here I'm cheating: test if x seconds have passed and if so create a fake salient point with a person
    _latestSalientPoints.clear();
    DEV_ASSERT(_robot != nullptr, "SalientPointsDetectorComponent.PersonDetected.RobotCantBeNull");
    float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ((currentTickCount - _timeSinceLastObservation) > 3) { // 3 seconds


      Vision::SalientPoint personDetected;

      personDetected.timestamp = _robot->GetLastImageTimeStamp();
      personDetected.salientType = Vision::SalientPointType::Person;

      // Random image coordinates
      personDetected.y_img = float(_robot->GetContext()->GetRandom()->RandDbl());
      personDetected.x_img = float(_robot->GetContext()->GetRandom()->RandDbl());

      _latestSalientPoints.push_back(std::move(personDetected));

      PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.SalientPointDetected.ConditionTrue", "");
      _timeSinceLastObservation = currentTickCount;
    } else {
      PRINT_CH_DEBUG("Behaviors", "SalientPointsDetectorComponent.SalientPointDetected.ConditionFalse", "");
    }
  }

  return std::any_of(_latestSalientPoints.begin(), _latestSalientPoints.end(),
                     [](const Vision::SalientPoint& p) { return p.salientType == Vision::SalientPointType::Person; }
  );
}


} // namespace Cozmo
} // namespace Anki
