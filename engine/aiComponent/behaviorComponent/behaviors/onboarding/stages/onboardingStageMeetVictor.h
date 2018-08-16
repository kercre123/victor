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
#include "proto/external_interface/onboardingSteps.pb.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

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
    _behaviors[Step::LookingAround]  = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::MeetVictor]     = GetBehaviorByID( bei, BEHAVIOR_ID(MeetVictor) );
    _behaviors[Step::SuccessfulScan] = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::WaitForReScan]  = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    _behaviors[Step::RenameFace]     = GetBehaviorByID( bei, BEHAVIOR_ID(RespondToRenameFace) );
    _enrollmentSuccessful = false;
    _step = Step::LookingAround;
    _receivedStartMsg = false;
    _enrollmentName.clear();
    _enrollmentEnded = false;
    _receivedEnrollmentResult = false;
    
    // initial behavior
    _selectedBehavior = _behaviors[Step::LookingAround];
    
    SetupConsoleFuncs(bei);
    
    DebugTransition("Waiting on continue to begin");
    // disable trigger word for now
    SetTriggerWordEnabled(false);
  }
  
  virtual bool OnContinue( BehaviorExternalInterface& bei, int stepNum ) override
  {
    bool accepted = false;
    if( _step == Step::LookingAround ) {
      accepted = (stepNum == external_interface::STEP_EXPECTING_CONTINUE_MEET_VECTOR);
      if( accepted ) {
        DebugTransition("Waiting on \"my name is\"");
        SetTriggerWordEnabled(true);
        SetAllowedIntent( USER_INTENT(meet_victor) );
        _receivedStartMsg = true;
      }
    } else if( _step == Step::WaitForReScan ) {
      accepted = (stepNum == external_interface::STEP_EXPECTING_RESCAN);
      if( accepted ) {
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
      }
    } else if( _step == Step::SuccessfulScan ) {
      accepted = (stepNum == external_interface::STEP_EXPECTING_END_MEET_VECTOR);
      if( accepted ) {
        // done
        DebugTransition("Complete");
        _step = Step::Complete;
        _selectedBehavior = nullptr;
      }
    } else if( _step != Step::Complete ) {
      DEV_ASSERT(false, "OnboardingStageMeetVictor.UnexpectedContinue");
    }
    return accepted;
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
      // todo: fix if renameFace is brought back to life
      TransitionToSuccessfulScan();
    } else {
      DEV_ASSERT(false, "OnboardingStageMeetVictor.UnexpectedEndToBehavior");
    }
  }
  
  virtual bool OnInterrupted( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    ResetFromInterruption(interruptingBehavior);
    // always resume, even if enrollment finished, in case they place the robot on the charger but still
    // want to rename their name
    return false;
  }
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    ResetFromInterruption(interruptingBehavior);
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
  
  virtual int GetExpectedStep() const override
  {
    switch( _step ) {
      case Step::LookingAround:
        return _receivedStartMsg
               ? external_interface::STEP_EXPECTING_MEET_VECTOR
               : external_interface::STEP_EXPECTING_CONTINUE_MEET_VECTOR;
      case Step::MeetVictor:
        return external_interface::STEP_MEET_VECTOR;
      case Step::SuccessfulScan:
        return external_interface::STEP_EXPECTING_END_MEET_VECTOR;
      case Step::WaitForReScan:
        return external_interface::STEP_EXPECTING_RESCAN;
      case Step::Invalid:
      case Step::Complete:
      case Step::RenameFace: // valid case, but no longer used in onboarding. keeping this here in case it is brought back
        return external_interface::STEP_INVALID;
    }
  }
  
  virtual void GetAdditionalMessages( std::set<EngineToGameTag>& tags ) const override
  {
    tags.insert( EngineToGameTag::FaceEnrollmentCompleted );
    tags.insert( EngineToGameTag::MeetVictorFaceScanStarted );
    tags.insert( EngineToGameTag::MeetVictorNameSaved );
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
    } else if( event.GetData().GetTag() == EngineToGameTag::MeetVictorFaceScanStarted ) {
      SetTriggerWordEnabled( false );
    } else if( event.GetData().GetTag() == EngineToGameTag::MeetVictorNameSaved ) {
      _enrollmentSuccessful = true;
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
  
  void TransitionToSuccessfulScan()
  {
    DebugTransition("Finished meeting victor, looking around, waiting for continue or rename face");
    _step = Step::SuccessfulScan;
    _selectedBehavior = _behaviors[_step];
    SetTriggerWordEnabled( false ); // no more intents for this stage
  }
  
  void TryEndingMeetVictor()
  {
    if( _receivedEnrollmentResult && _enrollmentEnded ) {
      if( _enrollmentSuccessful ) {
        TransitionToSuccessfulScan();
      } else {
        TransitionToWaitForReScan();
      }
    }
  }
  
  void ResetFromInterruption(BehaviorID interruptingBehavior)
  {
    static const std::set<BehaviorID> interruptionsThatReset = {
      //BEHAVIOR_ID(SingletonPoweringRobotOff)
      //BEHAVIOR_ID(OnboardingPhysicalReactions)
      BEHAVIOR_ID(OnboardingLowBattery),
      //BEHAVIOR_ID(OnboardingDetectHabitat)
      //BEHAVIOR_ID(OnboardingPickedUp),
      BEHAVIOR_ID(DriveOffChargerStraight),
      BEHAVIOR_ID(OnboardingPlacedOnCharger),
      //BEHAVIOR_ID(OnboardingFirstTriggerWord)
      //BEHAVIOR_ID(TriggerWordDetected),
    };
    
    if( (interruptionsThatReset.find(interruptingBehavior) != interruptionsThatReset.end()) && !_enrollmentSuccessful ) {
      // resume from the beginning
      DebugTransition("Interruption will resume from the beginning");
      _step = Step::LookingAround;
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
    SuccessfulScan,
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
  bool _receivedStartMsg = false;
  
  std::unordered_map<Step, IBehavior*> _behaviors;
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageMeetVictor__

