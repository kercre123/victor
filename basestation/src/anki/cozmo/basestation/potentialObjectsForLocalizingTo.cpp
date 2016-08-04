/**
 * File: potentialObjectsForLocalizingTo.cpp
 *
 * Author: Andrew Stein
 * Created: 7/5/2016
 *
 * Description:
 *   Helper class used during AddAndUpdate for tracking potential objects to localize to.
 *   Keeps the closest observed/matching object pair in each coordinate frame.
 *   Updates the matching object's pose to the observed pose for those that are not used
 *   or when they are replaced by a closer pair, if they are in the current origin.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/potentialObjectsForLocalizingTo.h"

#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/math/math.h"

#define ENABLE_BLOCK_BASED_LOCALIZATION 1

#define BLOCK_BASED_LOCALIZATION_DEBUG 0


namespace Anki {
namespace Cozmo {
  
// Using a function vs. a macro here to make sure we continue to compile all the code
// where this gets used below, even when the DEBUG flag is disabled.
static void VERBOSE_DEBUG_PRINT(const char* eventName, const char* description, ...)
{
# if BLOCK_BASED_LOCALIZATION_DEBUG
  va_list args;
  va_start(args, description);
  PRINT_NAMED_DEBUG(eventName, description, args);
  va_end(args);
# endif
}
  

PotentialObjectsForLocalizingTo::PotentialObjectsForLocalizingTo(Robot& robot)
: _robot(robot)
, _currentWorldOrigin(_robot.GetWorldOrigin())
, _kCanRobotLocalizeToObjects(ENABLE_BLOCK_BASED_LOCALIZATION &&
                              (_robot.GetLocalizedTo().IsUnknown() ||
                               _robot.HasMovedSinceBeingLocalized()))
{
  VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Constructor",
                      "IsLocalizationToObjectsPossible=%d: unknownLocObj=%d hasMovedSinceLoc=%d",
                      _kCanRobotLocalizeToObjects,
                      _robot.GetLocalizedTo().IsUnknown(),
                      _robot.HasMovedSinceBeingLocalized());
}
  
void PotentialObjectsForLocalizingTo::ObservedAndMatchedPair::UpdateMatchedObjectPose(bool isRobotMoving)
{
  VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.UpdateMatchedObjectPose",
                      "Updating %s %d pose from (%.1f,%.1f,%.1f) to (%.1f,%.1f,%.1f)",
                      EnumToString(matchedObject->GetType()),
                      matchedObject->GetID().GetValue(),
                      matchedObject->GetPose().GetTranslation().x(),
                      matchedObject->GetPose().GetTranslation().y(),
                      matchedObject->GetPose().GetTranslation().z(),
                      observedObject->GetPose().GetTranslation().x(),
                      observedObject->GetPose().GetTranslation().y(),
                      observedObject->GetPose().GetTranslation().z());
  
  if(isRobotMoving || Util::IsFltGT(distance, MAX_LOCALIZATION_AND_ID_DISTANCE_MM))
  {
    // If we're seeing this object from too far away or while moving...
    
    if(matchedObject->GetPoseState() != PoseState::Known) {
      // ...and it was already not known (dirty or unknown), then update the pose
      // and leave or upgrade it to dirty
      matchedObject->SetPose( observedObject->GetPose(), distance, PoseState::Dirty);
    }
    
    // ... and it was previously known, then don't update it with what is likely a
    // _worse_ pose
  }
  else
  {
    // Seeing from close enough and while not moving: update the pose (and mark it Known)
    matchedObject->SetPose( observedObject->GetPose(), distance );
  }
  
}

void PotentialObjectsForLocalizingTo::Insert(const std::shared_ptr<ObservableObject>& observedObject,
                                             ObservableObject* matchedObject,
                                             f32 observedDistance)
{
  ObservedAndMatchedPair newPair{
    .observedObject = observedObject,
    .matchedObject  = matchedObject,
    .distance       = observedDistance,
  };
  
  const PoseOrigin* matchedOrigin = &newPair.matchedObject->GetPose().FindOrigin();
  
  // Don't bother if the matched object doesn't pass these up-front checks:
  const bool couldUseForLocalization = CouldUseObjectForLocalization(matchedObject, observedDistance);
  if(!_kCanRobotLocalizeToObjects || !couldUseForLocalization)
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.NotUsing",
                        "Matched %s %d at dist=%.1f. IsLocalizationToObjectsPossible=%d, CouldUsePair=%d",
                        EnumToString(matchedObject->GetType()),
                        matchedObject->GetID().GetValue(),
                        observedDistance,
                        _kCanRobotLocalizeToObjects,
                        couldUseForLocalization);
    
    // Since we're not storing this pair, update pose if in the current frame
    if(matchedOrigin == _currentWorldOrigin) {
      newPair.UpdateMatchedObjectPose(_robot.GetMoveComponent().IsMoving());
    }
    return;
  }
  
  // We'd like to use this pair for localization, but the robot is moving,
  // so we just drop it on the floor instead (without even updating the object)  :-(
  // The reasoning here is that we don't want to mess up potentially-localizable objects'
  // poses because we are moving
  if (_robot.GetMoveComponent().IsMoving()) {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.IsMoving", "");
    return;
  }
  
  auto iter = _pairMap.find(matchedOrigin);
  if(iter == _pairMap.end()) {
    // This is the first match pair in its frame, just insert it
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.AddingNew",
                        "Adding %s %d in frame %p as new",
                        EnumToString(newPair.matchedObject->GetType()),
                        newPair.matchedObject->GetID().GetValue(),
                        matchedOrigin);
                        
    _pairMap[matchedOrigin] = std::move( newPair );
  }
  else
  {
    // We already have a possible localization object in this frame.
    if(newPair.distance < iter->second.distance)
    {
      // Replace the stored one if this new one is closer.
      // If the stored pair we are about to replace is in the current frame,
      // update the matched object's pose before we discard it
      VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.Replacing",
                          "Replacing %s %d at d=%.1fmm in frame %p with %s %d at d=%.1f",
                          EnumToString(iter->second.matchedObject->GetType()),
                          iter->second.matchedObject->GetID().GetValue(),
                          iter->second.distance,
                          matchedOrigin,
                          EnumToString(newPair.matchedObject->GetType()),
                          newPair.matchedObject->GetID().GetValue(),
                          newPair.distance);
      
      if(iter->first == _currentWorldOrigin) {
        iter->second.UpdateMatchedObjectPose(_robot.GetMoveComponent().IsMoving());
      }
      std::swap(iter->second, newPair);
    }
    else
    {
      // This pair is further than the one already stored, no need to store it.
      // Just merge it before we discard it if it's in the current world frame.
      VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.Ignoring",
                          "Ignoring %s %d in frame %p because d=%.1f is too large",
                          EnumToString(newPair.matchedObject->GetType()),
                          newPair.matchedObject->GetID().GetValue(),
                          matchedOrigin,
                          newPair.distance);
      
      if(matchedOrigin == _currentWorldOrigin) {
        newPair.UpdateMatchedObjectPose(_robot.GetMoveComponent().IsMoving());
      }
    }
  }
} // Insert()

Result PotentialObjectsForLocalizingTo::LocalizeRobot()
{
  if(_pairMap.empty()) {
    // No localization options, nothing to do
    return RESULT_OK;
  }
  
  // If we have more than 1 thing in the pair map or the only thing in the pair map
  // is from a different frame, then we have cross-frame localization options.
  const bool haveCrossFrameOptions = (_pairMap.size() > 1 ||
                                      _pairMap.begin()->first != _currentWorldOrigin);
  
  auto currFrameIter = _pairMap.find(_currentWorldOrigin);
  
  Result localizeResult = RESULT_OK;
  
  if(haveCrossFrameOptions)
  {
//    // We will not be localizing to the object in the current frame (if there is one).
//    // So make sure to merge it now and remove it from further consideration.
//    if(currFrameIter != _pairMap.end()) {
//      VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.LocalizeRobot.UpdateCurrFrame",
//                          "Using cross-frame options, not current frame");
//      currFrameIter->second.UpdateMatchedObjectPose(_robot.GetMoveComponent().IsMoving());
//      _pairMap.erase(currFrameIter);
//    }
    
    // Loop over all cross-frame matches from farthest to closest and perform
    // localization with each, to rejigger all their frames into one.
    
    std::map<f32, ObservedAndMatchedPair, std::greater<f32>> pairsToLocalizeToByDist;
    for (auto & matchPair : _pairMap) {
      pairsToLocalizeToByDist[matchPair.second.distance] = matchPair.second;
    }
    
    bool anyFailures = false;
    for (auto & matchPair : pairsToLocalizeToByDist)
    {
      ObservableObject* observedObj = matchPair.second.observedObject.get();
      ObservableObject* matchedObj  = matchPair.second.matchedObject;
      
      VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.LocalizeRobot.UsingCrossFrame",
                          "Localizing using %s %d",
                          EnumToString(matchedObj->GetType()),
                          matchedObj->GetID().GetValue());
      
      const Result result = _robot.LocalizeToObject(observedObj, matchedObj);
      if(result != RESULT_OK) {
        anyFailures = true;
        PRINT_NAMED_WARNING("PotentialObjectsForLocalizingTo.LocalizeRobot.CrossFrameLocalizeFailure",
                            "Failed to localize to %s object %d.",
                            ObjectTypeToString(matchedObj->GetType()),
                            matchedObj->GetID().GetValue());
      }
    }
    
    if(anyFailures) {
      localizeResult = RESULT_FAIL;
    }
  }
  else
  {
    // If there are no potential cross-frame localizations, then just localize
    // to the object in the current frame. Note the data structure
    // ensures this will be the closest object in this frame.
    ASSERT_NAMED(currFrameIter != _pairMap.end(),
                 "PotentialObjectsForLocalizingTo.LocalizeRobot.CurrFrameNotFound");
    ASSERT_NAMED(_pairMap.size() == 1,
                 "PotentialObjectsForLocalizingTo.LocalizeRobot.ExpectingSingleMatchPair");
    
    ObservedAndMatchedPair& pair = currFrameIter->second;
    
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.LocalizeRobot.UsingCurrentFrame",
                        "Localizing using %s %d",
                        EnumToString(pair.matchedObject->GetType()),
                        pair.matchedObject->GetID().GetValue());
    
    localizeResult = _robot.LocalizeToObject(pair.observedObject.get(), pair.matchedObject);
    if(localizeResult != RESULT_OK) {
      PRINT_NAMED_ERROR("PotentialObjectsForLocalizingTo.LocalizeRobot.LocalizeFailure",
                        "Failed to localize to %s object %d.",
                        ObjectTypeToString(pair.observedObject->GetType()),
                        pair.matchedObject->GetID().GetValue());
    }
  }
  
  return localizeResult;
}

// a.k.a. THE IF
bool PotentialObjectsForLocalizingTo::CouldUseObjectForLocalization(const ObservableObject* matchingObject,
                                                                    f32 distToObj)
{
  // Decide whether we will be updating the robot's pose relative to this
  // object or updating the object's pose w.r.t. the robot. We only localize
  // to the object if:
  //  - the object is close enough,
  //  - if the object believes itself to be usable for localization (based on
  //    pose state, pose flatness, etc)
  //  - the object is neither the object the robot is docking to or tracking to
  
  const bool closeEnough = distToObj <= MAX_LOCALIZATION_AND_ID_DISTANCE_MM;
  const bool objectCanBeUsedForLocalization = matchingObject->CanBeUsedForLocalization();
  const bool isDockingObject = matchingObject->GetID() == _robot.GetDockObject();
  const bool isTrackToObject = matchingObject->GetID() == _robot.GetMoveComponent().GetTrackToObject();
  
  const bool useThisObjectToLocalize = (closeEnough &&
                                        objectCanBeUsedForLocalization &&
                                        !isDockingObject &&
                                        !isTrackToObject);
  
  VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.CouldUseObjectForLocalization",
                      "%s %d: closeEnough=%d(%.1fmm) objCanBeUsedForLoc=%d(%s) isDockObj=%d isTrackToObj=%d",
                      EnumToString(matchingObject->GetType()),
                      matchingObject->GetID().GetValue(),
                      closeEnough, distToObj,
                      objectCanBeUsedForLocalization,
                      matchingObject->GetPoseState()==PoseState::Known ? "Known" : "Unknown/Dirty",
                      isDockingObject, isTrackToObject);
                      
  return useThisObjectToLocalize;
  
}
  
} // namespace Anki
} // namespace Cozmo