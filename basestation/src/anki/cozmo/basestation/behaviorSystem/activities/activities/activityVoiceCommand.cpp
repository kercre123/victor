/**
* File: activityVoiceCommand.cpp
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for responding to voice commands from the user
*
* Copyright: Anki, Inc. 2017
*
**/


#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityVoiceCommand.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/doATrickSelector.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/requestGameSelector.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  

  
ActivityVoiceCommand::~ActivityVoiceCommand() = default;
  
#include "clad/types/voiceCommandTypes.h"
#include "clad/types/unlockTypes.h"

using namespace ExternalInterface;
using namespace ::Anki::Cozmo::VoiceCommand;

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityVoiceCommand::ActivityVoiceCommand(Robot& robot, const Json::Value& config)
:IActivity(robot, config)
, _context(robot.GetContext())
, _voiceCommandBehavior(nullptr)
, _doATrickSelector(new DoATrickSelector(robot))
, _requestGameSelector(new RequestGameSelector(robot))
{
  // TODO: Will need to change how this works should we add more dance behaviors
  _danceBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::Dance_Mambo);
  DEV_ASSERT(_danceBehavior != nullptr &&
             _danceBehavior->GetClass() == BehaviorClass::Dance,
             "VoiceCommandBehaviorChooser.Dance.ImproperClassRetrievedForID");
  
  _comeHereBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::VC_ComeHere);
  DEV_ASSERT(_comeHereBehavior != nullptr &&
             _comeHereBehavior->GetClass() == BehaviorClass::DriveToFace,
             "VoiceCommandBehaviorChooser.ComeHereBehavior.ImproperClassRetrievedForName");
  
  _fistBumpBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FistBump);
  DEV_ASSERT(_fistBumpBehavior != nullptr &&
             _fistBumpBehavior->GetClass() == BehaviorClass::FistBump,
             "VoiceCommandBehaviorChooser.FistBump.ImproperClassRetrievedForID");
  _peekABooBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FPPeekABoo);
  DEV_ASSERT(_peekABooBehavior != nullptr &&
             _peekABooBehavior->GetClass() == BehaviorClass::PeekABoo,
             "VoiceCommandBehaviorChooser.PeekABoo.ImproperClassRetrievedForID");

  DEV_ASSERT(nullptr != _context, "ActivityVoiceCommand.Constructor.NullContext");
}

IBehavior* ActivityVoiceCommand::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.ChooseNextBehavior", "VoiceCommandComponent invalid"))
  {
    return _behaviorNone;
  }
  
  // If we already have a behavior assigned we should keep using it, if it's running
  if (_voiceCommandBehavior)
  {
    if (_voiceCommandBehavior->IsRunning())
    {
      return _voiceCommandBehavior;
    }
    
    voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandEnd(_respondingToCommandType));
    // Otherwise this behavior isn't running anymore so clear it and potentially get a new one to use below
    _voiceCommandBehavior = nullptr;
  }
  
  _respondingToCommandType = voiceCommandComponent->GetPendingCommand();
  if(_respondingToCommandType != VoiceCommandType::HeyCozmo &&
     _respondingToCommandType != VoiceCommandType::Count)
  {
    voiceCommandComponent->ClearHeardCommand();
  }
  
  switch (_respondingToCommandType)
  {
    case VoiceCommandType::LetsPlay:
    {
      _voiceCommandBehavior = _requestGameSelector->
                        GetNextRequestGameBehavior(robot, currentRunningBehavior);
      
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoADance:
    {
      _voiceCommandBehavior = _danceBehavior;
      
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      
      return _voiceCommandBehavior;
      
    }
    case VoiceCommandType::DoATrick:
    {
      _doATrickSelector->RequestATrick(robot);
      voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      return _behaviorNone;
    }
    
    case VoiceCommandType::ComeHere:
    {
      BehaviorPreReqRobot preReqData(robot);
      if(_comeHereBehavior->IsRunnable(preReqData)){
        _voiceCommandBehavior = _comeHereBehavior;
        voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      }
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::FistBump:
    {
      BehaviorPreReqRobot preReqRobot(robot);
      //Ensure fist bump is unlocked and runnable
      if(robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::FistBump) &&
         _fistBumpBehavior->IsRunnable(preReqRobot)){
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::FistBump, isSoftSpark);
        voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      }
      _voiceCommandBehavior = _behaviorNone;
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::GoToSleep:
    {
      // Force execute the PlacedOnCharger/ReactToOnCharger behavior which will send appropriate
      // messages to game triggering the sleep modal
      ExternalInterface::ExecuteReactionTrigger r;
      r.reactionTriggerToBehaviorEntry.behaviorID = BehaviorID::ReactToOnCharger;
      r.reactionTriggerToBehaviorEntry.trigger = ReactionTrigger::PlacedOnCharger;
      ExternalInterface::MessageGameToEngine m;
      m.Set_ExecuteReactionTrigger(std::move(r));
      robot.GetExternalInterface()->Broadcast(std::move(m));
      
      _voiceCommandBehavior = _behaviorNone;
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::PeekABoo:
    {
      BehaviorPreReqRobot preReqRobot(robot);
      //Ensure PeekABoo is unlocked and runnable
      if(robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::PeekABoo) &&
         _peekABooBehavior->IsRunnable(preReqRobot)){
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::PeekABoo, isSoftSpark);
        voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(_respondingToCommandType));
      }
      
      _voiceCommandBehavior = _behaviorNone;
      
      return _voiceCommandBehavior;
    }

    // Yes Please and No Thank You are handled by Unity, so just send up
    // the UserResponseToPrompt message
    case VoiceCommandType::YesPlease:
    {
      VoiceCommandEventUnion vcEvent;
      vcEvent.Set_responseToPrompt(UserResponseToPrompt(true));
      robot.Broadcast(ExternalInterface::MessageEngineToGame(VoiceCommandEvent(vcEvent)));
      return _behaviorNone;
    }
    case VoiceCommandType::NoThankYou:
    {
      VoiceCommandEventUnion vcEvent;
      vcEvent.Set_responseToPrompt(UserResponseToPrompt(false));
      robot.Broadcast(ExternalInterface::MessageEngineToGame(VoiceCommandEvent(vcEvent)));
      return _behaviorNone;
    }
    
    // These two commands will never be handled by this chooser:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      // We're intentionally not handling these types in ActivityVoiceCommand
      return _behaviorNone;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
