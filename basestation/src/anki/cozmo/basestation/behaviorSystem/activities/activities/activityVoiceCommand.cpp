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
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAnimSequence.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
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
  const std::map<VoiceCommand::VoiceCommandType, SparkableThings> kVoiceCommandToSparkableThingsMap = {
    {VoiceCommand::VoiceCommandType::DoATrick, SparkableThings::DoATrick},
    {VoiceCommand::VoiceCommandType::LetsPlay, SparkableThings::PlayAGame},
    {VoiceCommand::VoiceCommandType::FistBump, SparkableThings::FistBump},
    {VoiceCommand::VoiceCommandType::PeekABoo, SparkableThings::PeekABoo},
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
  _danceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::Dance_Mambo);
  DEV_ASSERT(_danceBehavior != nullptr &&
             _danceBehavior->GetClass() == BehaviorClass::Dance,
             "VoiceCommandBehaviorChooser.Dance.ImproperClassRetrievedForID");
  
  _driveToFaceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::VC_ComeHere);
  DEV_ASSERT(_driveToFaceBehavior != nullptr &&
             _driveToFaceBehavior->GetClass() == BehaviorClass::DriveToFace,
             "VoiceCommandBehaviorChooser.ComeHereBehavior.ImproperClassRetrievedForName");
  
  _searchForFaceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::VC_SearchForFace);
  DEV_ASSERT(_searchForFaceBehavior != nullptr &&
             _searchForFaceBehavior->GetClass() == BehaviorClass::SearchForFace,
             "VoiceCommandBehaviorChooser.SearchForFaceBehavior.ImproperClassRetrievedForName");
  
  _fistBumpBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FistBump);
  DEV_ASSERT(_fistBumpBehavior != nullptr &&
             _fistBumpBehavior->GetClass() == BehaviorClass::FistBump,
             "VoiceCommandBehaviorChooser.FistBump.ImproperClassRetrievedForID");

  _peekABooBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FPPeekABoo);
  DEV_ASSERT(_peekABooBehavior != nullptr &&
             _peekABooBehavior->GetClass() == BehaviorClass::PeekABoo,
             "VoiceCommandBehaviorChooser.PeekABoo.ImproperClassRetrievedForID");
  
  _laserBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::VC_TrackLaser);
  DEV_ASSERT(_laserBehavior != nullptr &&
             _laserBehavior->GetClass() == BehaviorClass::TrackLaser,
             "VoiceCommandBehaviorChooser.Laser.ImproperClassRetrievedForID");
  
  _pounceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::VC_PounceOnMotion);
  DEV_ASSERT(_pounceBehavior != nullptr &&
             _pounceBehavior->GetClass() == BehaviorClass::PounceOnMotion,
             "VoiceCommandBehaviorChooser.PounceOnMotion.ImproperClassRetrievedForID");
  
  //Create an arbitrary animation behavior
  IBehaviorPtr playAnimPtr = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::PlayArbitraryAnim);
  _playAnimBehavior = std::static_pointer_cast<BehaviorPlayArbitraryAnim>(playAnimPtr);
  
  DEV_ASSERT(_playAnimBehavior != nullptr &&
             _playAnimBehavior->GetClass() == BehaviorClass::PlayArbitraryAnim,
             "VoiceCommandBehaviorChooser.BehaviorPlayAnimPointerNotSet");

  DEV_ASSERT(nullptr != _context, "ActivityVoiceCommand.Constructor.NullContext");
  
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::BehaviorObjectiveAchieved>();
  }
}


