/**
 * File: salientPointComponent.cpp
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
#include "engine/aiComponent/salientPointsComponent.h"
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
// SalientPointsComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsComponent::SalientPointsComponent()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::SalientPointsDetectorComponent)
, _robot(nullptr)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SalientPointsComponent::~SalientPointsComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsComponent::InitDependent(Robot* robot, const AICompMap& dependentComps)
{
  _robot = robot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsComponent::UpdateDependent(const AICompMap& dependentComps)
{
  // TODO: what goes here?
}

void SalientPointsComponent::AddSalientPoints(const std::list<Vision::SalientPoint>& c) {

  std::set<Vision::SalientPointType> erased; // which lists (associated with a type) have been erased already
  for (const auto& salientPoint : c) {

    // if we haven't already, erase the elements of that type in _salientPoints
    const auto type = salientPoint.salientType;
    const auto insertionResult = erased.insert(type);
    if (insertionResult.second) { // we hadn't erased this list yet
      std::list<Vision::SalientPoint> newlist = {salientPoint};
      _salientPoints[type] = std::move(newlist);
    }
    else {
      // the list in _salientPoints[type] had already been reset, just add
      std::list<Vision::SalientPoint>& prevList = _salientPoints[type];
      prevList.push_back(salientPoint);
    }
  }

}

void SalientPointsComponent::GetSalientPointSinceTime(std::list<Vision::SalientPoint>& salientPoints,
                                                      const Vision::SalientPointType& type,
                                                      const uint32_t timestamp) const
{

#if ANKI_DEVELOPER_CODE
  const size_t previousSize = salientPoints.size(); // used for debugging
#endif
  auto it = _salientPoints.find(type);
  if (it != _salientPoints.end()) {
    const std::list<Vision::SalientPoint>& oldPoints = it->second;
    std::copy_if(oldPoints.begin(), oldPoints.end(), std::back_inserter(salientPoints),
                 [timestamp](const Vision::SalientPoint& p) {return p.timestamp > timestamp;}
    );

  }

  PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.GetSalientPointSinceTime.CopiedElements",
                 "Number of salient points of type %s received since timestamp %u: %zu",
                 Vision::SalientPointTypeToString(type), timestamp,
                 salientPoints.size() - previousSize);

}

bool SalientPointsComponent::SalientPointDetected(const Vision::SalientPointType& type, const uint32_t timestamp) const
{

  if (kRandomPersonDetection) {
    // TODO Here I'm cheating: test if x seconds have passed and if so create a few fake salient points
    DEV_ASSERT(_robot != nullptr, "SalientPointsComponent.PersonDetected.RobotCantBeNull");
    float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ((currentTickCount - _timeSinceLastObservation) > 8) { // 8 seconds

      std::list<Vision::SalientPoint> pointsToAdd;
      auto addPoint = [this, &pointsToAdd](const Vision::SalientPointType& type) {
        Vision::SalientPoint salientPoint;

        salientPoint.timestamp = _robot->GetLastImageTimeStamp();
        salientPoint.salientType = type;

        // Random image coordinates
        salientPoint.y_img = float(_robot->GetContext()->GetRandom()->RandDbl());
        salientPoint.x_img = float(_robot->GetContext()->GetRandom()->RandDbl());

        // Random size and score
        salientPoint.area_fraction =  float(_robot->GetContext()->GetRandom()->RandDbl());
        salientPoint.score =  float(_robot->GetContext()->GetRandom()->RandDbl());

        pointsToAdd.push_back(std::move(salientPoint));
      };

      addPoint(Vision::SalientPointType::Person);
      addPoint(Vision::SalientPointType::Object);
      addPoint(Vision::SalientPointType::Object);
      addPoint(Vision::SalientPointType::Person);
      addPoint(Vision::SalientPointType::Person);
      // TODO DANGER THIS IS ONLY FOR TESTING AND WILL BE REMOVED SOON!
      const_cast<SalientPointsComponent*>(this)->AddSalientPoints(pointsToAdd);

      PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.SalientPointDetected.ConditionTrue", "");
      _timeSinceLastObservation = currentTickCount;
    } else {
      PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.SalientPointDetected.ConditionFalse", "");
    }
  }

  auto it = _salientPoints.find(type);
  // let's see if we have the right type
  if (it != _salientPoints.end()) {
    const auto& salientPointsList = it->second;
    // find a SalientPoint with a greater timestamp
    return std::any_of(salientPointsList.begin(), salientPointsList.end(),
                       [timestamp](const Vision::SalientPoint& p) {
                         return p.timestamp > timestamp;
                       }
    );
  }
  else {
    return false;
  }

}


} // namespace Cozmo
} // namespace Anki
