/**
 * File: behaviorReactToBody.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2018-05-30
 *
 * Description: Turns towards a body if detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorReactToBody.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/faceWorld.h"


namespace Anki {
namespace Cozmo {

namespace {
  const char* const kDriveOffChargerBehaviorKey        = "driveOffChargerBehavior";
  const char* const kShouldDriveStraightIfNotOnCharger = "shouldDriveStraightIfNotOnCharger";
  const char* const kDrivingForwadDistance             = "drivingForwadDistance_mm";
  const char* const kUpperPortionLookUpPercent         = "upperPortionLookUpPercent";
  const char* const kTrackingTimeout                   = "trackingTimeout_sec";
  const char* const kFaceSelectionPenaltiesKey         = "faceSelectionPenalties";
}

#define LOG_CHANNEL "Behaviors"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBody::InstanceConfig::InstanceConfig(const Json::Value& config)
{

  const std::string& debugName = "BehaviorReactToBody.InstanceConfig.LoadConfig";
  moveOffChargerBehaviorStr = JsonTools::ParseString(config, kDriveOffChargerBehaviorKey,
                                                     debugName);
  shouldDriveStraightWhenBodyDetected = JsonTools::ParseBool(config, kShouldDriveStraightIfNotOnCharger,
                                                             debugName);
  drivingForwardDistance = JsonTools::ParseFloat(config, kDrivingForwadDistance,
                                                 debugName);
  upperPortionLookUpPercent = JsonTools::ParseFloat(config, kUpperPortionLookUpPercent,
                                                    debugName);
  upperPortionLookUpPercent = Util::Clamp(upperPortionLookUpPercent, 0.0f, 1.0f);
  trackingTimeout = JsonTools::ParseFloat(config, kTrackingTimeout,
                                          debugName);
  const bool parsedOk = FaceSelectionComponent::ParseFaceSelectionFactorMap(config[kFaceSelectionPenaltiesKey],
                                                                            faceSelectionCriteria);
  DEV_ASSERT(parsedOk, "BehaviorReactToBody.InstanceConfig.InstanceConfig.MissingFaceSelectionPenalties");

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBody::DynamicVariables::DynamicVariables()
{
  const bool kKeepPersistent = false;
  Reset(kKeepPersistent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::DynamicVariables::Reset(bool keepPersistent)
{
  lastPersonDetected = Vision::SalientPoint();
  searchingForFaces = false;
  imageTimestampWhenActivated = 0;
  droveOffCharger = false;
  actingOnFaceFound = false;
  if (! keepPersistent) {
    persistent.lastSeenTimeStamp = 0;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBody::BehaviorReactToBody(const Json::Value& config)
 : ICozmoBehavior(config), _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBody::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.moveOffChargerBehavior = BC.FindBehaviorByID(
      BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.moveOffChargerBehaviorStr));
  DEV_ASSERT(_iConfig.moveOffChargerBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullMoveOffChargerBehavior");

  // Need to enforce the behavior type here, using SearchWithinBoundingBox
  const bool result = BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SearchWithinBoundingBox),
                                                     BEHAVIOR_CLASS(SearchWithinBoundingBox),
                                                     _iConfig.searchBehavior);
  DEV_ASSERT(result, "BehaviorSearchWithinBoundingBoxNotFound");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = true;

  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.moveOffChargerBehavior.get());
  delegates.insert(_iConfig.searchBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kDriveOffChargerBehaviorKey,
      kShouldDriveStraightIfNotOnCharger,
      kDrivingForwadDistance,
      kUpperPortionLookUpPercent,
      kTrackingTimeout,
      kFaceSelectionPenaltiesKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::OnBehaviorActivated()
{
  // reset dynamic variables
  const bool kKeepPersistent = true;
  _dVars.Reset(kKeepPersistent);

  LOG_DEBUG( "BehaviorReactToPersonDetected.OnBehaviorActivated", "I am active!");

  // don't look for faces in the past
  _dVars.imageTimestampWhenActivated = GetBEI().GetRobotInfo().GetLastImageTimeStamp();

  // no need to start a ruckus if we already see a face
  const bool faceFound = GetBEI().GetFaceWorld().HasAnyFaces(_dVars.imageTimestampWhenActivated);
  if (faceFound) {
    LOG_DEBUG("BehaviorReactToPersonDetected.OnBehaviorActivated.FaceFound", "Face found, no need to search");
    TransitionToFaceFound();
    return;
  }

  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestPersons;

  //Get all the latest persons
  component.GetSalientPointSinceTime(latestPersons, Vision::SalientPointType::Person,
                                     _dVars.persistent.lastSeenTimeStamp);

  if (latestPersons.empty()) {
    LOG_DEBUG( "BehaviorReactToPersonDetected.OnBehaviorActivated.NoPersonDetected",
        "Activated but no person with a timestamp > %u", (TimeStamp_t)_dVars.persistent.lastSeenTimeStamp);
    TransitionToCompleted();
    return;
  }

  // Select the best one
  // Choose the latest one, or the biggest if tie
  auto maxElementIter = std::max_element(latestPersons.begin(), latestPersons.end(),
                                         [](const Vision::SalientPoint& p1, const Vision::SalientPoint& p2) {
                                           if (p1.timestamp == p2.timestamp) {
                                             return p1.area_fraction < p2.area_fraction;
                                           }
                                           else {
                                             return  p1.timestamp < p2.timestamp;
                                           }
                                         }
  );
  _dVars.lastPersonDetected = *maxElementIter;
  _dVars.persistent.lastSeenTimeStamp = _dVars.lastPersonDetected.timestamp;

  DEV_ASSERT(_dVars.lastPersonDetected.salientType == Vision::SalientPointType::Person,
             "BehaviorReactToPersonDetected.OnBehaviorActivated.LastSalientPointMustBePerson");

  // Action!
  TransitionToStart();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBody::CheckIfShouldStop()
{

  // pickup
  {
    const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

    if (pickedUp) {
      LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate.PickedUp",
                "Robot has been picked up");
      return true;
    }
  }

  // cliff
  {
    const CliffSensorComponent& cliff = GetBEI().GetCliffSensorComponent();
    if (cliff.IsCliffDetected()) {
      LOG_DEBUG("BehaviorReactToPersonDetected.BehaviorUpdate.CliffSensed",
               "There's a cliff, not moving");
      return true;
    }
  }

  // note: if an obstacle is in front of the robot, it can still look for a face
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::BehaviorUpdate()
{

  if (CheckIfShouldStop()) {
    TransitionToCompleted();
  }


  if( ! IsActivated() || ! IsControlDelegated()) {
    return;
  }

  if (_dVars.actingOnFaceFound) {
    // doing something with a found face, nothing to do here
    return;
  }

  if (_dVars.searchingForFaces) {
    const bool faceFound = GetBEI().GetFaceWorld().HasAnyFaces(_dVars.imageTimestampWhenActivated);
    if (faceFound) {
      LOG_DEBUG("BehaviorReactToPersonDetected.BehaviorUpdate.FaceFound", "Face found, cancelling search");
      CancelDelegates(false);
      TransitionToFaceFound();
      return;
    }

    if (! _iConfig.searchBehavior->IsActivated()) {
      LOG_DEBUG("BehaviorReactToPersonDetected.BehaviorUpdate.SearchBehaviorNotActive",
               "The search behavior is not active anymore.. shut down?");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToStart()
{
  DEBUG_SET_STATE(Start);
  // If on charger, then drive off of it
  if (GetBEI().GetRobotInfo().IsOnChargerContacts()) {
    DelegateIfInControl(_iConfig.moveOffChargerBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().IsOnChargerContacts()) {
                            LOG_WARNING("BehaviorReactToBody.TransitionToStart.CouldntMoveOffCharger",
                                        "Driving off the charger failed, stopping this behavior");
                            _dVars.droveOffCharger = false;
                            TransitionToCompleted();
                          }
                          else {
                            _dVars.droveOffCharger = true;
                            TransitionToMaybeGoStraightAndLookUp();
                          }
                        }
    );
  }
  else {
    TransitionToMaybeGoStraightAndLookUp();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToMaybeGoStraightAndLookUp()
{
  DEBUG_SET_STATE(MaybeGoStraightAndLookUp);

  bool shouldGoStraight = false;

  if (_iConfig.shouldDriveStraightWhenBodyDetected) {
    if (! _dVars.droveOffCharger) { // no need to drive straight twice

      const ProxSensorComponent &prox = GetBEI().GetRobotInfo().GetProxSensorComponent();
      u16 distance_mm = 0;
      if (prox.GetLatestDistance_mm(distance_mm)) {
        if (distance_mm < _iConfig.drivingForwardDistance) {
          LOG_DEBUG("BehaviorReactToBody.TransitionToMaybeGoStraightAndLookUp.ObstacleClose",
                   "Can't go straight, an obstacle is at %d while the driving distance would be %f",
                   distance_mm, _iConfig.drivingForwardDistance);
          shouldGoStraight = false; // redundant, but it makes things clear
        }
        else {
          LOG_DEBUG("BehaviorReactToBody.TransitionToMaybeGoStraightAndLookUp.ProxSensorData", "Distance is %u", distance_mm);
          shouldGoStraight = true;
        }
      }
      else {
        LOG_DEBUG("BehaviorReactToBody.TransitionToMaybeGoStraightAndLookUp.NoValidProxSensorData", "");
        shouldGoStraight = false; // redundant, but it makes things clear
      }
    }
  }

  CompoundActionSequential * action;

  // go straight (maybe) and then look up
  if (shouldGoStraight) {
    action = new CompoundActionSequential({new DriveStraightAction(_iConfig.drivingForwardDistance),
                                           new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE))});
  }
  else {
    action = new CompoundActionSequential({new MoveHeadToAngleAction(Radians(MAX_HEAD_ANGLE))});
  }
  CancelDelegates(false);
  DelegateIfInControl(action, &BehaviorReactToBody::TransitionToLookForFace);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToLookForFace()
{

  DEBUG_SET_STATE(LookForFace);

  // pass the bounding box values
  // find the extents of the bounding box
  float maxX = 0, minX = 1, maxY = 0, minY = 1;
  for (const auto &shape : _dVars.lastPersonDetected.shape) {
    if (shape.x > maxX) {
      maxX = shape.x;
    }
    if (shape.x < minX) {
      minX = shape.x;
    }
    if (shape.y > maxY) {
      maxY = shape.y;
    }
    if (shape.y < minY) {
      minY = shape.y;
    }
  }

  if (_iConfig.upperPortionLookUpPercent != 1.0) {
    const float length = maxY - minY;
    const float reducedLength = _iConfig.upperPortionLookUpPercent * length;
    minY = maxY - reducedLength;
  }


  LOG_DEBUG("BehaviorReactToBody.TransitionToLookForFace.BoundingBox",
           "The search bounding box is: x: (%f, %f), y: (%f, %f)", minX, maxX, minY, maxY);

  _iConfig.searchBehavior->SetNewBoundingBox(minX, maxX, minY, maxY);
  if (_iConfig.searchBehavior->WantsToBeActivated()) {
    _dVars.searchingForFaces = true;
    DelegateIfInControl(_iConfig.searchBehavior.get(), &BehaviorReactToBody::TransitionToCompleted);
  }
  else {
    PRINT_NAMED_WARNING("BehaviorReactToBody.TransitionToLookForFace.SearchBehaviorNotReady",
                        "Why doesn't the search behavior want to be activated?");
    TransitionToCompleted();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToFaceFound()
{
  // TODO Tracking a face for the moment, but need to decide what to do next

  DEBUG_SET_STATE(FaceFound);

  if (! ANKI_VERIFY(! _dVars.actingOnFaceFound, // using double not for clarity of intention
                  "BehaviorReactToBody.TransitionToFaceFound.ActingOnFaceFoundShouldBeFalse",
                  "")) {
    TransitionToCompleted();
    return;
  }

  _dVars.actingOnFaceFound = true;

  // getting the best face
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  SmartFaceID bestFace = faceSelection.GetBestFaceToUse(_iConfig.faceSelectionCriteria);

  if (bestFace.IsValid()) {
    auto *action = new TrackFaceAction(bestFace);
    action->SetUpdateTimeout(_iConfig.trackingTimeout);
    CancelDelegates(false);
    DelegateIfInControl(action, &BehaviorReactToBody::TransitionToCompleted);
  }
  else {
    LOG_WARNING("BehaviorReactToBody.TransitionToFaceFound.NoValidFace", "No valid face found for tracking");
    TransitionToCompleted();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToCompleted()
{
  DEBUG_SET_STATE(Completed);

  CancelDelegates(false);
  LOG_DEBUG("BehaviorReactToBody::TransitionToCompleted.Finished","");
}

} // namespace Cozmo
} // namespace Anki
