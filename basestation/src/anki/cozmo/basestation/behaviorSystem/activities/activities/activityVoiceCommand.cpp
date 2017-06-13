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

#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAnimSequence.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/doATrickSelector.h"
#include "anki/cozmo/basestation/behaviorSystem/voiceCommandUtils/requestGameSelector.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/inventoryComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace {

  // Maps a voice command to how many sparks it costs to execute
  const std::map<VoiceCommand::VoiceCommandType, SparkCosts> kVoiceCommandToSparkCostMap = {
    {VoiceCommand::VoiceCommandType::DoATrick, SparkCosts::DoATrick},
    {VoiceCommand::VoiceCommandType::LetsPlay, SparkCosts::PlayAGame},
    {VoiceCommand::VoiceCommandType::FistBump, SparkCosts::FistBump},
    {VoiceCommand::VoiceCommandType::PeekABoo, SparkCosts::PeekABoo},
  };
  
  
}
  
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
  
  const auto& currentCommand = voiceCommandComponent->GetPendingCommand();
  
  // If we already have a behavior assigned we should keep using it, if it's running
  if (_voiceCommandBehavior)
  {
    if (_voiceCommandBehavior->IsRunning())
    {
      return _voiceCommandBehavior;
    }
    // Otherwise this behavior isn't running anymore so clear it and potentially get a new one to use below
    _voiceCommandBehavior = nullptr;
  }
  
  // If we don't have an existing running voice command behavior, execute any waiting tasks
  if (_doneRespondingTask)
  {
    _doneRespondingTask();
    _doneRespondingTask = std::function<void()>{};
  }
  
  const bool isCommandValid = IsCommandValid(currentCommand);
  
  if(isCommandValid)
  {
    voiceCommandComponent->ClearHeardCommand();
  
    if(ShouldCheckNeeds(currentCommand))
    {
      // Check if we should refuse to execute the command due to needs state
      const bool shouldRefuse = CheckRefusalDueToNeeds(robot, _voiceCommandBehavior);
      if(shouldRefuse)
      {
        const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
        BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
        return _voiceCommandBehavior;
      }
    }
    
    // Check if we have enough sparks to execute the command
    if(!HasEnoughSparksForCommand(robot, currentCommand))
    {
      CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
  }
  
  switch (currentCommand)
  {
    case VoiceCommandType::LetsPlay:
    {
      _voiceCommandBehavior = _requestGameSelector->
                        GetNextRequestGameBehavior(robot, currentRunningBehavior);
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoADance:
    {
      _voiceCommandBehavior = _danceBehavior;
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoATrick:
    {
      _doATrickSelector->RequestATrick(robot);
      BeginRespondingToCommand(currentCommand);
      return _behaviorNone;
    }
    case VoiceCommandType::ComeHere:
    {
      BehaviorPreReqRobot preReqData(robot);
      if(_comeHereBehavior->IsRunnable(preReqData)){
        _voiceCommandBehavior = _comeHereBehavior;
      }
      else
      {
        CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      }
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::FistBump:
    {
      BehaviorPreReqRobot preReqRobot(robot);
      //Ensure fist bump is runnable
      if(_fistBumpBehavior->IsRunnable(preReqRobot))
      {
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::FistBump, isSoftSpark);
        _voiceCommandBehavior = _behaviorNone;
      }
      else
      {
        CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      }
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);

      return _voiceCommandBehavior;
    }
    case VoiceCommandType::PeekABoo:
    {
      BehaviorPreReqRobot preReqRobot(robot);
      //Ensure PeekABoo is runnable
      if(_peekABooBehavior->IsRunnable(preReqRobot))
      {
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::PeekABoo, isSoftSpark);
        _voiceCommandBehavior = _behaviorNone;
      }
      else
      {
        CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      }
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
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
      
      BeginRespondingToCommand(currentCommand);
      return _behaviorNone;
    }
    
    // Yes Please and No Thank You do not trigger a behavior here
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
    {
      BeginRespondingToCommand(currentCommand);
      // NOTE INTENTIONAL FALLTHROUGH
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

bool ActivityVoiceCommand::HasEnoughSparksForCommand(Robot& robot, VoiceCommandType command) const
{
  const auto& commandToSparkCost = kVoiceCommandToSparkCostMap.find(command);
  if(commandToSparkCost != kVoiceCommandToSparkCostMap.end())
  {
    int curNumSparks = robot.GetInventoryComponent().GetInventoryAmount(InventoryType::Sparks);
    return (curNumSparks > commandToSparkCost->second);
  }
  
  return true;
}

bool ActivityVoiceCommand::CheckRefusalDueToNeeds(Robot& robot, IBehavior*& outputBehavior) const
{
  BehaviorID whichRefuse = BehaviorID::NoneBehavior;
  Anki::Cozmo::NeedsState& curNeedsState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(curNeedsState.IsNeedAtBracket(NeedId::Repair, NeedBracketId::Critical))
  {
    whichRefuse = BehaviorID::VC_Refuse_Repair;
  }
  else if(curNeedsState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical))
  {
    whichRefuse = BehaviorID::VC_Refuse_Energy;
  }
  
  if(whichRefuse != BehaviorID::NoneBehavior)
  {
    return CheckAndSetupRefuseBehavior(robot, whichRefuse, outputBehavior);
  }
  return false;
}

bool ActivityVoiceCommand::IsCommandValid(VoiceCommand::VoiceCommandType command) const
{
  switch(command)
  {
    case VoiceCommandType::LetsPlay:
    case VoiceCommandType::DoADance:
    case VoiceCommandType::DoATrick:
    case VoiceCommandType::ComeHere:
    case VoiceCommandType::FistBump:
    case VoiceCommandType::PeekABoo:
    case VoiceCommandType::GoToSleep:
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
    {
      return true;
    }
    // These two commands will never be handled by this chooser:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      // We're intentionally not handling these types in ActivityVoiceCommand
      return false;
    }
  }
}

bool ActivityVoiceCommand::ShouldCheckNeeds(VoiceCommand::VoiceCommandType command) const
{
  switch(command)
  {
    case VoiceCommandType::LetsPlay:
    case VoiceCommandType::DoADance:
    case VoiceCommandType::DoATrick:
    case VoiceCommandType::ComeHere:
    case VoiceCommandType::FistBump:
    case VoiceCommandType::PeekABoo:
    {
      return true;
    }

    // These commands are special; we don't care what our needs are when handling them
    case VoiceCommandType::GoToSleep:
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      return false;
    }
  }
}

