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

#include "engine/potentialObjectsForLocalizingTo.h"

#include "coretech/common/engine/math/poseOriginList.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoObservableObject.h"
#include "engine/objectPoseConfirmer.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"
#include "util/math/math.h"

#define ENABLE_BLOCK_BASED_LOCALIZATION 1

#define BLOCK_BASED_LOCALIZATION_DEBUG 0


namespace Anki {
namespace Cozmo {
  
// Using a function vs. a macro here to make sure we continue to compile all the code
// where this gets used below, even when the NDEBUG flag is enabled.
static void VERBOSE_DEBUG_PRINT(const char* eventName, const char* description, ...)
{
# if BLOCK_BASED_LOCALIZATION_DEBUG
  va_list args;
  va_start(args, description);
  ::Anki::Util::sChanneledDebugV("Unnamed", eventName, {}, description, args);
  va_end(args);
# endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PotentialObjectsForLocalizingTo::PotentialObjectsForLocalizingTo(Robot& robot)
: _robot(robot)
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO rsam/andrew: PotentialObjectsForLocalizingTo should not know about PoseConfirmer/ObservationFilter, but
// directly affect objects in BlockWorld because it should receive/process confirmations, not observations
void PotentialObjectsForLocalizingTo::UseDiscardedObservation(ObservedAndMatchedPair& pair, bool wasRobotMoving)
{
  if ( pair.observationAlreadyUsed )
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.UseDiscardedObservation.AlreadyUsed",
                        "Not using observation from %s %d MATCH(%.1f,%.1f,%.1f) OBS(%.1f,%.1f,%.1f), already used. WasRobotMoving:%d",
                        EnumToString(pair.matchedObject->GetType()),
                        pair.matchedObject->GetID().GetValue(),
                        pair.matchedObject->GetPose().GetTranslation().x(),
                        pair.matchedObject->GetPose().GetTranslation().y(),
                        pair.matchedObject->GetPose().GetTranslation().z(),
                        pair.observedObject->GetPose().GetTranslation().x(),
                        pair.observedObject->GetPose().GetTranslation().y(),
                        pair.observedObject->GetPose().GetTranslation().z(),
                        wasRobotMoving);
  
  }
  else
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.UseDiscardedObservation",
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
    
    // affect pose of the object with the observation
    _robot.GetObjectPoseConfirmer().AddVisualObservation(pair.observedObject,
                                                         pair.matchedObject,
                                                         wasRobotMoving,
                                                         pair.distance);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PotentialObjectsForLocalizingTo::Insert(const std::shared_ptr<ObservableObject>& observedObject,
                                             ObservableObject* matchedObject,
                                             f32 observedDistance_mm,
                                             bool observationAlreadyUsed)
{

  ObservedAndMatchedPair newPair{
    .observedObject         = observedObject,
    .matchedObject          = matchedObject,
    .distance               = observedDistance_mm,
    .observationAlreadyUsed = observationAlreadyUsed
  };
  
  const RobotTimeStamp_t obsTime = observedObject->GetLastObservedTime();
  
  const bool isMatchedObjectInCurrentOrigin = _robot.IsPoseInWorldOrigin(newPair.matchedObject->GetPose());
  
  // Don't bother if the matched object doesn't pass these up-front checks:
  const bool couldUseForLocalization = CouldUseObjectForLocalization(matchedObject);
  const bool seeingFromTooFar = Util::IsFltGT(observedDistance_mm, observedObject->GetMaxLocalizationDistance_mm());
  if(!_kCanRobotLocalizeToObjects || !couldUseForLocalization || seeingFromTooFar)
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.NotUsing",
                        "Matched %s %d at dist=%.1f. IsLocalizationToObjectsPossible=%d, CouldUsePair=%d, TooFar=%d",
                        EnumToString(matchedObject->GetType()),
                        matchedObject->GetID().GetValue(),
                        observedDistance_mm,
                        _kCanRobotLocalizeToObjects,
                        couldUseForLocalization,
                        seeingFromTooFar);
    
    // Since we're not storing this pair, update pose if in the current frame
    if(isMatchedObjectInCurrentOrigin)
    {
      const bool wasCameraMoving = _robot.GetMoveComponent().WasCameraMoving(obsTime);
      UseDiscardedObservation(newPair, wasCameraMoving);
    }
    return false;
  }
  
  // Might be sufficient to check for movement at historical time, but to be conservative (and account for
  // timestamping inaccuracies?) we will also check _current_ moving status.
  const bool wasCameraMoving = (_robot.GetMoveComponent().IsCameraMoving() ||
                                _robot.GetMoveComponent().WasCameraMoving(obsTime));
  
  
  // We'd like to use this pair for localization, but the robot is moving,
  // so we just drop it on the floor instead (without even updating the object)  :-(
  // The reasoning here is that we don't want to mess up potentially-localizable objects'
  // poses because we are moving
  if (wasCameraMoving)
  {
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.WasMoving", "t=%u", (TimeStamp_t)obsTime);
    return false;
  }
  
