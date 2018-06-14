/**
 * File: animActions.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements animation and audio cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/types/behaviorComponent/behaviorStats.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/logging.h"
#include "util/logging/DAS.h"

namespace Anki {

  namespace Cozmo {

    #pragma mark ---- PlayAnimationAction ----

    PlayAnimationAction::PlayAnimationAction(const std::string& animName,
                                             u32 numLoops,
                                             bool interruptRunning,
                                             u8 tracksToLock,
                                             float timeout_sec)
    : IAction("PlayAnimation" + animName,
              RobotActionType::PLAY_ANIMATION,
              tracksToLock)
    , _animName(animName)
    , _numLoopsRemaining(numLoops)
    , _interruptRunning(interruptRunning)
    , _timeout_sec(timeout_sec)
    {
      // If an animation is supposed to loop infinitely, it should have a
      // much longer default timeout
      if((numLoops == 0) &&
         (timeout_sec == _kDefaultTimeout_sec)){
        _timeout_sec = _kDefaultTimeoutForInfiniteLoops_sec;
      }

    }

    PlayAnimationAction::~PlayAnimationAction()
    {
      if (HasStarted() && !_stoppedPlaying) {
        PRINT_NAMED_INFO("PlayAnimationAction.Destructor.StillStreaming",
                         "Action destructing, but AnimationComponent is still playing: %s. Telling it to stop.",
                         _animName.c_str());
        GetRobot().GetAnimationComponent().StopAnimByName(_animName);
      }
    }

    ActionResult PlayAnimationAction::Init()
    {
      _stoppedPlaying = false;
      _wasAborted = false;

      auto callback = [this](const AnimationComponent::AnimResult res) {
        _stoppedPlaying = true;
        if (res != AnimationComponent::AnimResult::Completed) {
          _wasAborted = true;
        }
      };

      Result res = GetRobot().GetAnimationComponent().PlayAnimByName(_animName, _numLoopsRemaining, _interruptRunning, callback, GetTag(), _timeout_sec);

      if(res != RESULT_OK) {
        _stoppedPlaying = true;
        _wasAborted = true;
        return ActionResult::ANIM_ABORTED;
      }

      GetRobot().GetComponent<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::BehaviorActived);

      return ActionResult::SUCCESS;
    }

    ActionResult PlayAnimationAction::CheckIfDone()
    {
      if(_stoppedPlaying) {
        return ActionResult::SUCCESS;
      } else if(_wasAborted) {
        return ActionResult::ANIM_ABORTED;
      } else {
        return ActionResult::RUNNING;
      }
    }

    void PlayAnimationAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      AnimationCompleted info;
      info.animationName = _animName;
      completionUnion.Set_animationCompleted(std::move( info ));
    }

    #pragma mark ---- TriggerAnimationAction ----

    TriggerAnimationAction::TriggerAnimationAction(AnimationTrigger animEvent,
                                                   u32 numLoops,
                                                   bool interruptRunning,
                                                   u8 tracksToLock,
                                                   float timeout_sec,
                                                   bool strictCooldown)
    : PlayAnimationAction("", numLoops, interruptRunning, tracksToLock, timeout_sec)
    , _animTrigger(animEvent)
    , _animGroupName("")
    , _strictCooldown(strictCooldown)
    {
      SetName("PlayAnimation" + _animGroupName);
      // will FAILURE_ABORT on Init if not an event
    }

    void TriggerAnimationAction::OnRobotSet()
    {
      SetAnimGroupFromTrigger(_animTrigger);
      OnRobotSetInternalTrigger();
    }

    void TriggerAnimationAction::SetAnimGroupFromTrigger(AnimationTrigger animTrigger)
    {
      _animTrigger = animTrigger;

      auto* data_ldr = GetRobot().GetContext()->GetDataLoader();
      if( data_ldr->HasAnimationForTrigger(_animTrigger) )
      {
        _animGroupName = data_ldr->GetAnimationForTrigger(_animTrigger);
        if(_animGroupName.empty()) {
          PRINT_NAMED_WARNING("TriggerAnimationAction.EmptyAnimGroupNameForTrigger",
                              "Event: %s", EnumToString(_animTrigger));
        }
      }

    }

    ActionResult TriggerAnimationAction::Init()
    {
      if(_animGroupName.empty())
      {
        PRINT_NAMED_WARNING("TriggerAnimationAction.NoAnimationForTrigger",
                            "Event: %s", EnumToString(_animTrigger));

        return ActionResult::NO_ANIM_NAME;
      }

      _animName = GetRobot().GetAnimationComponent().GetAnimationNameFromGroup(_animGroupName, _strictCooldown);

      if( _animName.empty() ) {
        return ActionResult::NO_ANIM_NAME;
      }
      else {
        const ActionResult res = PlayAnimationAction::Init();

        auto* dataLoader = GetRobot().GetContext()->GetDataLoader();
        const std::set<AnimationTrigger>& dasBlacklistedTriggers = dataLoader->GetDasBlacklistedAnimationTriggers();
        const bool isBlacklisted = std::find(dasBlacklistedTriggers.begin(), dasBlacklistedTriggers.end(), _animTrigger)
          != dasBlacklistedTriggers.end();

        if( res == ActionResult::SUCCESS && !isBlacklisted ) {
          const auto simpleMood = GetRobot().GetMoodManager().GetSimpleMood();
          const float headAngle_deg = Util::RadToDeg(GetRobot().GetComponent<FullRobotPose>().GetHeadAngle());

          // NOTE: you can add events to the blacklist in das_event_config.json to block them from sending here

          DASMSG(action_play_animation, "action.play_animation",
                 "An animation has been started on the robot (that wasn't blacklisted for DAS)");
          DASMSG_SET(s1, _animName, "The animation clip name");
          DASMSG_SET(s2, _animGroupName, "The animation group name");
          DASMSG_SET(s3, AnimationTriggerToString(_animTrigger), "The animation trigger name");
          DASMSG_SET(s4, EnumToString(simpleMood), "The current SimpleMood value");
          DASMSG_SET(i1, std::round(headAngle_deg), "The current head angle (in degrees)");
          DASMSG_SEND();
        }

        return res;
      }
    }


    #pragma mark ---- TriggerLiftSafeAnimationAction ----

    TriggerLiftSafeAnimationAction::TriggerLiftSafeAnimationAction(AnimationTrigger animEvent,
                                                                   u32 numLoops,
                                                                   bool interruptRunning,
                                                                   u8 tracksToLock,
                                                                   float timeout_sec,
                                                                   bool strictCooldown)
    : TriggerAnimationAction(animEvent, numLoops, interruptRunning, tracksToLock, timeout_sec, strictCooldown)
    {
    }


    u8 TriggerLiftSafeAnimationAction::TracksToLock(Robot& robot, u8 tracksCurrentlyLocked)
    {

      // Ensure animation doesn't throw cube down, but still can play get down animations
      if(robot.GetCarryingComponent().IsCarryingObject()
         && robot.GetOffTreadsState() == OffTreadsState::OnTreads){
        tracksCurrentlyLocked = tracksCurrentlyLocked | (u8) AnimTrackFlag::LIFT_TRACK;
      }

      return tracksCurrentlyLocked;
    }

    void TriggerLiftSafeAnimationAction::OnRobotSetInternalTrigger()
    {
      SetTracksToLock(TracksToLock(GetRobot(), GetTracksToLock()));
    }

  }
}
