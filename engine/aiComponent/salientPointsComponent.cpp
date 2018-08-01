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
void SalientPointsComponent::InitDependent(Robot* robot, const AICompMap& dependentComps)
{
  _robot = robot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SalientPointsComponent::AddSalientPoints(const std::list<Vision::SalientPoint>& c) {

  PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.AddSalientPoints.PointsReceived",
                 "Number of salient point received: %zu, previous size: %zu", c.size(), _salientPoints.size());

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

  PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.AddSalientPoints.NewPointsSize",
                 "Number of salient point after adding: %zu", _salientPoints.size());

#if ANKI_DEVELOPER_CODE
  for (auto &element : _salientPoints) {
    for (auto &salientPoint : element.second) {
      PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.AddSalientPoints.PointInfo",
                     "Timestamp of SalientPoint: %u", salientPoint.timestamp);
    }
  }
#endif

}

void SalientPointsComponent::GetSalientPointSinceTime(std::list<Vision::SalientPoint>& salientPoints,
                                                      const Vision::SalientPointType& type,
                                                      const RobotTimeStamp_t timestamp) const
{

  ANKI_DEVELOPER_CODE_ONLY(const size_t previousSize = salientPoints.size();) // used for debugging

  auto it = _salientPoints.find(type);
  if (it != _salientPoints.end()) {
    const std::list<Vision::SalientPoint>& oldPoints = it->second;
    std::copy_if(oldPoints.begin(), oldPoints.end(), std::back_inserter(salientPoints),
                 [timestamp](const Vision::SalientPoint& p) {return p.timestamp > timestamp;}
    );

  }

  PRINT_CH_DEBUG("Behaviors", "SalientPointsComponent.GetSalientPointSinceTime.CopiedElements",
                 "Number of salient points of type %s received since timestamp %u: %zu",
                 Vision::SalientPointTypeToString(type), (TimeStamp_t)timestamp,
                 salientPoints.size() - previousSize);


}

bool SalientPointsComponent::SalientPointDetected(const Vision::SalientPointType& type, const RobotTimeStamp_t timestamp) const
{

  if (kRandomPersonDetection) {
    // TODO Here I'm cheating: test if x seconds have passed and if so create a few fake salient points
    PRINT_NAMED_WARNING("SalientPointsComponent.SalientPointDetected.Cheating","");
    DEV_ASSERT(_robot != nullptr, "SalientPointsComponent.PersonDetected.RobotCantBeNull");
    float currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ((currentTickCount - _timeSinceLastObservation) > 8) { // 8 seconds

      std::list<Vision::SalientPoint> pointsToAdd;
      auto addPoint = [this, &pointsToAdd](const Vision::SalientPointType& type) {
        Vision::SalientPoint salientPoint;

        salientPoint.timestamp = (TimeStamp_t) _robot->GetLastImageTimeStamp();
        salientPoint.salientType = type;

        // Random image coordinates
        Util::RandomGenerator *rng = _robot->GetContext()->GetRandom();
        salientPoint.y_img = float(rng->RandDbl());
        salientPoint.x_img = float(rng->RandDbl());

        // Random size and score
        salientPoint.area_fraction =  float(rng->RandDbl());
        salientPoint.score =  float(rng->RandDbl());
        
        // Random shape, 4 points
        {
          const float minX = std::max(float(salientPoint.x_img - rng->RandDbl()), 0.0f);
          const float maxX = std::min(float(salientPoint.x_img + rng->RandDbl()), 1.0f);
          const float minY = std::max(float(salientPoint.y_img - rng->RandDbl()), 0.0f);
          const float maxY = std::min(float(salientPoint.y_img + rng->RandDbl()), 1.0f);

          salientPoint.shape = { CladPoint2d(minX, minY),
                                 CladPoint2d(maxX, minY),
                                 CladPoint2d(minX, maxY),
                                 CladPoint2d(maxX, maxY)};

        }

        pointsToAdd.push_back(std::move(salientPoint));
      };

      addPoint(Vision::SalientPointType::Person);
      addPoint(Vision::SalientPointType::Object);
      addPoint(Vision::SalientPointType::Object);
      addPoint(Vision::SalientPointType::Person);
      addPoint(Vision::SalientPointType::Person);
      // TODO DANGER THIS IS ONLY FOR TESTING AND WILL BE REMOVED SOON!
      const_cast<SalientPointsComponent*>(this)->AddSalientPoints(pointsToAdd);

      _timeSinceLastObservation = currentTickCount;
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