  // Check whether or not the robot had moved since the last time the matched object's pose was updated.
  // If it hasn't moved, then only localize if the observed object pose is nearly identical to the
  // matched object pose.
  if(isMatchedObjectInCurrentOrigin) {
    const RobotTimeStamp_t lastObservedTime = _robot.GetObjectPoseConfirmer().GetLastPoseUpdatedTime(matchedObject->GetID());
    const RobotTimeStamp_t currObservedTime = obsTime;
    const HistRobotState *hrsMatched = nullptr;
    const HistRobotState *hrsObserved = nullptr;
    if (_robot.GetStateHistory()->GetComputedStateAt(lastObservedTime, &hrsMatched) == RESULT_OK &&
        _robot.GetStateHistory()->GetComputedStateAt(currObservedTime, &hrsObserved) == RESULT_OK) {
      
      // Extra tight isSameAs thresholds
      const f32 kSamePoseThresh_mm = 1.f;
      const f32 kSameAngleThresh_rad = DEG_TO_RAD(1.f);
      
      if (hrsObserved->GetPose().IsSameAs(hrsMatched->GetPose(), kSamePoseThresh_mm, kSameAngleThresh_rad)) {
        if (!observedObject->GetPose().IsSameAs(matchedObject->GetPose(), kSamePoseThresh_mm, kSameAngleThresh_rad)) {
          VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.ObjectPoseNotSameEnough", "");
          UseDiscardedObservation(newPair, wasCameraMoving);
          return false;
        }
      }
    }
  }
  
  const PoseOriginID_t matchedOriginID = newPair.matchedObject->GetPose().GetRootID();
  DEV_ASSERT(_robot.GetPoseOriginList().ContainsOriginID(matchedOriginID),
             "PotentialObjectsForLocalizingTo.Insert.ObjectOriginNotInList");
 
  auto iter = _pairMap.find(matchedOriginID);
  if(iter == _pairMap.end()) {
    // This is the first match pair in its frame, just insert it
    VERBOSE_DEBUG_PRINT("PotentialObjectsForLocalizingTo.Insert.AddingNew",
                        "Adding %s %d in frame %p as new",
                        EnumToString(newPair.matchedObject->GetType()),
                        newPair.matchedObject->GetID().GetValue(),
                        matchedOriginID);
                        
    _pairMap[matchedOriginID] = std::move( newPair );
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
                          matchedOriginID,
                          EnumToString(newPair.matchedObject->GetType()),
                          newPair.matchedObject->GetID().GetValue(),
                          newPair.distance);
      
      if(iter->first == _robot.GetPoseOriginList().GetCurrentOriginID()) {
        UseDiscardedObservation(iter->second, wasCameraMoving);
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
                          matchedOriginID,
                          newPair.distance);
      
      if(isMatchedObjectInCurrentOrigin) {
        UseDiscardedObservation(newPair, wasCameraMoving);
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
 
  const PoseOriginID_t currentWorldOriginID = _robot.GetPoseOriginList().GetCurrentOriginID();
  
  // If we have more than 1 thing in the pair map or the only thing in the pair map
  // is from a different frame, then we have cross-frame localization options.
  const bool haveCrossFrameOptions = (_pairMap.size() > 1 ||
                                      _pairMap.begin()->first != currentWorldOriginID);
  
  auto currFrameIter = _pairMap.find(currentWorldOriginID);

  Result localizeResult = RESULT_OK;
  
  if(haveCrossFrameOptions)
  {
    DEV_ASSERT(currFrameIter != _pairMap.end(),
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
    const PoseOriginID_t firstOriginID = pairsToLocalizeToByDist.begin()->second.matchedObject->GetPose().GetRootID();
    const bool recheckMatchObjectByID = (currentWorldOriginID != firstOriginID);
    if( recheckMatchObjectByID )
    {
      DistanceTable::iterator distanceTableIt = pairsToLocalizeToByDist.begin();
      while ( distanceTableIt != pairsToLocalizeToByDist.end() ) {
        const PoseOriginID_t matchedOriginID = distanceTableIt->second.matchedObject->GetPose().GetRootID();
        if ( matchedOriginID == currentWorldOriginID ) {
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
        DEV_ASSERT(matchedID.IsSet(), "PotentialObjectsForLocalizingTo.LocalizeToRobot.NullMatchWithUnsetID");
        
        matchedObj = _robot.GetBlockWorld().GetLocatedObjectByID(matchedID);
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
    DEV_ASSERT(currFrameIter != _pairMap.end(),
               "PotentialObjectsForLocalizingTo.LocalizeRobot.CurrFrameNotFound");
    DEV_ASSERT(_pairMap.size() == 1,
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
  const bool isDockingObject = matchingObject->GetID() == _robot.GetDockingComponent().GetDockObject();
  const bool isTrackToObject = matchingObject->GetID() == _robot.GetMoveComponent().GetTrackToObject();
  const bool objectWasTappedRecently = _robot.WasObjectTappedRecently(matchingObject->GetID());
  
  const bool useThisObjectToLocalize = (objectCanBeUsedForLocalization &&
                                        !isDockingObject &&
                                        !isTrackToObject &&
                                        !objectWasTappedRecently);
  
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
