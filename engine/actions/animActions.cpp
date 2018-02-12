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

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotDataLoader.h"


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
        const bool isBlacklisted = std::find(dasBlacklistedTriggers.begin(), dasBlacklistedTriggers.end(), _animTrigger) != dasBlacklistedTriggers.end();
        
        if( res == ActionResult::SUCCESS && !isBlacklisted ) {
          const std::string& dataStr = std::string(AnimationTriggerToString(_animTrigger)) + ":" + _animGroupName;
          Anki::Util::sEvent("robot.play_animation",
                             {{DDATA, dataStr.c_str()}},
                             _animName.c_str());
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
    
    
    TriggerCubeAnimationAction::TriggerCubeAnimationAction(const ObjectID& objectID,
                                                           const CubeAnimationTrigger& trigger)
    : IAction("TriggerCubeAnimation_" + std::string(EnumToString(trigger)),
              RobotActionType::PLAY_CUBE_ANIMATION,
              (u8)AnimTrackFlag::NO_TRACKS)
    , _objectID(objectID)
    , _trigger(trigger)
    {
    
    }
    
    TriggerCubeAnimationAction::~TriggerCubeAnimationAction()
    {
      // If the action has started and the light animation has not ended stop the animation
      if(HasStarted() && !_animEnded)
      {
        GetRobot().GetCubeLightComponent().StopLightAnimAndResumePrevious(_trigger);
      }
    }
    
    ActionResult TriggerCubeAnimationAction::Init()
    {
      // If the animation corresponding to the trigger has no duration we can't play it from an
      // action because then the action would never complete
      if(GetRobot().GetCubeLightComponent().GetAnimDuration(_trigger) == 0)
      {
        PRINT_NAMED_WARNING("TriggerCubeAnimationAction.AnimPlaysForever",
                            "AnimTrigger %s plays forever refusing to play in an action",
                            EnumToString(_trigger));
        _animEnded = true;
        return ActionResult::ABORT;
      }
    
      // Use a private function of CubeLightComponent in order to play on the user/game layer instead of
      // the engine layer
      const bool animPlayed = GetRobot().GetCubeLightComponent().PlayLightAnimFromAction(_objectID,
                                                                                            _trigger,
                                                                                            [this](){_animEnded = true;},
                                                                                            GetTag());
      if(!animPlayed)
      {
        _animEnded = true;
        return ActionResult::ABORT;
      }
      return ActionResult::SUCCESS;
    }
    
    ActionResult TriggerCubeAnimationAction::CheckIfDone()
    {
      return (_animEnded ? ActionResult::SUCCESS : ActionResult::RUNNING);
    }

    PlayNeedsGetOutAnimIfNeeded::PlayNeedsGetOutAnimIfNeeded()
      : Base(AnimationTrigger::Count)
    {
      SetName("PlayNeedsGetOut");
    }
    
    PlayNeedsGetOutAnimIfNeeded::~PlayNeedsGetOutAnimIfNeeded()
    {
      if(HasRobot()){
        auto& severeNeedsComponent = GetRobot().GetAIComponent().GetSevereNeedsComponent();
        if(severeNeedsComponent.HasSevereNeedExpression() && !_hasClearedExpression){
          severeNeedsComponent.ClearSevereNeedExpression();
        }
      }
    }

    ActionResult PlayNeedsGetOutAnimIfNeeded::Init()
    {

      if(GetRobot().GetAIComponent().GetSevereNeedsComponent().HasSevereNeedExpression()) {
        const auto& severeNeedsComponent = GetRobot().GetAIComponent().GetSevereNeedsComponent();
        
        AnimationTrigger animTrigger = AnimationTrigger::Count;        

        switch( severeNeedsComponent.GetSevereNeedExpression() ) {
          case NeedId::Repair: {
            animTrigger = AnimationTrigger::NeedsSevereLowRepairForceGetOut;
            break;
          }
          case NeedId::Energy: {
            animTrigger = AnimationTrigger::NeedsSevereLowEnergyForceGetOut;
            break;
          }
          case NeedId::Play: {
            animTrigger = AnimationTrigger::NeedsSevereLowPlayForceGetOut;
            break;
          }
          case NeedId::Count:
            PRINT_NAMED_ERROR("PlayNeedsGetOutAnimIfNeeded.InconsistentState",
                              "AIWhiteboard said we had a severe needs state, but now says its equal to Count");
            break;
        }

        if( animTrigger != AnimationTrigger::Count ) {
          Base::SetAnimGroupFromTrigger(animTrigger);
          return Base::Init();
        }
        else {
          // we decided not to play something, so this action will immediately success
          return ActionResult::SUCCESS;
        }
        
      }
      else {
        // behavior will succeed automatically in the first CheckIfDone
        PRINT_CH_INFO("Actions", "PlayNeedsGetOutAnimIfNeeded.NoTransitionNeeded",
                      "[%d] %s: no transition animation needed",
                      GetTag(),
                      GetName().c_str());

        return ActionResult::SUCCESS;
      }
    }

    ActionResult PlayNeedsGetOutAnimIfNeeded::CheckIfDone()
    {      
      if( ! HasAnimTrigger() ) {
        // if no animation was required, succeed now
        return ActionResult::SUCCESS;
      }
      else {
          
        const ActionResult result = Base::CheckIfDone();
        if (result == ActionResult::SUCCESS) {
          // No eye popping of momentary severe needs should happen since there is
          // wait timer before idle anims play from AnimationComponent.
          auto& severeNeedsComponent = GetRobot().GetAIComponent().GetSevereNeedsComponent();
          severeNeedsComponent.ClearSevereNeedExpression();
          _hasClearedExpression = true;
        }
        return result;
      }
    }
  
  }
}
