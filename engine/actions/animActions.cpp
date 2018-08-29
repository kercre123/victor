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
#include "engine/components/batteryComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
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

#include "webServerProcess/src/webVizSender.h"

namespace Anki {

  namespace Vector {

    namespace {
      static constexpr const char* kManualBodyTrackLockName = "PlayAnimationOnChargerSpecialLock";
    }

    #pragma mark ---- PlayAnimationAction ----

    PlayAnimationAction::PlayAnimationAction(const std::string& animName,
                                             u32 numLoops,
                                             bool interruptRunning,
                                             u8 tracksToLock,
                                             float timeout_sec,
                                             TimeStamp_t startAtTime_ms,
                                             AnimationComponent::AnimationCompleteCallback callback)
    : IAction("PlayAnimation" + animName,
              RobotActionType::PLAY_ANIMATION,
              tracksToLock)
    , _animName(animName)
    , _numLoopsRemaining(numLoops)
    , _interruptRunning(interruptRunning)
    , _timeout_sec(timeout_sec)
    , _startAtTime_ms(startAtTime_ms)
    , _passedInCallback(callback)
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
        if (HasRobot()) {
          GetRobot().GetAnimationComponent().StopAnimByName(_animName);          
        } else {
          // This shouldn't happen if HasStarted()...
          PRINT_NAMED_WARNING("PlayAnimationAction.Dtor.NoRobot", "");
        }
      }

