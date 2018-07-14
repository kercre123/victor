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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/faceWorld.h"


namespace Anki {
namespace Cozmo {

namespace {
  const char* const kDriveOffChargerBehaviorKey        = "driveOffChargerBehavior";
  const char* const kShouldDriveStraightIfNotOnCharger = "shouldDriveStraightIfNotOnCharger";
  const char* const kDrivingForwadDistance             = "drivingForwadDistance_mm";
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBody::DynamicVariables::DynamicVariables()
{
  Reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::DynamicVariables::Reset()
{
  lastPersonDetected = Vision::SalientPoint();
  searchingForFaces = false;
  imageTimestampWhenActivated = 0;
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
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::OnBehaviorActivated()
{

  // save the last salient point
  Vision::SalientPoint lastPersonDetected(std::move(_dVars.lastPersonDetected));

  // reset dynamic variables
  _dVars = DynamicVariables();

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

  auto& component = GetBEI().GetAIComponent().GetComponent<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestPersons;

  //Get all the latest persons
  component.GetSalientPointSinceTime(latestPersons, Vision::SalientPointType::Person,
                                     lastPersonDetected.timestamp);

  if (latestPersons.empty()) {
    LOG_DEBUG( "BehaviorReactToPersonDetected.OnBehaviorActivated.NoPersonDetected", ""
        "Activated but no person with a timestamp > %u", _dVars.lastPersonDetected.timestamp);
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


  DEV_ASSERT(_dVars.lastPersonDetected.salientType == Vision::SalientPointType::Person,
             "BehaviorReactToPersonDetected.OnBehaviorActivated.LastSalientPointMustBePerson");

  // Action!
  TransitionToStart();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::BehaviorUpdate()
{

  const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

  if (pickedUp) {
    // definitively stop here
    LOG_DEBUG( "BehaviorReactToPersonDetected.BehaviorUpdate.PickedUp",
              "Robot has been picked up");
    TransitionToCompleted();
    return;
  }

  if( ! IsActivated() || ! IsControlDelegated()) {
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
                        &BehaviorReactToBody::TransitionToCheckIfGoStraight);
  }
  else {
    TransitionToCheckIfGoStraight();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cozmo::BehaviorReactToBody::TransitionToCheckIfGoStraight()
{
  DEBUG_SET_STATE(CheckIfFGoStraight);

  if (_iConfig.shouldDriveStraightWhenBodyDetected) {
    TransitionToGoStraight();
  }
  else {
    TransitionToLookForFace();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cozmo::BehaviorReactToBody::TransitionToGoStraight()
{
  DEBUG_SET_STATE(GoStraight);

  auto* action = new DriveStraightAction(_iConfig.drivingForwardDistance);
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

  LOG_DEBUG("BehaviorReactToBody.TransitionToLookForFace.BoundingBox",
           "The search bounding box is: min: (%f, %f), max: (%f, %f)", minX, minY, maxX, maxY);

  _iConfig.searchBehavior->SetNewBoundingBox(minX, maxX, minY, maxY);
  if (_iConfig.searchBehavior->WantsToBeActivated()) {
    _dVars.searchingForFaces = true;
    DelegateIfInControl(_iConfig.searchBehavior.get(), &BehaviorReactToBody::TransitionToCompleted);
  }
  else {
    PRINT_NAMED_WARNING("BehaviorReactToBody.TransitionToLookForFace.SearchBehaviorNotReady",
                        "Why doens't the search behavior want to be activated?");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBody::TransitionToFaceFound()
{
  // TODO Play an animation? Do something?

  DEBUG_SET_STATE(FaceFound);
  LOG_DEBUG("BehaviorReactToPersonDetected.TransitionToFaceFound.WhatNext",
            "Play an animation?");
  TransitionToCompleted();
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