bool ActivityVoiceCommand::CheckAndSetupRefuseBehavior(Robot& robot,
                                                       BehaviorID whichRefuse,
                                                       IBehavior*& outputBehavior) const
{
  IBehavior* refuseBehavior = robot.GetBehaviorFactory().FindBehaviorByID(whichRefuse);
  DEV_ASSERT(refuseBehavior != nullptr &&
             refuseBehavior->GetClass() == BehaviorClass::PlayAnim,
             "VoiceCommandBehaviorChooser.Refuse.ImproperClassRetrievedForID");
  
  BehaviorPreReqNone preReq;
  if(refuseBehavior->IsRunnable(preReq))
  {
    outputBehavior = refuseBehavior;
    return true;
  }
  return false;
}

void ActivityVoiceCommand::BeginRespondingToCommand(VoiceCommand::VoiceCommandType command, bool trackResponseLifetime)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  
  voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommand(command));
  
  // If we don't care about the lifetime of this command response, just send out the response event
  if (!trackResponseLifetime)
  {
    return;
  }
  
  voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(command));
  DEV_ASSERT_MSG(!_doneRespondingTask,
                 "ActivityVoiceCommand.BeginRespondingToCommand",
                 "The _doneRespondingTask was not cleared and is being overwritten");
  _doneRespondingTask = [voiceCommandComponent, command] ()
  {
    voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandEnd(command));
  };
}

Result ActivityVoiceCommand::Update(Robot& robot)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.Update", "VoiceCommandComponent invalid"))
  {
    return Result::RESULT_FAIL;
  }
  
  // These voice commands don't have behaviors directly associated, so we send out events here when the command is triggered instead
  const auto& currentCommand = voiceCommandComponent->GetPendingCommand();
  if (currentCommand == VoiceCommandType::YesPlease)
  {
    voiceCommandComponent->BroadcastVoiceEvent(UserResponseToPrompt(true));
    voiceCommandComponent->ClearHeardCommand();
  }
  else if (currentCommand == VoiceCommandType::NoThankYou)
  {
    voiceCommandComponent->BroadcastVoiceEvent(UserResponseToPrompt(false));
    voiceCommandComponent->ClearHeardCommand();
  }

  return Result::RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
