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

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const char* kDebugName = "BehaviorHighLevelAI";
  
}

////////////////////////////////////////////////////////////////////////////////

BehaviorHighLevelAI::BehaviorHighLevelAI(const Json::Value& config)
  : InternalStatesBehavior( config, CreatePreDefinedStrategies() )
{
  _params.socializeKnownFaceCooldown_s = JsonTools::ParseFloat(config, "socializeKnownFaceCooldown_s", kDebugName);
  _params.playWithCubeCooldown_s = JsonTools::ParseFloat(config, "playWithCubeCooldown_s", kDebugName);
  _params.playWithCubeOnChargerCooldown_s = JsonTools::ParseFloat(config, "playWithCubeOnChargerCooldown_s", kDebugName);
  _params.goToSleepTimeout_s = JsonTools::ParseFloat(config, "goToSleepTimeout_s", kDebugName);
  _params.minFaceAgeToAllowSleep_ms = JsonTools::ParseUInt32(config, "minFaceAgeToAllowSleep_ms", kDebugName);
  _params.needsToChargeTime_ms = JsonTools::ParseUInt32(config, "needsToChargeTime_ms", kDebugName);
  _params.maxFaceDistanceToSocialize_mm = JsonTools::ParseFloat(config, "maxFaceDistanceToSocialize_mm", kDebugName);
  
  MakeMemberTunable( _params.socializeKnownFaceCooldown_s, "socializeKnownFaceCooldown_s" );
  MakeMemberTunable( _params.playWithCubeOnChargerCooldown_s, "playWithCubeOnChargerCooldown_s" );
  MakeMemberTunable( _params.playWithCubeCooldown_s, "playWithCubeCooldown_s" );
  
  AddConsoleVarTransitions( "MoveToState", kDebugName );
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
      {
        [this](BehaviorExternalInterface& behaviorExternalInterface) {
          if( !StateExitCooldownExpired(GetStateID("Socializing"), _params.socializeKnownFaceCooldown_s) ) {
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
                  distanceToFace < Util::Square(_params.maxFaceDistanceToSocialize_mm) ) {
                return true;
              }
            }
          }
          return false;
        },
        {
          { VisionMode::DetectingFaces, EVisionUpdateFrequency::Low }
        }
      }
    },
    {
      "WantsToSleep",
      {
        [this](BehaviorExternalInterface& behaviorExternalInterface) {
          if( GetCurrentStateID() != GetStateID("ObservingOnCharger") ) {
            PRINT_NAMED_WARNING("BehaviorHighLevelAI.WantsToSleepCondition.WrongState",
                                "This condition only works from ObservingOnCharger");
            return false;
          }
          
          const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          if( GetLastTimeStarted( GetStateID("ObservingOnCharger") ) + _params.goToSleepTimeout_s <= currTime_s ) {

            // only go to sleep if we haven't recently seen a face
            auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
            Pose3d waste;
            const bool inRobotOriginOnly = false; // might have been picked up
            const TimeStamp_t lastFaceTime = faceWorld.GetLastObservedFace(waste, inRobotOriginOnly);
            const TimeStamp_t lastImgTime = behaviorExternalInterface.GetRobotInfo().GetLastImageTimeStamp();
            if( lastFaceTime < lastImgTime &&
                lastImgTime - lastFaceTime > _params.minFaceAgeToAllowSleep_ms ) {
              return true;
            }
          }
          return false;
        },
        {
          { VisionMode::DetectingFaces, EVisionUpdateFrequency::Low }
        }
      }
    },
    {
      "ChargerLocated",
      {
        [](BehaviorExternalInterface& behaviorExternalInterface) {
          BlockWorldFilter filter;
          filter.SetFilterFcn( [](const ObservableObject* obj){
              return IsCharger(obj->GetType(), false) && obj->IsPoseStateKnown();
            });
          const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
          const auto* block = blockWorld.FindLocatedMatchingObject(filter);
          return block != nullptr;
        },
        {
          { VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low }
        }
      }
    },
    {
      "NeedsToCharge",
      {
        [this](BehaviorExternalInterface& behaviorExternalInterface) {
          const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
          if( !robotInfo.IsCharging() ) {
            const TimeStamp_t lastChargeTime = robotInfo.GetLastChargingStateChangeTimestamp();
            const TimeStamp_t timeSinceNotCharging =
              robotInfo.GetLastMsgTimestamp() > lastChargeTime ?
              robotInfo.GetLastMsgTimestamp() - lastChargeTime :
              0;

            return timeSinceNotCharging >= _params.needsToChargeTime_ms;
          }
          return false;
        },
        {/* Empty Set:: No Vision Requirements*/ }
      }
    },
    {
      "NeedsToLeaveChargerForPlay",
      {
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
                                                                            _params.playWithCubeOnChargerCooldown_s,
                                                                            valueIfNeverRun);
          const auto& timer = GetBEI().GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::PlayingWithCube );
          const bool hasntPlayed = timer.HasCooldownExpired(_params.playWithCubeCooldown_s, valueIfNeverRun);
          
          return hasntDrivenOffChargerForPlay && hasntPlayed;
        },
        {/* Empty Set:: No Vision Requirements*/ }
      }
    },
    {
      "CloseCubeForPlaying",
      {
        [this](BehaviorExternalInterface& behaviorExternalInterface) {

          const auto& timer = GetBEI().GetBehaviorTimerManager().GetTimer( BehaviorTimerTypes::PlayingWithCube );
          if( !timer.HasCooldownExpired(_params.playWithCubeCooldown_s) ) {
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
        },
        {
          { VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low }
        }
      }
    }
  };
}

}
}
