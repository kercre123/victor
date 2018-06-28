/**
 * File: onboardingStageMeetVictor.h
 *
 * Author: ross
 * Created: 2018-06-02
 *
 * Description: Onboarding logic unit for meet victor, which is the same as meet victor except you can edit the name
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageMeetVictor__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageMeetVictor__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/iOnboardingStage.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/externalInterface/externalInterface.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

class OnboardingStageMeetVictor : public IOnboardingStage
{
public:
  virtual ~OnboardingStageMeetVictor() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    // todo: behavior to look around and at user instead of just at user
    delegates.insert( BEHAVIOR_ID(MeetVictor) );
    delegates.insert( BEHAVIOR_ID(OnboardingLookAtUser) );
    delegates.insert( BEHAVIOR_ID(RespondToRenameFace) );
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _selectedBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    // load all behaviors
    _behaviors.clear();
    // todo: for LookingAround, behavior to look around and at user instead of just at user
    _behaviors[Step::LookingAround] = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::MeetVictor]    = GetBehaviorByID( bei, BEHAVIOR_ID(MeetVictor) );
    _behaviors[Step::LookAtUser]    = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::WaitForReScan] = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::RenameFace]    = GetBehaviorByID( bei, BEHAVIOR_ID(RespondToRenameFace) );
    _enrollmentSuccessful = false;
    _step = Step::LookingAround;
    
    // initial behavior
    _selectedBehavior = _behaviors[Step::LookingAround];
    
    SetupConsoleFuncs(bei);
    
    DebugTransition("Waiting on continue to begin");
    // disable trigger word for now
    SetTriggerWordEnabled(false);
  }
  
  virtual bool OnContinue( BehaviorExternalInterface& bei, OnboardingContinueEnum continueNum ) override
  {
    if( _step == Step::LookingAround ) {
      DebugTransition("Waiting on \"my name is\"");
      SetTriggerWordEnabled(true);
      SetAllowedIntent( USER_INTENT(meet_victor) );
    } else if( _step == Step::WaitForReScan ) {
      DebugTransition("Trying to start meet victor again for rescan");
      // restart
      ANKI_VERIFY(!_enrollmentName.empty(), "OnboardingStageMeetVictor.OnContinue.NoName", "Unknown name!");
      const bool sayName = true;
      const bool saveFaceToRobot = true;
      const bool useMusic = false;
      const s32 faceID = Vision::UnknownFaceID;
      ExternalInterface::SetFaceToEnroll setFaceToEnroll(_enrollmentName, faceID, faceID, saveFaceToRobot, sayName, useMusic);
      auto* ei = bei.GetRobotInfo().GetExternalInterface();
      if( ei != nullptr ) {
        ei->Broadcast(ExternalInterface::MessageGameToEngine{std::move(setFaceToEnroll)});
      }
    } else if( _step == Step::LookAtUser ) {
      // done
      DebugTransition("Complete");
      _step = Step::Complete;
      _selectedBehavior = nullptr;
    } else if( _step != Step::Complete ) {
      DEV_ASSERT(false, "OnboardingStageMeetVictor.UnexpectedContinue");
    }
    return true;
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    // all skips move to the next stage
    DebugTransition("Skipping. Complete");
    _selectedBehavior = nullptr;
    _step = Step::Complete;
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::MeetVictor ) {
      _enrollmentEnded = true;
      TryEndingMeetVictor();
    } else if( _step == Step::RenameFace ) {
      TransitionToLookAtFace();
    } else {
      DEV_ASSERT(false, "OnboardingStageMeetVictor.UnexpectedEndToBehavior");
    }
  }
  
  virtual bool OnInterrupted( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    if( (interruptingBehavior == BEHAVIOR_ID(OnboardingPlacedOnCharger)) && !_enrollmentSuccessful ) {
      // resume from the beginning
      DebugTransition("Interruption will resume from the beginning");
      _step = Step::LookingAround;
    }
    // always resume, even if enrollment finished, in case they place the robot on the charger but still
    // want to rename their name
    return false;
  }
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    DebugTransition("Resuming");
    _selectedBehavior = _behaviors[_step];
    const bool triggerEnabled = _step == Step::LookingAround || _step == Step::WaitForReScan;
    SetTriggerWordEnabled( triggerEnabled );
  }
  
  virtual void Update( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::Complete ) {
      return;
    } else if( ((_step == Step::LookingAround) || (_step == Step::WaitForReScan)) && _behaviors[Step::MeetVictor]->WantsToBeActivated() ) {
      // move to meet victor when the voice intent triggers it
      TransitionToMeetVictor();
    } else if( _behaviors[Step::RenameFace]->WantsToBeActivated() ) {
      DebugTransition("Renaming face");
      _step = Step::RenameFace;
      _selectedBehavior = _behaviors[_step];
    }
  }
  
  virtual void GetAdditionalMessages( std::set<EngineToGameTag>& tags ) const override
  {
    tags.insert( EngineToGameTag::FaceEnrollmentCompleted );
  }
  
  virtual void OnMessage( BehaviorExternalInterface& bei, const EngineToGameEvent& event ) override
  {
    if( _step == Step::Complete ) {
      return;
    }
    if( event.GetData().GetTag() == EngineToGameTag::FaceEnrollmentCompleted ) {
      const auto& msg = event.GetData().Get_FaceEnrollmentCompleted();
      _enrollmentName = msg.name;
      if( msg.result == FaceEnrollmentResult::Success ) {
        // now, any interruptions to this stage should consider it as completed
        _enrollmentSuccessful = true;
        _receivedEnrollmentResult = true;
        TryEndingMeetVictor();
      } else if( msg.result != FaceEnrollmentResult::Incomplete ) {
        _enrollmentSuccessful = false;
        _receivedEnrollmentResult = true;
        TryEndingMeetVictor();
      }
    }
  }
  
private:
  
  void TransitionToMeetVictor()
  {
    DebugTransition("Meeting victor");
    _step = Step::MeetVictor;
    _selectedBehavior = _behaviors[_step];
    
    _receivedEnrollmentResult = false;
    _enrollmentEnded = false;
  }
         
  void TransitionToWaitForReScan()
  {
    DebugTransition("Scanning failed. Waiting for Continue to scan again, or another \"my name is\"");
    _step = Step::WaitForReScan;
    _selectedBehavior = _behaviors[_step];
  }
  
  void TransitionToLookAtFace()
  {
    DebugTransition("Finished meeting victor, looking around, waiting for continue or rename face");
    _step = Step::LookAtUser;
    _selectedBehavior = _behaviors[_step];
    SetTriggerWordEnabled( false ); // no more intents for this stage
  }
  
  void TryEndingMeetVictor()
  {
    if( _receivedEnrollmentResult && _enrollmentEnded ) {
      if( _enrollmentSuccessful ) {
        TransitionToLookAtFace();
      } else {
        TransitionToWaitForReScan();
      }
    }
  }
  
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.MeetVictor", "%s", debugInfo.c_str());
  }
  
  void SetupConsoleFuncs(BehaviorExternalInterface& bei)
  {
    // this stage needs a RenameFace console func since that would be sent by the app. It is sent
    // as a message so it gets queued in the same order that it would be queued if the app sent it.
    if( ANKI_DEV_CHEATS ) {
      if( _consoleFuncs.empty() ) {
        auto func = [&bei,this](ConsoleFunctionContextRef context) {
          if( _enrollmentName.empty() ) {
            // probably called before onboarding
            return;
          }
          const char* stateName = ConsoleArg_Get_String(context, "newName");
          ExternalInterface::UpdateEnrolledFaceByID msg;
          msg.faceID = 1; // just assume only one face, since this is onboarding. 0th is invalid, so must be 1
          msg.oldName = _enrollmentName;
          msg.newName = stateName;
          bei.GetRobotInfo().GetExternalInterface()->Broadcast( ExternalInterface::MessageGameToEngine{std::move(msg)} );
        };
        _consoleFuncs.emplace_front( "RenameFace", std::move(func), "Onboarding", "const char* newName" );
      }
    }
  }
  
  enum class Step : uint8_t {
    Invalid=0,
    LookingAround,
    MeetVictor,
    LookAtUser,
    WaitForReScan,
    RenameFace,
    Complete,
  };
  
  Step _step = Step::Invalid;
  IBehavior* _selectedBehavior = nullptr;
  bool _enrollmentSuccessful = false;
  
  bool _receivedEnrollmentResult = false;
  bool _enrollmentEnded = false;
  std::string _enrollmentName;
  
  std::unordered_map<Step, IBehavior*> _behaviors;
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageMeetVictor__

