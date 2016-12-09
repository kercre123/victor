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

#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/math/math.h"

#define ENABLE_BLOCK_BASED_LOCALIZATION 1

#define BLOCK_BASED_LOCALIZATION_DEBUG 0


namespace Anki {
namespace Cozmo {
  
namespace {
  CONSOLE_VAR_RANGED(f32, kMaxBodyRotationSpeed_degPerSec, "LocalizationToObjects", 0.1f,  0.f, 20.f);
  CONSOLE_VAR_RANGED(f32, kMaxHeadRotationSpeed_degPerSec, "LocalizationToObjects", 0.1f,  0.f, 20.f);
}
  
// Using a function vs. a macro here to make sure we continue to compile all the code
// where this gets used below, even when the DEBUG flag is disabled.
static void VERBOSE_DEBUG_PRINT(const char* eventName, const char* description, ...)
{
# if BLOCK_BASED_LOCALIZATION_DEBUG
  va_list args;
  va_start(args, description);
  ::Anki::Util::sChanneledDebugV("Unnamed", eventName, {}, description, args);
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
  
  
void PotentialObjectsForLocalizingTo::UpdateMatchedObjectPose(ObservedAndMatchedPair& pair, bool wasRobotMoving)
{
  VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.UpdateMatchedObjectPose",
                      "Updating %s %d pose from (%.1f,%.1f,%.1f) to (%.1f,%.1f,%.1f). WasRobotMoving:%d",
                      EnumToString(pair.matchedObject->GetType()),
                      pair.matchedObject->GetID().GetValue(),
                      pair.matchedObject->GetPose().GetTranslation().x(),
                      pair.matchedObject->GetPose().GetTranslation().y(),
                      pair.matchedObject->GetPose().GetTranslation().z(),
                      pair.observedObject->GetPose().GetTranslation().x(),
                      pair.observedObject->GetPose().GetTranslation().y(),
                      pair.observedObject->GetPose().GetTranslation().z(),
                      wasRobotMoving);
  
  Result result = _robot.GetObjectPoseConfirmer().AddVisualObservation(pair.matchedObject,
                                                                       pair.observedObject->GetPose(),
                                                                       wasRobotMoving,
                                                                       pair.distance);
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("PotentialObjectsForLocalizingTo.UpdateMatchedObjectPose.AddVisualObservationFailed", "");
    return;
  }
  
}
  

bool PotentialObjectsForLocalizingTo::Insert(const std::shared_ptr<ObservableObject>& observedObject,
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
  const bool couldUseForLocalization = CouldUseObjectForLocalization(matchedObject);
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
      const bool wasRobotMoving = _robot.GetVisionComponent().WasMovingTooFast(observedObject->GetLastObservedTime(),
                                                                               DEG_TO_RAD(kMaxBodyRotationSpeed_degPerSec),
                                                                               DEG_TO_RAD(kMaxHeadRotationSpeed_degPerSec));
      UpdateMatchedObjectPose(newPair, wasRobotMoving);
    }
    return false;
  }
  