      if (HasStarted() && _bodyTrackManuallyLocked ) {
        GetRobot().GetMoveComponent().UnlockTracks( (u8) AnimTrackFlag::BODY_TRACK, kManualBodyTrackLockName );
        _bodyTrackManuallyLocked = false;
      }
    }

    void PlayAnimationAction::OnRobotSet()
    {
      OnRobotSetInternalAnim();
    }

    void PlayAnimationAction::InitTrackLockingForCharger()
    {
      if( _bodyTrackManuallyLocked ) {
        // already locked, nothing to do
        return;
      }

      if( GetRobot().GetBatteryComponent().IsOnChargerPlatform() ) {
        // default here is now to LOCK the body track, but first check the whitelist

        auto* dataLoader = GetRobot().GetContext()->GetDataLoader();
        const auto& whitelist = dataLoader->GetAllWhitelistedChargerAnimationClips();
        if( whitelist.find(_animName) == whitelist.end() ) {

          // time to lock the body track. Unfortunately, the action has already been Init'd, so it's tracks
          // are already locked. Therefore we have to manually lock the body to make this work
          GetRobot().GetMoveComponent().LockTracks( (u8) AnimTrackFlag::BODY_TRACK,
                                                        kManualBodyTrackLockName,
                                                        "PlayAnimationAction.LockBodyOnCharger" );
          _bodyTrackManuallyLocked = true;

          PRINT_CH_DEBUG("Animations", "PlayAnimationAction.LockingBodyOnCharger",
                         "anim '%s' is not in the whitelist, locking the body track",
                         _animName.c_str());
        }
      }
    }
  
    ActionResult PlayAnimationAction::Init()
    {

      InitTrackLockingForCharger();

      _stoppedPlaying = false;
      _wasAborted = false;

      auto callback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded) {
        _stoppedPlaying = true;
        if (res != AnimationComponent::AnimResult::Completed) {
          _wasAborted = true;
        }
      };

      Result res = GetRobot().GetAnimationComponent().PlayAnimByName(_animName, _numLoopsRemaining,
                                                                     _interruptRunning, callback, GetTag(), 
                                                                     _timeout_sec, _startAtTime_ms, _renderInEyeHue);

      if(res != RESULT_OK) {
        _stoppedPlaying = true;
        _wasAborted = true;
        return ActionResult::ANIM_ABORTED;
      }else if(_passedInCallback != nullptr){
        const bool callEvenIfAnimCanceled = true;
        GetRobot().GetAnimationComponent().AddAdditionalAnimationCallback(_animName, _passedInCallback, callEvenIfAnimCanceled);
      }

      GetRobot().GetComponent<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::AnimationPlayed);

      InitSendStats();

      return ActionResult::SUCCESS;
    }

    void PlayAnimationAction::InitSendStats()
    {
      // NOTE: this is overridden most of the time by TriggerAnimationAction
      SendStatsToDasAndWeb(_animName, "", AnimationTrigger::Count);
    }


    void PlayAnimationAction::SendStatsToDasAndWeb(const std::string& animClipName,
                                                   const std::string& animGroupName,
                                                   const AnimationTrigger& animTrigger)
    {
      const auto simpleMood = GetRobot().GetMoodManager().GetSimpleMood();
      const float headAngle_deg = Util::RadToDeg(GetRobot().GetComponent<FullRobotPose>().GetHeadAngle());

      if( animTrigger != AnimationTrigger::Count ) {
        auto* dataLoader = GetRobot().GetContext()->GetDataLoader();
        const std::set<AnimationTrigger>& dasBlacklistedTriggers = dataLoader->GetDasBlacklistedAnimationTriggers();

        const bool isBlacklisted = dasBlacklistedTriggers.find(animTrigger) != dasBlacklistedTriggers.end();

        if( !isBlacklisted ) {
          // NOTE: you can add events to the blacklist in das_event_config.json to block them from sending
          // here.

          DASMSG(action_play_animation, "action.play_animation",
                 "An animation action has been started on the robot (that wasn't blacklisted for DAS)");
          DASMSG_SET(s1, animClipName, "The animation clip name");
          DASMSG_SET(s2, animGroupName, "The animation group name");
          DASMSG_SET(s3, AnimationTriggerToString(animTrigger), "The animation trigger name");
          DASMSG_SET(s4, EnumToString(simpleMood), "The current SimpleMood value");
          DASMSG_SET(i1, std::round(headAngle_deg), "The current head angle (in degrees)");
          DASMSG_SEND();
        }
      }


      if( auto webSender = WebService::WebVizSender::CreateWebVizSender("animationengine",
                             GetRobot().GetContext()->GetWebService()) ) {
        webSender->Data()["clip"] = animClipName;
        webSender->Data()["group"] = animGroupName;
        if( animTrigger != AnimationTrigger::Count ) {
          webSender->Data()["trigger"] = AnimationTriggerToString(animTrigger);
        }
        webSender->Data()["mood"] = EnumToString(simpleMood);
        webSender->Data()["headAngle_deg"] = headAngle_deg;
      }
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

    void TriggerAnimationAction::OnRobotSetInternalAnim()
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
        return res;
      }
    }

    void TriggerAnimationAction::InitSendStats()
    {
      SendStatsToDasAndWeb(_animName, _animGroupName, _animTrigger);
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
    
    #pragma mark ---- ReselectingLoopAnimationAction ----

    ReselectingLoopAnimationAction::ReselectingLoopAnimationAction(AnimationTrigger animEvent,
                                                                   u32 numLoops,
                                                                   bool interruptRunning,
                                                                   u8 tracksToLock,
                                                                   float timeout_sec,
                                                                   bool strictCooldown)
      : IAction(GetDebugString(animEvent),
                RobotActionType::RESELECTING_LOOP_ANIMATION,
                tracksToLock)
      , _numLoops( numLoops )
      , _loopForever( 0 == numLoops )
      , _numLoopsRemaining( numLoops )
    {
      _animParams.animEvent = animEvent;
      _animParams.interruptRunning = interruptRunning;
      _animParams.tracksToLock = tracksToLock;
      _animParams.timeout_sec = timeout_sec;
      _animParams.strictCooldown = strictCooldown;
      
      constexpr f32 defaultTimeout_s = PlayAnimationAction::GetDefaultTimeoutInSeconds();
      if( (numLoops == 0) && (_animParams.timeout_sec == defaultTimeout_s) ) {
        _animParams.timeout_sec = PlayAnimationAction::GetInfiniteTimeoutInSeconds();
      }
    }
    
    std::string ReselectingLoopAnimationAction::GetDebugString(const AnimationTrigger& trigger)
    {
      return std::string{"ReselectingLoopAnimationAction"} + AnimationTriggerToString(trigger);
    }
    
    ReselectingLoopAnimationAction::~ReselectingLoopAnimationAction() {
      if( _subAction != nullptr ) {
        _subAction->PrepForCompletion();
      }
    }
    
    void ReselectingLoopAnimationAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
    {
      if( _subAction != nullptr ) {
        _subAction->GetCompletionUnion( completionUnion );
      }
    }

    ActionResult ReselectingLoopAnimationAction::Init() {
      ResetSubAction();
      _numLoopsRemaining = _numLoops;
      _loopForever = (0 == _numLoops);
      return ActionResult::SUCCESS;
    }
    
    void ReselectingLoopAnimationAction::ResetSubAction() {
      if( _subAction != nullptr ) {
        _subAction->PrepForCompletion();
      }
      _subAction.reset( new TriggerLiftSafeAnimationAction{_animParams.animEvent,
                                                           1, // only one loop here!
                                                           _animParams.interruptRunning,
                                                           _animParams.tracksToLock,
                                                           _animParams.timeout_sec,
                                                           _animParams.strictCooldown} );
      _subAction->SetRobot( &GetRobot() );
    }
    
    ActionResult ReselectingLoopAnimationAction::CheckIfDone()
    {
      if( _subAction == nullptr ) {
        return ActionResult::NULL_SUBACTION;
      }
      
      ActionResult subActionResult = _subAction->Update();
      const ActionResultCategory category = IActionRunner::GetActionResultCategory(subActionResult);
      
      if( (category == ActionResultCategory::SUCCESS)
          && (_loopForever || (--_numLoopsRemaining > 0)) )
      {
        ResetSubAction();
        return ActionResult::RUNNING;
      } else {
        return subActionResult;
      }
    }
    
    void ReselectingLoopAnimationAction::StopAfterNextLoop()
    {
      if( _numLoopsRemaining > 1 ) {
        _numLoopsRemaining = 1;
      }
      _loopForever = false;
    }


  }
}
