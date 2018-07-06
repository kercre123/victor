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

  return Detected(Vision::SalientPointType::Person);
}

bool SalientPointsDetectorComponent::ObjectDetected() const
{
  PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.ObjectDetected.Check", "");
  if (kRandomPersonDetection) {
    // TODO Here I'm cheating: test if x seconds have passed and if so create a fake salient point with a person
    _latestSalientPoints.clear();
    DEV_ASSERT(_robot != nullptr, "SalientPointsDetectorComponent.ObjectDetected.RobotCantBeNull");
    float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ((currentTickCount - _timeSinceLastObservation) > 3)
    {
      Vision::SalientPoint objectDetected;

      objectDetected.timestamp = _robot->GetLastImageTimeStamp();
      objectDetected.salientType = Vision::SalientPointType::Object;

      // Random image coordinates
      objectDetected.y_img = float(_robot->GetContext()->GetRandom()->RandDbl());
      objectDetected.x_img = float(_robot->GetContext()->GetRandom()->RandDbl());

      _latestSalientPoints.push_back(std::move(objectDetected));

      PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.SalientPointDetected.ConditionTrue", "");
      _timeSinceLastObservation = currentTickCount;
    } 
    else
    {
      PRINT_CH_DEBUG("Behaviors", "SalientPointsDetectorComponent.SalientPointDetected.ConditionFalse", "");
    }
  }

  return Detected(Vision::SalientPointType::Object);
}

bool SalientPointsDetectorComponent::Detected(const Vision::SalientPointType type) const
{
  if (type == Vision::SalientPointType::Object)
  {
    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.Detected", "Trying see if an object was detected");
  }
  else
  {
    PRINT_CH_INFO("Behaviors", "SalientPointsDetectorComponent.Detected", "Trying see if a person was detected");
  }
  return std::any_of(_latestSalientPoints.begin(), _latestSalientPoints.end(),
                     [type](const Vision::SalientPoint& p) { return p.salientType == type; }
  );
}

} // namespace Cozmo
} // namespace Anki