IBehaviorPtr ActivityVoiceCommand::ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr emptyPtr;
  
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.ChooseNextBehavior", "VoiceCommandComponent invalid"))
  {
    return emptyPtr;
  }
  
  // Check to see if we should update the _voiceCommandBehavior should we be trying to track a laser
  const bool didUpdateLaserBehavior = CheckForLaserTrackingChange(robot, currentRunningBehavior);
  if(didUpdateLaserBehavior)
  {
    return _voiceCommandBehavior;
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
      RemoveSparksForCommand(robot, currentCommand);
      
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

      RemoveSparksForCommand(robot, currentCommand);

      return emptyPtr;
    }
    case VoiceCommandType::ComeHere:
    {
      BehaviorPreReqRobot preReqData(robot);
      if(_driveToFaceBehavior->IsRunnable(preReqData)){
        _voiceCommandBehavior = _driveToFaceBehavior;
      }
      else
      {
        BehaviorPreReqRobot preReqData(robot);
        if(ANKI_VERIFY(_searchForFaceBehavior->IsRunnable(preReqData),
                       "ActivityVoiceCommand.ChooseNextBehaviorInternal.SearchForFaceNotRunnable",
                       "No way to respond to the voice command")){
          _voiceCommandBehavior = _searchForFaceBehavior;
        }
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
        RemoveSparksForCommand(robot, currentCommand);
      
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::FistBump, isSoftSpark);
        _voiceCommandBehavior = emptyPtr;
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
        RemoveSparksForCommand(robot, currentCommand);
      
        const bool isSoftSpark = false;
        robot.GetBehaviorManager().SetRequestedSpark(UnlockId::PeekABoo, isSoftSpark);
        _voiceCommandBehavior = emptyPtr;
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
      return emptyPtr;
    }
    case VoiceCommandType::LookDown:
    {
      BehaviorPreReqRobot preReqRobot(robot);
      if(_laserBehavior->IsRunnable(preReqRobot))
      {
        _lookDownState = LookDownState::WAITING_FOR_LASER;
        _voiceCommandBehavior = _laserBehavior;
      }
      else
      {
        CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      }
      
      const bool& shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    
    // These commands will never be handled by this chooser:
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::NoThankYou:
    case VoiceCommandType::Continue:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Count:
    {
      // We're intentionally not handling these types in ActivityVoiceCommand
      return emptyPtr;
    }
  }
}

bool ActivityVoiceCommand::HasEnoughSparksForCommand(Robot& robot, VoiceCommandType command) const
{
  const auto& commandToSparkCost = kVoiceCommandToSparkableThingsMap.find(command);
  if(commandToSparkCost != kVoiceCommandToSparkableThingsMap.end())
  {
    int curNumSparks = robot.GetInventoryComponent().GetInventoryAmount(InventoryType::Sparks);
    const u32 sparkCost = GetSparkCosts(commandToSparkCost->second, 0);
    return (curNumSparks >= sparkCost);
  }
  
  return true;
}

void ActivityVoiceCommand::RemoveSparksForCommand(Robot& robot, VoiceCommandType command)
{
  const auto& commandToSparkCost = kVoiceCommandToSparkableThingsMap.find(command);
  if(commandToSparkCost != kVoiceCommandToSparkableThingsMap.end())
  {
    const u32 sparkCost = GetSparkCosts(commandToSparkCost->second, 0);
    robot.GetInventoryComponent().AddInventoryAmount(InventoryType::Sparks, -sparkCost);
  }
  else
  {
    PRINT_NAMED_WARNING("ActivityVoiceCommand.RemoveSparksForCommand.InvalidCommand",
                        "Command %s was not found in kVoiceCommandToSparkCostMap",
                        EnumToString(command));
  }
}

bool ActivityVoiceCommand::CheckRefusalDueToNeeds(Robot& robot, IBehaviorPtr& outputBehavior) const
{
  Anki::Cozmo::NeedsState& curNeedsState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(curNeedsState.IsNeedAtBracket(NeedId::Repair, NeedBracketId::Critical))
  {
    BehaviorID whichRefuse = BehaviorID::VC_Refuse_Repair;
    return CheckAndSetupRefuseBehavior(robot, whichRefuse, outputBehavior);
  }
  else if(curNeedsState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical))
  {
    BehaviorID whichRefuse = BehaviorID::VC_Refuse_Energy;
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

    case VoiceCommandType::LookDown:
    {
      return true;
    }
    // These commands will never be handled by this chooser:
    case VoiceCommandType::Continue:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::NoThankYou:
    case VoiceCommandType::YesPlease:
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
    case VoiceCommandType::LookDown:
    {
      return true;
    }

    // These commands are special; we don't care what our needs are when handling them
    case VoiceCommandType::Continue:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::GoToSleep:
    case VoiceCommandType::NoThankYou:
    case VoiceCommandType::YesPlease:
    case VoiceCommandType::Count:
    {
      return false;
    }
  }
}