//  const bool wasRobotMoving = _robot.GetVisionComponent().WasMovingTooFast(observedObject->GetLastObservedTime(),
//                                                                           DEG_TO_RAD(kMaxBodyRotationSpeed_degPerSec),
//                                                                           DEG_TO_RAD(kMaxHeadRotationSpeed_degPerSec));

  // Seems to be better than the commented line above which should be more accurate,
  // but this incorporates some delay which might compensate for whatever timestamp inaccuracies there may be...
  // or something...
  const bool wasRobotMoving = _robot.GetMoveComponent().IsMoving();
  
  
  // We'd like to use this pair for localization, but the robot is moving,
  // so we just drop it on the floor instead (without even updating the object)  :-(
  // The reasoning here is that we don't want to mess up potentially-localizable objects'
  // poses because we are moving
  if (wasRobotMoving)
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.IsMoving", "");
    return false;
  }
  
  // Check whether or not the robot had moved since the last time the matched object's pose was updated.
  // If it hasn't moved, then only localize if the observed object pose is nearly identical to the
  // matched object pose.
  if(matchedOrigin == _currentWorldOrigin) {
    const TimeStamp_t lastObservedTime = _robot.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(matchedObject->GetID());
    const TimeStamp_t currObservedTime = observedObject->GetLastObservedTime();
    const RobotPoseStamp *rpsMatched;
    const RobotPoseStamp *rpsObserved;
    if (_robot.GetPoseHistory()->GetComputedPoseAt(lastObservedTime, &rpsMatched) == RESULT_OK &&
        _robot.GetPoseHistory()->GetComputedPoseAt(currObservedTime, &rpsObserved) == RESULT_OK) {
      
      // Extra tight isSameAs thresholds
      const f32 kSamePoseThresh_mm = 1.f;
      const f32 kSameAngleThresh_rad = DEG_TO_RAD(1.f);
      
      if (rpsObserved->GetPose().IsSameAs(rpsMatched->GetPose(), kSamePoseThresh_mm, kSameAngleThresh_rad)) {
        if (!observedObject->GetPose().IsSameAs(matchedObject->GetPose(), kSamePoseThresh_mm, kSameAngleThresh_rad)) {
          VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.ObjectPoseNotSameEnough", "");
          UpdateMatchedObjectPose(newPair, wasRobotMoving);
          return false;
        }
      }
    }
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
        UpdateMatchedObjectPose(iter->second, wasRobotMoving);
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
        UpdateMatchedObjectPose(newPair, wasRobotMoving);
      }
    }
    
  }
  
  // Wanted to insert this object (whether we ended up doing so or not)
  return true;
  
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
    ASSERT_NAMED(currFrameIter != _pairMap.end(),
                 "PotentialObjectsForLocalizingTo.LocalizeRobot.MustHaveCurrFrameObjectWithCrossFrameOptions");
    
    // sort frames from farthest to closest
    using DistanceTable = std::multimap<f32, ObservedAndMatchedPair, std::greater<f32>>;
    DistanceTable pairsToLocalizeToByDist;
    for (auto & matchPair : _pairMap)
    {
      pairsToLocalizeToByDist.insert(std::make_pair(matchPair.second.distance, matchPair.second));
    }
    
    // If the current origin is not the first one we will use for localization, then
    // we will need to re-look it up by ID while localizing below since it will have
    // been rejiggered out of the current frame
    const Pose3d* firstOrigin = &(pairsToLocalizeToByDist.begin()->second.matchedObject->GetPose().FindOrigin());
    const bool recheckMatchObjectByID = _currentWorldOrigin != firstOrigin;
    if( recheckMatchObjectByID )
    {
      DistanceTable::iterator distanceTableIt = pairsToLocalizeToByDist.begin();
      while ( distanceTableIt != pairsToLocalizeToByDist.end() ) {
        const Pose3d* origin = &(distanceTableIt->second.matchedObject->GetPose().FindOrigin());
        if ( origin == _currentWorldOrigin ) {
          distanceTableIt->second.matchedID = distanceTableIt->second.matchedObject->GetID();
          PRINT_CH_INFO("BlockWorld", "PotentialObjectsForLocalizingTo.LocalizeRobot.StoringMatchedObjectID",
                        "Match in current frame not farthest. Storing ID=%d to recheck when encountered while localizing.",
                        distanceTableIt->second.matchedID.GetValue());
          distanceTableIt->second.matchedObject = nullptr; // Mark the fact that we should use the ID instead
          break;
        }
        ++distanceTableIt;
      }
    }

    // Loop over all cross-frame matches
    // localization with each, to rejigger all their frames into one.
    
    bool anyFailures = false;
    for (auto & matchPair : pairsToLocalizeToByDist)
    {
      ObservableObject* observedObj = matchPair.second.observedObject.get();
      ObservableObject* matchedObj  = matchPair.second.matchedObject;
     
      if(nullptr == matchedObj)
      {
        const ObjectID& matchedID = matchPair.second.matchedID;
        ASSERT_NAMED(matchedID.IsSet(), "PotentialObjectsForLocalizingTo.LocalizeToRobot.NullMatchWithUnsetID");
        
        matchedObj = _robot.GetBlockWorld().GetObjectByID(matchedID);
        if(nullptr == matchedObj)
        {
          PRINT_NAMED_WARNING("PotentialObjectsForLocalizingTo.LocalizeToRobot.MissingMatchedObjectInCurrentFrame",
                              "Matched object %d no longer exists. Skipping match pair.", observedObj->GetID().GetValue());
          continue;
        }
      }
      
      if(matchedObj != nullptr)
      {
        VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.LocalizeRobot.UsingCrossFrame",
                            "Localizing using %s %d",
                            EnumToString(matchedObj->GetType()),
                            matchedObj->GetID().GetValue());
      }
      
      const Result result = _robot.LocalizeToObject(observedObj, matchedObj);
      if(result != RESULT_OK) {
        anyFailures = true;
        
        if(matchedObj != nullptr)
        {
          PRINT_NAMED_WARNING("PotentialObjectsForLocalizingTo.LocalizeRobot.CrossFrameLocalizeFailure",
                              "Failed to localize to %s object %d.",
                              ObjectTypeToString(matchedObj->GetType()),
                              matchedObj->GetID().GetValue());
        }
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
bool PotentialObjectsForLocalizingTo::CouldUseObjectForLocalization(const ObservableObject* matchingObject)
{
  // Decide whether we will be updating the robot's pose relative to this
  // object or updating the object's pose w.r.t. the robot. We only localize
  // to the object if:
  //  - if the object believes itself to be usable for localization (based on
  //    pose state (which includes observation distance), pose flatness, etc)
  //  - the object is neither the object the robot is docking to or tracking to
  
  const bool objectCanBeUsedForLocalization = matchingObject->CanBeUsedForLocalization();
  const bool isDockingObject = matchingObject->GetID() == _robot.GetDockObject();
  const bool isTrackToObject = matchingObject->GetID() == _robot.GetMoveComponent().GetTrackToObject();
  
  const bool useThisObjectToLocalize = (objectCanBeUsedForLocalization &&
                                        !isDockingObject &&
                                        !isTrackToObject);
  
  VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.CouldUseObjectForLocalization",
                      "%s %d: objCanBeUsedForLoc=%d(%s) isDockObj=%d isTrackToObj=%d",
                      EnumToString(matchingObject->GetType()),
                      matchingObject->GetID().GetValue(),
                      objectCanBeUsedForLocalization,
                      matchingObject->GetPoseState()==PoseState::Known ? "Known" : "Unknown/Dirty",
                      isDockingObject, isTrackToObject);
                      
  return useThisObjectToLocalize;
  
}
  
} // namespace Anki
} // namespace Cozmo
