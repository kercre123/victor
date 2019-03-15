/**
 * File: heldInPalmTracker.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-02-05
 *
 * Description: Behavior component to track various metrics, such as the
 * "trust" level that the robot has when being held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#define LOG_CHANNEL "Behaviors"

#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include "webServerProcess/src/webService.h"
#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

#define CONSOLE_GROUP "HeldInPalm.Tracker"

CONSOLE_VAR(float, kTrackerWebVizUpdatePeriod_s, CONSOLE_GROUP, 60.0);

CONSOLE_VAR(bool, kEnableDebugTransitionPrintouts, CONSOLE_GROUP, false);

namespace {
  static const char* kWebVizModuleNameHeldInPalm = "heldinpalm";
  
  // Once the robot has started being held in a user's palm, the robot should not immediately
  // cancel if a few cliffs are detected, since this makes the tracker less robust to situations
  // where the robot can turn in place and accidentally expose one ore more cliff sensors to a
  // "cliff" caused by the gaps between the user's fingers when holding the robot.
  static const int kMaxCliffsAllowedWhileHeldInPalm = 1;
  
  static const int kMaxCliffsToConfirmRobotPlacedInPalm = 0;
  
  // To prevent false-positive detections of the robot being held in a palm, we enforce that
  // the robot must observe at least this many cliffs after being picked up to try to capture
  // the fact that when a robot is being held by the user and put in their palm, they usually
  // lift the robot high enough in the air that multiple cliffs are detected, whereas when the
  // robot is just pushed or dragged along the ground, or only slightly picked up, the cliff
  // sensors will still be covered, and therefore the robot is not about to be placed in a palm.
  static const int kMinCliffsToConfirmHeldInPalmPickup = 3;
  
  // Sometimes the OffTreadsState experiences a delay in switching from OnTreads to InAir even
  // after 3+ cliffs are detected, so we need to compensate for this in
  // `HasDetectedEnoughCliffsSincePickup`.
  static const u32 kOffTreadsStateChangeDelay_ms = 250;
  
  // If no cliffs have been detected since the robot was picked up, but the robot has been
  // reporting that it has been picked up and held upright for this amount of time, go ahead and
  // declare the robot to be held in a palm anyways.
  static const u32 kTimeToConfirmRobotHeldInPalmDefault_ms = Util::SecToMilliSec(5.0f);
}

CONSOLE_VAR_RANGED(u32, kMinTimeToConfirmRobotHeldInPalm_ms,
                   CONSOLE_GROUP, 1000, 0, kTimeToConfirmRobotHeldInPalmDefault_ms);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HeldInPalmTracker::HeldInPalmTracker()
  : IDependencyManagedComponent(this, BCComponentID::HeldInPalmTracker )
{
  DEV_ASSERT_MSG(kMinCliffsToConfirmHeldInPalmPickup > 0,
                 "HIPTracker.CTor.InvalidNumberOfCliffsToConfirmHeldInPalmPickup",
                 "Number of cliffs to confirm pickup (%i) must be positive",
                 kMinCliffsToConfirmHeldInPalmPickup);
  
  DEV_ASSERT_MSG(kMinCliffsToConfirmHeldInPalmPickup <= CliffSensorComponent::kNumCliffSensors,
                 "HIPTracker.CTor.InvalidNumberOfCliffsToConfirmHeldInPalmPickup",
                 "Number of cliffs to confirm pickup (%i) must be <= to number of cliff sensors %i",
                 kMinCliffsToConfirmHeldInPalmPickup, CliffSensorComponent::kNumCliffSensors);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeldInPalmTracker::SetIsHeldInPalm(const bool isHeldInPalm)
{
  if (_isHeldInPalm != isHeldInPalm) {
    LOG_INFO("HeldInPalmTracker.SetIsHeldInPalm", "%s", isHeldInPalm ? "true" : "false");
  }
  _isHeldInPalm = isHeldInPalm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeldInPalmTracker::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  // Set up event handle for WebViz subscription, and cache a pointer to the FeatureGate interface if it exists
  const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      auto onWebVizSubscribed = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // Send data upon getting subscribed to WebViz
        Json::Value data;
        PopulateWebVizJson(data);
        sendToClient(data);
      };
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameHeldInPalm ).ScopedSubscribe(
                                    onWebVizSubscribed ));
    }
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeldInPalmTracker::UpdateDependent(const BCCompMap& dependentComps)
{
  const float currBSTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currBSTime_s > _nextUpdateTime_s ) {
    _nextUpdateTime_s = currBSTime_s + kTrackerWebVizUpdatePeriod_s;
    
    const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
    if( context ) {
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameHeldInPalm, context->GetWebService()) ) {
        PopulateWebVizJson(webSender->Data());
      }
    }
  }
  
  CheckIfIsHeldInPalm(dependentComps.GetComponent<BEIRobotInfo>());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HeldInPalmTracker::CheckIfIsHeldInPalm(const BEIRobotInfo& robotInfo)
{
  // Grab the current time:
  const auto& currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  // Cache current OffTreadsState and charger-docking status
  const auto& otState = robotInfo.GetOffTreadsState();
  const bool onCharger = robotInfo.IsOnChargerContacts();
  // Reset the flag that indicates that enough cliffs were detected since the last time the robot
  // was picked up, since the robot has yet to be picked up or is still on the charger.
  if (otState == OffTreadsState::OnTreads || onCharger) {
    _enoughCliffsDetectedSincePickup = false;
    _lastTimeOnTreadsOrCharger = currTime;
  }
  
  if ( _isHeldInPalm ) {
    const auto& cliffComp = robotInfo.GetCliffSensorComponent();
    // The robot only continues to be considered held "in a palm" as long as:
    //   1) The robot is being held (picked up and moved around)
    //   2) The cliff sensors detect very few cliffs (currently 0-1 cliffs)
    //   3) The robot is not returned to its charger
    //
    // If condition #2 is not true, it is indicative that some behavior/action has caused the robot
    // to drive very far over the edge of the user's palm. For more details, see explanation of
    // `kMaxCliffsAllowedWhileHeldInPalm`.
    SetIsHeldInPalm(robotInfo.IsBeingHeld() &&
                    cliffComp.GetNumCliffsDetected() <= kMaxCliffsAllowedWhileHeldInPalm &&
                    !onCharger &&
                    // NOTE(GB): The following condition can be removed when a behavior is added
                    // that supports dealing with the user gripping Vector in the palm of their
                    // hand while holding them in the OnRightSide, OnLeftSide, OnBack, or OnFace
                    // orientations. Tracked in VIC-12701.
                    otState == OffTreadsState::InAir);
  } else {
    // If the robot is no longer held in a palm, there are 3 transitions possible that could result
    // in the robot returning to a user's palm:
    //
    // A) The robot is on the ground or on the charger, and therefore the following sequence of
    //    events needs to happen exactly as follows, in order to consider the robot "held on palm":
    //     1) Robot should be currently held, AND not on the charger.
    //     2) Robot should START to observe at least 3 cliff(s) while picked up, i.e. since the
    //        last engine tick that the robot was OnTreads or on its charger
    //     3) Robot should observe NO cliffs while picked up AND held by the user in the UPRIGHT
    //        position (OffTreadsState::InAir), for at least X milliseconds.
    //
    // B) The robot is on the ground (or thinks he's OnTreads) on top of some object that can be
    //    picked up along with the robot. The following events need to happen:
    //     1) Robot should be currently held, AND not on the charger.
    //     2) Robot should observe NO cliffs while picked up AND held by the user in the UPRIGHT
    //        position (OffTreadsState::InAir), for at least Y milliseconds.
    //
    // C) The robot is in the air (in any orientation) because it was recently removed from the
    //    palm of a user's hand, and therefore the following sequence of events needs to happen
    //    exactly as follows, in order to consider the robot "held on palm" AGAIN:
    //     1) Robot should be currently held, AND not on the charger.
    //     2) Robot should observe NO cliffs while picked up AND held by the user in the UPRIGHT
    //        position (OffTreadsState::InAir), for at least X milliseconds.
    //
    //  NOTE(GB): Y >> X
    
    if ( robotInfo.IsBeingHeld() && !onCharger ) {
      const u32 timeSinceOnTreads_ms = GetMillisecondsSince(_lastTimeOnTreadsOrCharger);
      if ( GetTimeSinceLastHeldInPalm_ms() > timeSinceOnTreads_ms ) {
        // Robot recently picked up from ground, hasn't been held in a palm since
        // the pickup occurred. Check condition #2 for scenario B above.
        _enoughCliffsDetectedSincePickup = _enoughCliffsDetectedSincePickup ||
                                            HasDetectedEnoughCliffsSincePickup(robotInfo);
        
        // Check final conditions for scenarios A and B, to see if robot has been placed in palm
        // Depending on whether _enougCliffsDetectedSincePickup is now true after the check above,
        // we select a different time threshold for confirming that the robot is held in a palm.
        
        // We assume by default that Scenario B is occurring
        u32 timeToConfirmRobotHeldInPalm_ms = kTimeToConfirmRobotHeldInPalmDefault_ms;
        if (_enoughCliffsDetectedSincePickup) {
          // If enough cliffs are detected, we can reduce the amount of time needed to confirm
          // that the robot is being held on a user's palm (Scenario A).
          timeToConfirmRobotHeldInPalm_ms = kMinTimeToConfirmRobotHeldInPalm_ms;
        }
        
        SetIsHeldInPalm(WasRobotPlacedInPalmWhileHeld(robotInfo, timeToConfirmRobotHeldInPalm_ms));
#if REMOTE_CONSOLE_ENABLED
        if (kEnableDebugTransitionPrintouts && _isHeldInPalm) {
          if (!_enoughCliffsDetectedSincePickup) {
            LOG_INFO("HIPTracker.CheckIfIsHeldInPalm",
                     "Insufficient cliffs to confirm true pickup, but robot picked up & held for "
                     "%u [ms], confirming held-on-palm status anyways",
                     kTimeToConfirmRobotHeldInPalmDefault_ms);
          } else {
            LOG_INFO("HIPTracker.CheckIfIsHeldInPalm", "Robot transitioned to palm from ground");
          }
        }
#endif
      } else {
        // Robot has been held in a user's palm more recently than it was on the
        // ground (OffTreadsState::OnTreads). Check final condition for scenario C.
        SetIsHeldInPalm(WasRobotPlacedInPalmWhileHeld(robotInfo, kMinTimeToConfirmRobotHeldInPalm_ms));
#if REMOTE_CONSOLE_ENABLED
        if (kEnableDebugTransitionPrintouts && _isHeldInPalm) {
          LOG_INFO("HIPTracker.CheckIfIsHeldInPalm", "Robot transitioned to palm while being held");
        }
#endif
      }
    }
  }
  
  if (!_isHeldInPalm) {
    _lastTimeNotHeldInPalm = currTime;
  } else {
    _lastHeldInPalmTime = currTime;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HeldInPalmTracker::WasRobotPlacedInPalmWhileHeld(const BEIRobotInfo& robotInfo,
                                                      const u32 timeToConfirmHeldInPalm_ms) const
{
  ANKI_VERIFY(robotInfo.IsBeingHeld(), "HIPTracker.WasRobotPlacedInPalmWhileHeld",
              "Robot NOT being held, check is invalid");
  ANKI_VERIFY(!robotInfo.IsOnChargerContacts(), "HIPTracker.WasRobotPlacedInPalmWhileHeld",
              "Robot still on charger contacts, check is invalid");
  
  const auto& otStateLastChangeTime = robotInfo.GetOffTreadsStateLastChangedTime_ms();
  // NOTE(GB): In this case, "InAir" specifically refers to the robot being upright
  // while held by the user, i.e. no extreme pitch or roll
  const auto timeSinceNotInAir_ms = GetMillisecondsSince(otStateLastChangeTime);
  
  const auto& heldStatusLastChangeTime = robotInfo.GetBeingHeldLastChangedTime_ms();
  const auto timeSinceNotHeld_ms = GetMillisecondsSince(heldStatusLastChangeTime);
  
  if (robotInfo.IsBeingHeld() &&
      robotInfo.GetOffTreadsState() == OffTreadsState::InAir &&
      timeSinceNotHeld_ms >= timeToConfirmHeldInPalm_ms &&
      timeSinceNotInAir_ms >= timeToConfirmHeldInPalm_ms ) {
    const auto& cliffComp = robotInfo.GetCliffSensorComponent();
    const u32 durationOfNoCliffDetections_ms =
      cliffComp.GetDurationForNCliffDetections_ms(kMaxCliffsToConfirmRobotPlacedInPalm);
    
    return durationOfNoCliffDetections_ms >= timeToConfirmHeldInPalm_ms;
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HeldInPalmTracker::HasDetectedEnoughCliffsSincePickup(const BEIRobotInfo& robotInfo) const
{
  ANKI_VERIFY(robotInfo.IsBeingHeld(), "HIPTracker.HasDetectedEnoughCliffsSincePickup",
              "Robot NOT being held, check is invalid");
  ANKI_VERIFY(!robotInfo.IsOnChargerContacts(), "HIPTracker.HasDetectedEnoughCliffsSincePickup",
              "Robot still on charger contacts, check is invalid");
  
  const auto& cliffComp = robotInfo.GetCliffSensorComponent();
  const int currNumCliffs = cliffComp.GetNumCliffsDetected();
  if (currNumCliffs >= kMinCliffsToConfirmHeldInPalmPickup) {
    const u32 timeSinceOnTreadsOrCharger_ms = GetMillisecondsSince(_lastTimeOnTreadsOrCharger);
    const u32 durationOfCurrentCliffDetections_ms =
      cliffComp.GetDurationForNCliffDetections_ms(currNumCliffs);
    if (durationOfCurrentCliffDetections_ms <= timeSinceOnTreadsOrCharger_ms + kOffTreadsStateChangeDelay_ms) {
#if REMOTE_CONSOLE_ENABLED
      if (kEnableDebugTransitionPrintouts) {
        LOG_INFO("HIPTracker.HasDetectedEnoughCliffsSincePickup",
                 "Robot has been detecting at least %u cliffs for %u [ms], %u [ms] elapsed since "
                 "last OnTreads or on charger", currNumCliffs, durationOfCurrentCliffDetections_ms,
                 timeSinceOnTreadsOrCharger_ms);
      }
#endif
      return true;
    }
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 HeldInPalmTracker::GetTimeSinceLastHeldInPalm_ms() const
{
  return _isHeldInPalm ? 0 : GetMillisecondsSince(_lastHeldInPalmTime);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 HeldInPalmTracker::GetHeldInPalmDuration_ms() const
{
  return _isHeldInPalm ? GetMillisecondsSince(_lastTimeNotHeldInPalm) : 0;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u32 HeldInPalmTracker::GetMillisecondsSince(const EngineTimeStamp_t& pastTimestamp) const
{
  const auto& currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  return static_cast<u32>( (currTime > pastTimestamp) ? (currTime - pastTimestamp) : 0 );
}

}
}