bool ActivityVoiceCommand::CheckAndSetupRefuseBehavior(Robot& robot,
                                                       BehaviorID whichRefuse,
                                                       IBehaviorPtr& outputBehavior) const
{
  IBehaviorPtr refuseBehavior = robot.GetBehaviorManager().FindBehaviorByID(whichRefuse);
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
    BeginRespondingToCommand(currentCommand);
  }
  else if (currentCommand == VoiceCommandType::NoThankYou)
  {
    voiceCommandComponent->BroadcastVoiceEvent(UserResponseToPrompt(false));
    voiceCommandComponent->ClearHeardCommand();
    BeginRespondingToCommand(currentCommand);
  }else if ( currentCommand == VoiceCommandType::Continue){
    // The continue command is handled by the VC Component and doesn't require
    // data to be sent up - so go ahead and clear the command
    voiceCommandComponent->ClearHeardCommand();
    BeginRespondingToCommand(currentCommand);
  }

  return Result::RESULT_OK;
}

bool ActivityVoiceCommand::CheckForLaserTrackingChange(Robot& robot, const IBehaviorPtr curBehavior)
{
  // If we are waiting for a laser or motion but have entered a reactionary behavior then stop waiting for
  // the laser or motion and give up
  if(_lookDownState != LookDownState::NONE &&
     robot.GetBehaviorManager().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger)
  {
    _lookDownState = LookDownState::NONE;
    return false;
  }
  
  if(curBehavior != nullptr)
  {
    // If we are waiting for a laser but are no longer in the wait for laser behavior
    // then it must have not found a laser so switch to looking for motion with the PounceOnMotion
    // behavior
    if(_lookDownState == LookDownState::WAITING_FOR_LASER &&
       curBehavior->GetID() != BehaviorID::VC_TrackLaser)
    {
      BehaviorPreReqNone preReq;
      if(_pounceBehavior->IsRunnable(preReq))
      {
        _voiceCommandBehavior = _pounceBehavior;
      }
      
      _lookDownState = LookDownState::WAITING_FOR_MOTION;
      
      return true;
    }
    // Otherwise if we are waiting for motion but are no longer in the wait for motion behavior
    // then it must have not found motion so switch to playing a "nothing was found" animation
    else if(_lookDownState == LookDownState::WAITING_FOR_MOTION &&
            curBehavior->GetID() != BehaviorID::VC_PounceOnMotion)
    {
      _playAnimBehavior->SetAnimationTrigger(AnimationTrigger::VCLookDownNoLaser, 1);
      
      BehaviorPreReqNone preReq;
      if(_playAnimBehavior->IsRunnable(preReq))
      {
        _voiceCommandBehavior = _playAnimBehavior;
      }
      
      _lookDownState = LookDownState::NONE;
      
      return true;
    }
  }
  
  return false;
}

template<>
void ActivityVoiceCommand::HandleMessage(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  // If we are waiting for a laser and the TrackLaser behavior successfully tracked a laser then
  // we are no longer waiting for a laser
  if(_lookDownState == LookDownState::WAITING_FOR_LASER &&
     msg.behaviorObjective == BehaviorObjective::LaserTracked)
  {
    _lookDownState = LookDownState::NONE;
  }
  else if(_lookDownState == LookDownState::WAITING_FOR_MOTION &&
          msg.behaviorObjective == BehaviorObjective::PouncedAndCaught)
  {
    _lookDownState = LookDownState::NONE;
  }
}

} // namespace Cozmo
} // namespace Anki
