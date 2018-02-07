/**
 * File: behaviorHighLevelAI.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the high level AI of victor (similar to Cozmo's
 *              freeplay activities)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const float kSocializeKnownFaceCooldown_s = 60.0f * 10;
constexpr const float kPlayWithCubeCooldown_s = 60.0f * 1;
constexpr const float kPlayWithCubeOnChargerCooldown_s = 60.0f * 10;
constexpr const float kGoToSleepTimeout_s = 60.0f * 4;
constexpr const u32 kMinFaceAgeToAllowSleep_ms = 5000;
constexpr const u32 kNeedsToChargeTime_ms = 1000 * 60 * 5;
constexpr const float kMaxFaceDistanceToSocialize_mm = 1000.0f;

}

////////////////////////////////////////////////////////////////////////////////

BehaviorHighLevelAI::BehaviorHighLevelAI(const Json::Value& config)
  : InternalStatesBehavior( config, CreatePreDefinedStrategies() )
{
  
}
  
BehaviorHighLevelAI::~BehaviorHighLevelAI()
{
  
}

InternalStatesBehavior::PreDefinedStrategiesMap BehaviorHighLevelAI::CreatePreDefinedStrategies()
{
  return
  {
    {
      "CloseFaceForSocializing",
      [this](BehaviorExternalInterface& behaviorExternalInterface) {
        if( !StateExitCooldownExpired(GetStateID("Socializing"), kSocializeKnownFaceCooldown_s) ) {
          // still on cooldown
          return false;
        }
        
        auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
        const auto& faces = faceWorld.GetFaceIDs(true);
        for( const auto& faceID : faces ) {
          const auto* face = faceWorld.GetFace(faceID);
          if( face != nullptr ) {
            const Pose3d facePose = face->GetHeadPose();
            float distanceToFace = 0.0f;
            if( ComputeDistanceSQBetween( behaviorExternalInterface.GetRobotInfo().GetPose(),
                                          facePose,
                                          distanceToFace ) &&
                distanceToFace < Util::Square(kMaxFaceDistanceToSocialize_mm) ) {
              return true;
            }
          }
        }
        return false;
      }
    },
    {
      "WantsToSleep",
      [this](BehaviorExternalInterface& behaviorExternalInterface) {
        if( GetCurrentStateID() != GetStateID("ObservingOnCharger") ) {
          PRINT_NAMED_WARNING("BehaviorHighLevelAI.WantsToSleepCondition.WrongState",
                              "This condition only works from ObservingOnCharger");
          return false;
        }
        
        const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if( GetLastTimeStarted( GetStateID("ObservingOnCharger") ) + kGoToSleepTimeout_s <= currTime_s ) {

          // only go to sleep if we haven't recently seen a face
          auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
          Pose3d waste;
          const bool inRobotOriginOnly = false; // might have been picked up
          const TimeStamp_t lastFaceTime = faceWorld.GetLastObservedFace(waste, inRobotOriginOnly);
          const TimeStamp_t lastImgTime = behaviorExternalInterface.GetRobotInfo().GetLastImageTimeStamp();
          if( lastFaceTime < lastImgTime &&
              lastImgTime - lastFaceTime > kMinFaceAgeToAllowSleep_ms ) {
            return true;
          }
        }
        return false;
      }
    },
    {
      "ChargerLocated",
      [](BehaviorExternalInterface& behaviorExternalInterface) {
        BlockWorldFilter filter;
        filter.SetFilterFcn( [](const ObservableObject* obj){
            return IsCharger(obj->GetType(), false) && obj->IsPoseStateKnown();
          });
        const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
        const auto* block = blockWorld.FindLocatedMatchingObject(filter);
        return block != nullptr;
      }
    },
    {
      "NeedsToCharge",
      [](BehaviorExternalInterface& behaviorExternalInterface) {
        const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
        if( !robotInfo.IsCharging() ) {
          const TimeStamp_t lastChargeTime = robotInfo.GetLastChargingStateChangeTimestamp();
          const TimeStamp_t timeSinceNotCharging =
            robotInfo.GetLastMsgTimestamp() > lastChargeTime ?
            robotInfo.GetLastMsgTimestamp() - lastChargeTime :
            0;

          return timeSinceNotCharging >= kNeedsToChargeTime_ms;
        }
        return false;
      }
    },
    {
      "NeedsToLeaveChargerForPlay",
      [this](BehaviorExternalInterface& behaviorExternalInterface) {
        // this keeps two separate timers, one for driving off the charger into play, and one for playing.
        // the latter is useful when seeing cubes when in the observing state, and is used by the
        // CloseCubeForPlaying strategy. The former decides whether the robot should leave the
        // charger to play. In case the robot finds no cube after leaving the charger, placing it
        // back on the charger won't cause it to immediately drive off in search of play.
        // Similarly, if a long time has elapsed since the robot last successfully drove off the charger
        // for playing, but it recently played with a cube, then it shouldn't try to drive off the charger
        // again.
        const bool valueIfNeverRun = false;
        const bool hasntDrivenOffChargerForPlay = StateExitCooldownExpired(GetStateID("DriveOffChargerIntoPlay"),
                                                                           kPlayWithCubeOnChargerCooldown_s,
                                                                           valueIfNeverRun);
        const bool hasntPlayed = StateExitCooldownExpired(GetStateID("PlayingWithCube"),
                                                          kPlayWithCubeCooldown_s,
                                                          valueIfNeverRun);
        return hasntDrivenOffChargerForPlay && hasntPlayed;
      }
    },
    {
      "CloseCubeForPlaying",
      [this](BehaviorExternalInterface& behaviorExternalInterface) {

        if( !StateExitCooldownExpired(GetStateID("PlayingWithCube"), kPlayWithCubeCooldown_s) ) {
          // still on cooldown
          return false;
        }

        BlockWorldFilter filter;
        filter.SetFilterFcn( [](const ObservableObject* obj) {
            return IsValidLightCube(obj->GetType(), false) && obj->IsPoseStateKnown();
          });
        const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
        const auto* block = blockWorld.FindLocatedMatchingObject(filter);
        return block != nullptr;
      }
    }
  };
}

}
}
