/**
 * File: BehaviorPlayMusic.cpp
 *
 * Author: ross
 * Created: 2018-09-06
 *
 * Description: plays online music
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorPlayMusic.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/moodSystem/moodManager.h"


namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayMusic::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayMusic::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayMusic::BehaviorPlayMusic(const Json::Value& config)
 : ICozmoBehavior(config)
{
  std::set<RobotInterface::RobotToEngineTag> tags = {
    RobotInterface::RobotToEngineTag::onlineMusicReady,
    RobotInterface::RobotToEngineTag::onlineMusicComplete,
  };
  SubscribeToTags(std::move(tags));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlayMusic::~BehaviorPlayMusic()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlayMusic::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntent intent;
  const bool playMusic = uic.IsUserIntentPending(USER_INTENT(play_music));
  const bool playAnyMusic = uic.IsUserIntentPending(USER_INTENT(play_any_music));
  return playMusic || playAnyMusic;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false; // todo: change 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsUserIntentPending(USER_INTENT(play_music)) ) {
    UserIntentPtr intentData = SmartActivateUserIntent(USER_INTENT(play_music));
    _dVars.request = intentData->intent.Get_play_music().request;
    if( _dVars.request == "NPR" ) {
      _dVars.request = "KQED";
    }
  } else {
    UserIntentPtr intentData = SmartActivateUserIntent(USER_INTENT(play_any_music));
    static const std::vector<std::string> genres = {
//      "smooth jazz",
//      "classic rock",
      "KQED",
//      "dance music",
    };
    const auto idx = GetBEI().GetRNG().RandInt((int)genres.size());
    _dVars.request = genres[idx];
  }
  
  PRINT_NAMED_WARNING("WHATNOW", "Activated! request=%s", _dVars.request.c_str());
  
  // request cloud to check if stream is available
  {
    RobotInterface::OnlineMusicRequest requestMsg;
    requestMsg.request = _dVars.request;
    RobotInterface::EngineToRobot wrapper(std::move(requestMsg));
    GetBEI().GetRobotInfo().SendRobotMessage( wrapper );
  }
  
  FixStimAtMax();
  // todo: void UnFixStim();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  const bool offTreads = GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads;
  if( offTreads ) {
    // robot was picked up. stop
    RobotInterface::OnlineMusicStop stopMsg;
    stopMsg.request = _dVars.request;
    RobotInterface::EngineToRobot wrapper(std::move(stopMsg));
    GetBEI().GetRobotInfo().SendRobotMessage( wrapper );
    
    CancelSelf();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  // do this when not activated to send messages to anim as needed, but don't react if not active
  const auto& msg = event.GetData();
  if( msg.GetTag() == RobotInterface::RobotToEngineTag::onlineMusicReady ) {
    if( IsActivated() ) {
      if( msg.Get_onlineMusicReady().ready ) {
        // request play
        RobotInterface::OnlineMusicPlay playMsg;
        playMsg.request = msg.Get_onlineMusicReady().request;
        RobotInterface::EngineToRobot wrapper(std::move(playMsg));
        GetBEI().GetRobotInfo().SendRobotMessage( wrapper );
      }
    } else if( msg.Get_onlineMusicReady().ready ) {
      // unexpected ready when not active. send stop music
      RobotInterface::OnlineMusicStop stopMsg;
      stopMsg.request = msg.Get_onlineMusicReady().request;
      RobotInterface::EngineToRobot wrapper(std::move(stopMsg));
      GetBEI().GetRobotInfo().SendRobotMessage( wrapper );
    }
  } else if( msg.GetTag() == RobotInterface::RobotToEngineTag::onlineMusicComplete ) {
    // stream ended
    if( IsActivated() ) {
      CancelSelf();
    }
  }
}
  
void BehaviorPlayMusic::FixStimAtMax()
{
  if( GetBEI().HasMoodManager() && !_dVars.isStimMaxed ) {
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("OnboardingStarted");
    moodManager.SetEmotionFixed( EmotionType::Stimulated, true );
    _dVars.isStimMaxed = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlayMusic::UnFixStim()
{
  if( GetBEI().HasMoodManager() && _dVars.isStimMaxed ) {
    auto& moodManager = GetBEI().GetMoodManager();
    // since this behavior is always at the base of the stack (todo: unit test this), just set it fixed==false
    moodManager.SetEmotionFixed( EmotionType::Stimulated, false );
    _dVars.isStimMaxed = false;
  }
}

}
}
