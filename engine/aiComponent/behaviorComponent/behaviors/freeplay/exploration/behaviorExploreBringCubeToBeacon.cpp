/**
 * File: behaviorExploreBringCubeToBeacon
 *
 * Author: Raul
 * Created: 04/14/16
 *
 * Description: Behavior to pick up and bring a cube to the current beacon.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorExploreBringCubeToBeacon.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/viz/vizManager.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

#include <array>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// internal constants and helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// padding between cubes when calculating destination positions inside a beacon
CONSOLE_VAR(float, kBebctb_PaddingBetweenCubes_mm, "BehaviorExploreBringCubeToBeacon", 10.0f);
// debug render for the behavior
CONSOLE_VAR(bool, kBebctb_DebugRenderAll, "BehaviorExploreBringCubeToBeacon", true);

// number of attempts to do an action before flagging as failure
const int kMaxAttempts = 3; // 1 + retries (actions seem to fail fairly often for no apparent reason)
// if a cube fails, how far does it have to move so that we ignore that previous failure
const float kCubeFailureDist_mm = 20.0f;
// if a cube fails, how far does it have to turn so that we ignore that previous failure
const float kCubeFailureRot_rad = DEG_TO_RAD(22.5f);

// if a location fails, how far does it invalidate other poses
// if a location failed, chances are nearby locations will fail too. Now, the whiteboard doesn't store all failures, so
// if we find one, and we fail, it has to invalidate a bunch of locations, otherwise we may find a valid one, fail, and
// add it to the whiteboard, which can cause the first location we tried to be removed as failure (due to limited
// storage) and we will try again on that location despite we should have known it can fail. The distance here should be
// enough that we won't try within a beacon more locations that we can store as failures in the whiteboard. This can
// be tricky to figure out, so I will just tune: beacon radius, whiteboard storage and this distance to work together.
// If any changes, chances are the robot can get stuck here.
const float kLocationFailureDist_mm = 100.0f;
// if a location fails, what rotation invalidates for other poses
const float kLocationFailureRot_rad = M_PI_F;
  
const char* kRecentFailureCooldownKey = "recentFailureCooldown_sec";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LocationCalculator: given row and column can calculate a 3d pose in a beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct LocationCalculator {
  LocationCalculator(const ObservableObject* pickedUpObject, const Vec3f& beaconCenter, const Rotation3d& directionality, float beaconRadius_mm, const BehaviorExternalInterface& bei, float recentFailureCooldown_sec)
  : object(pickedUpObject), center(beaconCenter), rotation(directionality), radiusSQ(beaconRadius_mm*beaconRadius_mm), beiRef(bei), renderAsFirstFind(true), recentFailureCooldown_sec(recentFailureCooldown_sec)
  {
    DEV_ASSERT(nullptr != object, "BehaviorExploreBringCubeToBeacon.LocationCalculator.NullObjectWillCrash");
  }
  
  const ObservableObject* object;
  const Vec3f& center;
  const Rotation3d& rotation;
  const float radiusSQ;
  const BehaviorExternalInterface& beiRef;
  mutable bool renderAsFirstFind;
  const float recentFailureCooldown_sec;

  // calculate location offset given current config
  float GetLocationOffset() const;
  
  // compute pose for row and col, and return true if it's free to drop a cube, false otherwise
  bool IsLocationFreeForObject(const int row, const int col, Pose3d& outPose) const;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float LocationCalculator::GetLocationOffset() const
{
  const float cubeLen = object->GetSize().x();
  DEV_ASSERT(FLT_NEAR(object->GetSize().x(), object->GetSize().y()) , "LocationCalculator.GetLocationOffset");
  return cubeLen + kBebctb_PaddingBetweenCubes_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LocationCalculator::IsLocationFreeForObject(const int row, const int col, Pose3d& outPose) const
{
  // calculate 3D location
  const float locOffset = GetLocationOffset();
  
  Vec3f offset{ locOffset*row, locOffset*col, 0.0f};
  offset = rotation * offset;
  Vec3f candidateLoc = center + offset;
  outPose = Pose3d(rotation, candidateLoc, beiRef.GetRobotInfo().GetWorldOrigin()); // override even if not free
  
  // check if out of radius
  if ( offset.LengthSq() > radiusSQ )
  {
    // debug render
    if ( kBebctb_DebugRenderAll )
    {
      Quad2f candidateQuad = object->GetBoundingQuadXY(outPose);
      beiRef.GetRobotInfo().GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorExploreBringCubeToBeacon.Locations",
        candidateQuad, 20.0f, NamedColors::BLACK, false);
    }
    return false;
  }
  
  // calculate if candidate pose is close to a previous failure
  const bool isLocationFailureInWhiteboard = beiRef.GetAIComponent().GetComponent<AIWhiteboard>().DidFailToUse(-1,
    AIWhiteboard::ObjectActionFailure::PlaceObjectAt,
    recentFailureCooldown_sec,
    outPose, kLocationFailureDist_mm, kLocationFailureRot_rad);
  
  // calculate if candidate pose is free of other objects
  bool collidesWithObjects = false;
  if ( !isLocationFailureInWhiteboard )
  {
    // note: this part of the code is similar to the DriveToPlaceCarriedObjectAction free goal check, standarize?
    BlockWorldFilter ignoreSelfFilter;
    ignoreSelfFilter.AddIgnoreID( object->GetID() );
    
    // calculate quad at candidate destination
    const Quad2f& candidateQuad = object->GetBoundingQuadXY(outPose);

    // TODO rsam: this only checks for other cubes, but not for unknown obstacles since we don't have collision sensor
    std::vector<const ObservableObject *> intersectingObjects;
    beiRef.GetBlockWorld().FindLocatedIntersectingObjects(candidateQuad,
                                                            intersectingObjects,
                                                            kBebctb_PaddingBetweenCubes_mm,
                                                            ignoreSelfFilter);
    
    collidesWithObjects = !intersectingObjects.empty();
  }

  // it's free if it passes all checks
  bool isFree = !isLocationFailureInWhiteboard && !collidesWithObjects;
  
  // debug render
  if ( kBebctb_DebugRenderAll ) {
    beiRef.GetRobotInfo().GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorExploreBringCubeToBeacon.Locations",
      object->GetBoundingQuadXY(outPose), 20.0f, isFree ? (renderAsFirstFind ? NamedColors::YELLOW : NamedColors::WHITE) : NamedColors::RED, false);
    renderAsFirstFind = renderAsFirstFind && !isFree; // render as first as long as we don't find one free
  }
  
  return isFree;
}

} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreBringCubeToBeacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreBringCubeToBeacon::BehaviorExploreBringCubeToBeacon(const Json::Value& config)
: ICozmoBehavior(config)
{
  LoadConfig(config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreBringCubeToBeacon::~BehaviorExploreBringCubeToBeacon()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kRecentFailureCooldownKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetDebugLabel() + ".BehaviorExploreBringCubeToBeacon";

  _configParams.recentFailureCooldown_sec = ParseFloat(config, kRecentFailureCooldownKey, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreBringCubeToBeacon::WantsToBeActivatedBehavior() const
{
  _candidateObjects.clear();
  const AIWhiteboard& whiteboard = GetAIComp<AIWhiteboard>();
  
  // check that we have an active beacon
  const AIBeacon* selectedBeacon = whiteboard.GetActiveBeacon();
  if ( nullptr == selectedBeacon ) {
    return false;
  }
  
  // check that we haven't failed recently here
  const float lastBeaconFailure = selectedBeacon->GetLastTimeFailedToFindLocation();
  const bool beaconEverFailed = !NEAR_ZERO(lastBeaconFailure);
  if ( beaconEverFailed ) {
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float beaconTimeoutUntil = lastBeaconFailure + _configParams.recentFailureCooldown_sec;
    const bool beaconIsInCooldown = FLT_LT(curTime, beaconTimeoutUntil);
    if ( beaconIsInCooldown ) {
      return false;
    }
  }

  // ask the whiteboard to retrieve the cubes to bring to beacons
  AIWhiteboard::ObjectInfoList cubesOutOfBeacons;
  const bool hasBlocksOutOfBeacons = whiteboard.FindUsableCubesOutOfBeacons(cubesOutOfBeacons);
  if ( hasBlocksOutOfBeacons )
  {
    // check if there were recent failures with those objects, we don't want to try to pick them up again
    // so that we don't go into a loop on pick up
    for( const AIWhiteboard::ObjectInfo& objectInfo : cubesOutOfBeacons )
    {
      const ObservableObject* objPtr = GetBEI().GetBlockWorld().
                                          GetLocatedObjectByID( objectInfo.id, objectInfo.family );
      if ( nullptr != objPtr )
      {
        const Pose3d& currentPose = objPtr->GetPose();
        const bool recentFail = whiteboard.DidFailToUse(objectInfo.id,
          {{AIWhiteboard::ObjectActionFailure::PickUpObject, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie}},
          _configParams.recentFailureCooldown_sec,
          currentPose, kCubeFailureDist_mm, kCubeFailureRot_rad);
        
        if ( !recentFail )
        {
          // this object should be ok to be picked up
          _candidateObjects.emplace_back(objectInfo);
        }
      }
    }
  }
  
  const bool hasCandidates = !_candidateObjects.empty();
  return hasCandidates;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::OnBehaviorActivated()
{
  // clear variables from previous run
  _selectedObjectID.UnSet();
  
  // calculate starting state
  if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() )
  {
    // we are carrying an object
    // assert what we expect from WantsToBeActivated cache
    DEV_ASSERT(_candidateObjects.size() == 1 &&
               _candidateObjects[0].id == GetBEI().GetRobotInfo().GetCarryingComponent().GetCarryingObject(),
               "BehaviorExploreBringCubeToBeacon.InitInternal.CarryingObjectNotCached" );
    
    // select it and pretend we just picked it up
    _selectedObjectID = _candidateObjects[0].id;
    TransitionToObjectPickedUp();
  }
  else
  {
    // we are not currently carrying one, pick up one
    const int attemptNumber = 1;
    TransitionToPickUpObject(attemptNumber);
  }
  
  // this is now a valid situation. We started the behavior but did not find valid poses. It should have flagged
  // the beacon as not valid anymore
  // bool shouldControlBeDelegated = true;
  // if ( !IsControlDelegated() ) {
  //   const AIBeacon* activeBeacon = GetAIComp<AIWhiteboard>().GetActiveBeacon();
  //   const float lastBeaconFailure = activeBeacon->GetLastTimeFailedToFindLocation();
  //   const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  //   const bool beaconFlaggedFail = FLT_NEAR(curTime, lastBeaconFailure);
  //   shouldControlBeDelegated = !beaconFlaggedFail; // should have delegated if the beacon is valid
  // }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::OnBehaviorDeactivated()
{
  // clear render on exit just in case
  if ( kBebctb_DebugRenderAll ) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    
    robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreBringCubeToBeacon.Locations");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToPickUpObject(int attempt)
{
  if ( !_candidateObjects.empty() )
  {
    // if _selectedObjectID is set this is a retry
    const bool isRetry = _selectedObjectID.IsSet();
    if ( !isRetry )
    { 
      // pick closest block (rsam/brad: this would be a good place to use path distance instead of euclidean)
      const Pose3d& robotPose = GetBEI().GetRobotInfo().GetPose();
      const BlockWorld& world = GetBEI().GetBlockWorld();

      size_t bestIndex = 0;
      float bestDistSQ = FLT_MAX;

      for (size_t idx=0; idx < _candidateObjects.size(); ++idx)
      {
        const ObservableObject* candidateObj = GetCandidate(world, idx);
        
        // calculate distance wrt robot
        Pose3d objWrtRobot;
        const bool canGetWrtRobot = (candidateObj && candidateObj->GetPose().GetWithRespectTo(robotPose, objWrtRobot) );
        const float candidateDistSQ = (canGetWrtRobot ? objWrtRobot.GetTranslation().LengthSq() : FLT_MAX);
        
        // check if candidate is better than current best
        if ( FLT_LT(candidateDistSQ, bestDistSQ) )
        {
          bestDistSQ = candidateDistSQ;
          bestIndex = idx;
        }
      }

      // did we find a candidate?
      const bool foundCandidate = FLT_LT(bestDistSQ, FLT_MAX);
      if ( foundCandidate ) {
        _selectedObjectID = _candidateObjects[bestIndex].id;
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".TransitionToPickUpObject.Selected").c_str(), "Going to pick up '%d'", _selectedObjectID.GetValue());
      } else {
        PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.InvalidCandidates", "Could not pick candidate");
        return;
      }
    }
    else
    {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".TransitionToPickUpObject.Retry").c_str(), "Trying to pick up '%d' again", _selectedObjectID.GetValue());
    }
    
    // fire action with proper callback
    DriveToPickupObjectAction* pickUpAction = new DriveToPickupObjectAction(_selectedObjectID );
    RobotCompletedActionCallback onPickUpActionResult = [this, attempt](const ExternalInterface::RobotCompletedAction& actionRet)
    {
      // arguably here we could check isCarrying regardless of action result. Even if the action failed, as long
      // as we are carrying the object we intended to pick up, we should be good
      bool pickUpFinalFail = false;
      const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(actionRet.result);
      if ( resCat == ActionResultCategory::SUCCESS ) {
        // object was picked up
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPickUpActionResult.Done").c_str(), "Picked up '%d'", _selectedObjectID.GetValue());
        TransitionToObjectPickedUp();
      }
      else if (resCat == ActionResultCategory::RETRY)
      {
        const auto& robotInfo = GetBEI().GetRobotInfo();
        
        // do we currently have the object in the lift?
        const bool isCarrying = (robotInfo.GetCarryingComponent().IsCarryingObject() &&
                                 robotInfo.GetCarryingComponent().GetCarryingObject() == _selectedObjectID);
        if ( isCarrying )
        {
          PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPickUpActionResult.RetryOk").c_str(), "We do have '%d' picked up, so pretend we are fine", _selectedObjectID.GetValue());
          // not sure what failed on pick up, but we are carrying the object, so continue to next state
          TransitionToObjectPickedUp();
        }
        else if ( attempt < kMaxAttempts )
        {
          PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPickUpActionResult.RetryMaybe").c_str(), "Let's try to pick up '%d' again (%d tries out of %d)",
            _selectedObjectID.GetValue(), attempt, kMaxAttempts);
          // something else failed, maybe we failed to align with the cube, try to pick up the cube again
          TransitionToPickUpObject(attempt+1 );
        }
        else
        {
          PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPickUpActionResult.Fail").c_str(), "Not trying to pick up '%d' again. Failing", _selectedObjectID.GetValue());
          // do not queue more actions here so that we get kicked out, flag as fail
          pickUpFinalFail = true;
        }
      } else if(resCat == ActionResultCategory::ABORT) {
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPickUpActionResult.NoRetry").c_str(), "Failed to pick up '%d', action does not retry.", _selectedObjectID.GetValue());
        // do not queue more actions here so that we get kicked out, flag as fail
        pickUpFinalFail = true;
      }
      
      // failed to pick up this object, tell the whiteboard about it
      if ( pickUpFinalFail ) {
        const ObservableObject* failedObject = GetBEI().GetBlockWorld().GetLocatedObjectByID( _selectedObjectID );
        if ( failedObject ) {
          GetAIComp<AIWhiteboard>().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::PickUpObject);
        }
        
        // rsam: considering this, but pickUp action would already fire emotion events
        // fire emotion event, Cozmo is sad he could not pick up the cube it selected
        // GetBEI().GetMoodManager().TriggerEmotionEvent("HikingFailedToPickUp", MoodManager::GetCurrentTimeInSeconds());
      }
    };
    DelegateIfInControl(pickUpAction, onPickUpActionResult);
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.NoCandidates", "Can't run with no selected objects");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TryToStackOn(const ObjectID& bottomCubeID, int attempt)
{
  // note pass bottomCubeID to the lambda as copy, do not trust it's scope
  // create action to stack
  DriveToPlaceOnObjectAction* stackAction = new DriveToPlaceOnObjectAction(bottomCubeID);
  RobotCompletedActionCallback onStackActionResult = [this, bottomCubeID, attempt](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    bool stackOnCubeFinalFail = false;
    const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(actionRet.result);
    if (resCat == ActionResultCategory::SUCCESS)
    {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onStackActionResult.Done").c_str(), "Successfully stacked cube");
      // emotions and behavior objective check
      FireEmotionEvents();
    }
    else if (resCat == ActionResultCategory::RETRY)
    {
      const auto& robotInfo = GetBEI().GetRobotInfo();
      
      const bool canRetry = (attempt < kMaxAttempts);
      const bool isCarrying = (robotInfo.GetCarryingComponent().IsCarryingObject() &&
                               robotInfo.GetCarryingComponent().GetCarryingObject() == _selectedObjectID);
      if ( canRetry && isCarrying )
      {
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onStackActionResult.CanRetry").c_str(),
          "Failed to stack '%d' on top of '%d', but can retry",
          _selectedObjectID.GetValue(), bottomCubeID.GetValue());

        // we are carrying the cube and we can try to stack again
        TryToStackOn(bottomCubeID, attempt+1 );
      }
      else
      {
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onStackActionResult.CannotRetry").c_str(),
          "Failed to stack '%d' on top of '%d' (attempt=%d/%d) (carrying=%s)",
          _selectedObjectID.GetValue(), bottomCubeID.GetValue(),
          attempt, kMaxAttempts,
          isCarrying ? "yes" : "no");
        // if we are not carrying the cube, wth happened? Did someone remove the cube from the lift?
        // it probably isn't the bottom's cube fault, but maybe it actually is, so flag as a failure
        // to stack on top of it, in case we tried and we dropped our cube upon placing it on top of the bottom one
        // if we are carrying the cube, this is definitely a stack issue
        stackOnCubeFinalFail = true;
      }
    } else if(resCat == ActionResultCategory::ABORT) {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onStackActionResult.NoRetryAllowed").c_str(), "Failed to stack (no retry allowed by action)");
      stackOnCubeFinalFail = true;
    }
    
    // failed to stack on the bottom object, notify the whiteboard
    if ( stackOnCubeFinalFail ) {
      const ObservableObject* failedObject = GetBEI().GetBlockWorld().GetLocatedObjectByID( bottomCubeID );
      if (failedObject) {
        GetAIComp<AIWhiteboard>().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::StackOnObject);
      }
      
      // rsam: considering this, but placeOn action would already fire emotion events
      // fire emotion event, Cozmo is sad he could not stack the cube on top of the other one
      // GetBEI().GetMoodManager().TriggerEmotionEvent("HikingFailedToStack", MoodManager::GetCurrentTimeInSeconds());
    }
  };
  DelegateIfInControl( stackAction, onStackActionResult );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TryToPlaceAt(const Pose3d& pose, int attempt)
{
  // note pass pose to the lambda as copy, do not trust it's scope
  // create action to drive to the drop location
  const bool checkFreeDestination = true;
  const float padding_mm = kBebctb_PaddingBetweenCubes_mm;
  
  PlaceObjectOnGroundAtPoseAction* placeObjectAction = new PlaceObjectOnGroundAtPoseAction(pose, false, checkFreeDestination, padding_mm);
  RobotCompletedActionCallback onPlaceActionResult = [this, pose, attempt](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    bool placeAtCubeFinalFail = false;
    const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(actionRet.result);
    if (resCat == ActionResultCategory::SUCCESS)
    {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPlaceActionResult.Done").c_str(), "Successfully placed cube");
      
      // emotions and behavior objective check
      FireEmotionEvents();
    }
    else if (resCat == ActionResultCategory::RETRY)
    {
      const auto& robotInfo = GetBEI().GetRobotInfo();
      
      const bool canRetry = false || (attempt < kMaxAttempts);
      const bool isCarrying = (robotInfo.GetCarryingComponent().IsCarryingObject() &&
                               robotInfo.GetCarryingComponent().GetCarryingObject() == _selectedObjectID);
      if ( canRetry && isCarrying )
      {
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPlaceActionResult.Done.CanRetry").c_str(),
          "Failed to place '%d' at pose [%.2f,%.2f,%.2f], but can retry",
          _selectedObjectID.GetValue(), pose.GetTranslation().x(), pose.GetTranslation().y(), pose.GetTranslation().z() );

        // we are carrying the cube and we want to retry this location
        TryToPlaceAt( pose, attempt+1 );
      }
      else
      {
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPlaceActionResult.CannotRetry").c_str(),
          "Failed to place '%d' at pose [%.2f,%.2f,%.2f] (attempt=%d/%d) (carrying=%s)",
          _selectedObjectID.GetValue(),
          pose.GetTranslation().x(), pose.GetTranslation().y(), pose.GetTranslation().z(),
          attempt, kMaxAttempts,
          isCarrying ? "yes" : "no");
        // if we are not carrying the cube, wth happened? Did someone remove the cube from the lift?
        // it although the location was probably not the reason why we failed, maybe it is, so flag here as
        // a failure to place this cube at that specific location
        placeAtCubeFinalFail = true;
      }
    } else {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".onPlaceActionResult.NoRetryAllowed").c_str(), "Failed to place (no retry allowed by action)");
      placeAtCubeFinalFail = true;
    }
    
    // failed to place this cube at this location
    if ( placeAtCubeFinalFail ) {
      const ObservableObject* failedObject = GetBEI().GetBlockWorld().GetLocatedObjectByID( _selectedObjectID );
      if (failedObject) {
        GetAIComp<AIWhiteboard>().SetFailedToUse(*failedObject, AIWhiteboard::ObjectActionFailure::PlaceObjectAt, pose);
      }

      // rsam: considering this, but placeAt action would already fire emotion events
      // fire emotion event, Cozmo is sad he could not place the cube at the selected destination
      // GetBEI().GetMoodManager().TriggerEmotionEvent("HikingFailedToPlace", MoodManager::GetCurrentTimeInSeconds());
    }

  };

  DelegateIfInControl( placeObjectAction, onPlaceActionResult );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::FireEmotionEvents()
{
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    const bool allCubesInBeacons = GetAIComp<AIWhiteboard>().AreAllCubesInBeacons();
    if ( allCubesInBeacons )
    {
      // fire emotion event, Cozmo is happy he brought the last cube to the beacon
      moodManager.TriggerEmotionEvent("HikingBroughtLastCubeToBeacon", MoodManager::GetCurrentTimeInSeconds());
    }
    else
    {
      // fire emotion event, Cozmo is happy he brought a cube to the beacon
      moodManager.TriggerEmotionEvent("HikingBroughtCubeToBeacon", MoodManager::GetCurrentTimeInSeconds());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToObjectPickedUp()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // clear this render to not mistake previous frame debug with this
  if ( kBebctb_DebugRenderAll ) {
    robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreBringCubeToBeacon.Locations");
  }
  
  // if we successfully picked up the object we wanted we can continue, otherwise freak out
  if ( robotInfo.GetCarryingComponent().IsCarryingObject() &&
       robotInfo.GetCarryingComponent().GetCarryingObject() == _selectedObjectID )
  {
    const ObservableObject* const pickedUpObject = GetBEI().GetBlockWorld().GetLocatedObjectByID( _selectedObjectID);
    if ( pickedUpObject == nullptr ) {
      PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.ObjectIsNull",
        "Could not obtain obj from ID '%d'", _selectedObjectID.GetValue());
        return;
    }
    
    // grab the selected beacon (there should be one)
    AIWhiteboard& whiteboard = GetAIComp<AIWhiteboard>();
    AIBeacon* selectedBeacon = whiteboard.GetActiveBeacon();
    if (nullptr == selectedBeacon) {
      DEV_ASSERT(nullptr!= selectedBeacon, "BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.NullBeacon");
      return;
    }
    
    // 1) if there're other cubes and we can put this on top, do that
    const ObservableObject* bottomCube = FindFreeCubeToStackOn(pickedUpObject, selectedBeacon);
    if ( nullptr != bottomCube )
    {
      // log
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".TransitionToObjectPickedUp").c_str(), "Decided to place '%d' on top of '%d'",
        _selectedObjectID.GetValue(),
        bottomCube->GetID().GetValue());
      
      // queue stack on action
      const ObjectID& bottomCubeID = bottomCube->GetID();
      const int attemptNumber = 1;
      TryToStackOn(bottomCubeID, attemptNumber);
    }
    else
    {
      // 2) otherwise, find area as close to the center as possible (use memory map for this?)
      
      Pose3d dropPose;
      const bool foundPose = FindFreePoseInBeacon(pickedUpObject, selectedBeacon, GetBEI(),
                                                  dropPose, _configParams.recentFailureCooldown_sec);
      if ( foundPose )
      {
        // log
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".TransitionToObjectPickedUp").c_str(),
          "Decided to place '%d' on the floor at [%.2f,%.2f,%.2f]",
          _selectedObjectID.GetValue(), dropPose.GetTranslation().x(), dropPose.GetTranslation().y(), dropPose.GetTranslation().z());
        
        // queue place at action
        const int attemptNumber = 1;
        TryToPlaceAt(dropPose, attemptNumber);
      }
      else
      {
        // aw, this beacon is bad :( do not use anymore (at least for some time)
        // maybe this should cause the creation of a new beacon? <--
        whiteboard.FailedToFindLocationInBeacon(selectedBeacon);
      
        // log
        PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".TransitionToObjectPickedUp.NoFreePoses").c_str(),
          "Could not decide where to drop the cube in the beacon (all poses failed)");

        // fire emotion event, Cozmo is sad he could not put down the cube in the beacon
        // This may be hard for the player to understand, but at least will give context as to why
        // Cozmo is simply putting down the cube
        GetBEI().GetMoodManager().TriggerEmotionEvent("HikingNoLocationAtBeacon", MoodManager::GetCurrentTimeInSeconds());
      }
    }
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.NotPickedUp",
      "We do not have the cube picked up, we should not have transitioned here.");
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorExploreBringCubeToBeacon::FindFreeCubeToStackOn(
  const ObservableObject* object,
  const AIBeacon* beacon) const
{
  // ask for all cubes we know, and if any is not inside a beacon, then we want to bring that one to the closest beacon
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ ObjectFamily::Block, ObjectFamily::LightCube }});
  
  // additional threshold so that we don't stack on top of a cube in the border. This prevents stacking on
  // a cube close to the border, which would cause the stacked cube to be out of the beacon
  const float kPrecisionOffset_mm = 10.0f; // this is just to account for errors when readjusting cube positions
  const float inwardThreshold_mm = object->GetDimInParentFrame<'X'>() + kPrecisionOffset_mm;
  
  filter.AddFilterFcn([object,beacon,inwardThreshold_mm,this](const ObservableObject* blockPtr)
  {
    // if this is our block or state is not good, skip
    if ( blockPtr == object || !blockPtr->IsPoseStateKnown() ) {
      return false;
    }
    
    // if we don't want to stack on this cube, skip
    const AIWhiteboard& whiteboard = GetAIComp<AIWhiteboard>();
    const Pose3d& currentPose = blockPtr->GetPose();
    const bool recentFail = whiteboard.DidFailToUse(blockPtr->GetID(),
      AIWhiteboard::ObjectActionFailure::StackOnObject,
      _configParams.recentFailureCooldown_sec,
      currentPose, kCubeFailureDist_mm, kCubeFailureRot_rad);
    
    
    if ( recentFail )
    {
      return false;
    }
    
    DEV_ASSERT(FLT_NEAR(object->GetSize().x(), object->GetSize().y()) ,
               "BehaviorExploreBringCubeToBeacon.FindFreeCubeToStackOn.AssumedXYEqual");
    
    // check it this cube is in beacon, but also if it's actually closer than
    const bool isBlockInSelectedBeacon = beacon->IsLocWithinBeacon(blockPtr->GetPose(), inwardThreshold_mm);
    if ( isBlockInSelectedBeacon )
    {
      const auto& robotInfo = GetBEI().GetRobotInfo();
      
      // check if we can stack on top of this object
      const bool canStackOnObject = robotInfo.GetDockingComponent().CanStackOnTopOfObject(*blockPtr);
      if ( canStackOnObject )
      {
        // TODO rsam: this stops at the first found, not closest or anything fancy
        return true;
      }
    }
    
    return false;
  });
                      
  const ObservableObject* ret = GetBEI().GetBlockWorld().FindLocatedMatchingObject(filter);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Directionality helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

const Vec3f& kUpVector = Z_AXIS_3D();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// calculate average directionality and return true if calculated, false if could not figure out average (because
// objects were null)
__attribute__((used))
bool CalculateDirectionalityAverage(AIWhiteboard::ObjectInfoList& objectsInBeacon, const BlockWorld& world, Rotation3d& outDirectionality)
{
  double avgAngle = 0.0;
  int angleCount  = 0; // how many angles we calculated (should be objectsInBeacon.size() if no null objs are allowed)

  for( const auto& objectInfo : objectsInBeacon )
  {
    const ObservableObject* objectPtr = world.GetLocatedObjectByID( objectInfo.id, objectInfo.family );
    if ( nullptr != objectPtr )
    {
      double upAngle = objectPtr->GetPose().GetWithRespectToRoot().GetRotation().GetAngleAroundZaxis().ToDouble();
      // normalize to to range [-45deg,45deg], to align either axis
      const double closest90Angle = std::round(upAngle/M_PI_2) * M_PI_2;
      upAngle = upAngle - closest90Angle;
      DEV_ASSERT(upAngle <=  M_PI_4, "FindFreePoseInBeacon.OutOfRange.Positive");
      DEV_ASSERT(upAngle >= -M_PI_4, "FindFreePoseInBeacon.OutOfRange.Negative");

      // add angle to
      avgAngle += upAngle;
      ++angleCount;
    }
  }

  // if we have some angles to average
  if ( angleCount > 0 )
  {
    // calculate final angle and apply
    const double finalAngle = avgAngle / angleCount;
    outDirectionality = Rotation3d( finalAngle, kUpVector );
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// inherit the directionality from the object closest to the center
__attribute__((used))
bool CalculateDirectionalityClosest(AIWhiteboard::ObjectInfoList& objectsInBeacon, const BlockWorld& world, const Pose3d& center, Rotation3d& outDirectionality)
{
  float bestDistToCenterSQ = FLT_MAX;
  const ObservableObject* bestObject = nullptr;
  
  for( const auto& objectInfo : objectsInBeacon )
  {
    const ObservableObject* objectPtr = world.GetLocatedObjectByID( objectInfo.id, objectInfo.family );
    if ( nullptr != objectPtr )
    {
      Pose3d relative;
      if ( objectPtr->GetPose().GetWithRespectTo(center, relative) )
      {
        const float distSQ = relative.GetTranslation().LengthSq();
        if ( distSQ < bestDistToCenterSQ )
        {
          bestDistToCenterSQ = distSQ;
          bestObject = objectPtr;
        }
      }
      else
      {
        PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.CalculateDirectionalityClosest.InvalidPose",
                          "Can't get object '%d' pose wrt beacon center", objectInfo.id.GetValue());
      }
    }
  }

  // if we got the best object (closest to center)
  if ( nullptr != bestObject )
  {
    // calculate final angle and apply (do not copy Rotation since objects sometimes have roll due to bad estimations,
    // which we want to discard here
    Radians rotZ = bestObject->GetPose().GetWithRespectToRoot().GetRotation().GetAngleAroundZaxis();
    outDirectionality = Rotation3d( rotZ, kUpVector );
    return true;
  }

  return false;
}

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreBringCubeToBeacon::FindFreePoseInBeacon(
  const ObservableObject* object,
  const AIBeacon* beacon, 
  const BehaviorExternalInterface& behaviorExternalInterface, 
  Pose3d& freePose, 
  float recentFailureCooldown_sec)
{
  Rotation3d beaconDirectionality(0.0f, kUpVector);
  bool isDirectionalitySetFromObjects = false;
  
  // compute directionality when there are cubes inside the beacon
  AIWhiteboard::ObjectInfoList objectsInBeacon;
  const bool hasObjectsInBeacon = behaviorExternalInterface.GetAIComponent().GetComponent<AIWhiteboard>().FindCubesInBeacon(beacon, objectsInBeacon);
  if ( hasObjectsInBeacon )
  {
    const BlockWorld& world = behaviorExternalInterface.GetBlockWorld();
    
    // I have implemented two ways of doing this. Average works well, but aligned to the cube closest to center
    // seems to look better. In the general case where no cubes or one start at the beacon, all cubes should
    // line up anyway, regardless of these methods. This covers the case of what to do if there are already two or more
    // cubes in the beacon and we bring another one
    #define USE_AVERAGE 0
    #if USE_AVERAGE
    {
      isDirectionalitySetFromObjects = CalculateDirectionalityAverage(objectsInBeacon, world, beaconDirectionality);
    }
    #else
    {
      const Pose3d& beaconCenter = beacon->GetPose();
      isDirectionalitySetFromObjects = CalculateDirectionalityClosest(objectsInBeacon, world, beaconCenter, beaconDirectionality);
    }
    #endif
  }
    
  // if there are no objects currently in the beacon, or could not get a valid directionality, calculate one now
  if( !isDirectionalitySetFromObjects )
  {
    const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
    
    // TODO rsam put this utility somewhere: create Rotation3d from vector in XY plane
    Vec3f beaconNormal = (beacon->GetPose().GetWithRespectToRoot().GetTranslation() - robotInfo.GetPose().GetTranslation());
    beaconNormal.z() = 0.0f;
    float distance = beaconNormal.MakeUnitLength();
    
    float rotRads = 0.0f;
    if ( !NEAR_ZERO(distance) )
    {
      // calculate rotation transform for the beaconNormal
      const Vec3f& kFwdVector = X_AXIS_3D();
      const Vec3f& kRightVector = -Y_AXIS_3D();
    
      const float fwdAngleCos = DotProduct(beaconNormal, kFwdVector);
      const bool isPositiveAngle = (DotProduct(beaconNormal, kRightVector) >= 0.0f);
      rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);
      
      beaconDirectionality = Rotation3d( rotRads, kUpVector );
    }
  }
  const Vec3f beaconCenter = beacon->GetPose().GetWithRespectToRoot().GetTranslation();
  LocationCalculator locCalc(object, beaconCenter, beaconDirectionality, beacon->GetRadius(), behaviorExternalInterface, recentFailureCooldown_sec);

  const int kMaxRow = beacon->GetRadius() / locCalc.GetLocationOffset();
  const int kMaxCol = kMaxRow;
  
  bool foundLocation = false;
  
  // iterate negative rows first so that locations start at center and become closer every time
  for( int row=0; row>=-kMaxRow; --row)
  {
    foundLocation = locCalc.IsLocationFreeForObject(row, 0, freePose);
    if( foundLocation ) { break; }
    
    for( int col=1; col<=kMaxCol; ++col)
    {
      foundLocation = locCalc.IsLocationFreeForObject(row, -col, freePose);
      if( foundLocation ) { break; }
      foundLocation = locCalc.IsLocationFreeForObject(row,  col, freePose);
      if( foundLocation ) { break; }
    }
    
    // this is necessary because the inner breaks affect columns only
    if( foundLocation ) { break; }
  }
  
  // if first loop didn't find location yet, try second
  if ( !foundLocation ) {
    // positive order now (away from robot)
    for( int row=1; row<=kMaxRow; ++row)
    {
      foundLocation = locCalc.IsLocationFreeForObject(row, 0, freePose);
      if( foundLocation ) { break; }
      
      for( int col=1; col<=kMaxCol; ++col)
      {
        foundLocation = locCalc.IsLocationFreeForObject(row, -col, freePose);
        if( foundLocation ) { break; }
        
        foundLocation = locCalc.IsLocationFreeForObject(row,  col, freePose);
        if( foundLocation ) { break; }
      }
      
      // this is necessary because the inner breaks affect columns only
      if( foundLocation ) { break; }
    }
  }
  
  return foundLocation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorExploreBringCubeToBeacon::GetCandidate(const BlockWorld& world, size_t index) const
{
  const ObservableObject* ret = nullptr;
  if ( index < _candidateObjects.size() ) {
    ret = world.GetLocatedObjectByID(_candidateObjects[index].id, _candidateObjects[index].family);
  }
  return ret;
}

} // namespace Cozmo
} // namespace Anki
