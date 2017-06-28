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

#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/doATrickSelector.h"
#include "anki/cozmo/basestation/aiComponent/requestGameComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAnimSequence.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"
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
{
  
  const auto& BM = robot.GetBehaviorManager();
  // TODO: Will need to change how this works should we add more dance behaviors
  _danceBehavior = BM.FindBehaviorByID(BehaviorID::Dance_Mambo);
  DEV_ASSERT(_danceBehavior != nullptr &&
             _danceBehavior->GetClass() == BehaviorClass::Dance,
             "VoiceCommandBehaviorChooser.Dance.ImproperClassRetrievedForID");
  
  _driveToFaceBehavior = BM.FindBehaviorByID(BehaviorID::VC_ComeHere);
  DEV_ASSERT(_driveToFaceBehavior != nullptr &&
             _driveToFaceBehavior->GetClass() == BehaviorClass::DriveToFace,
             "VoiceCommandBehaviorChooser.ComeHereBehavior.ImproperClassRetrievedForName");
  
  _searchForFaceBehavior = BM.FindBehaviorByID(BehaviorID::VC_SearchForFace);
  DEV_ASSERT(_searchForFaceBehavior != nullptr &&
             _searchForFaceBehavior->GetClass() == BehaviorClass::SearchForFace,
             "VoiceCommandBehaviorChooser.SearchForFaceBehavior.ImproperClassRetrievedForName");
  
  _fistBumpBehavior = BM.FindBehaviorByID(BehaviorID::FistBump);
  DEV_ASSERT(_fistBumpBehavior != nullptr &&
             _fistBumpBehavior->GetClass() == BehaviorClass::FistBump,
             "VoiceCommandBehaviorChooser.FistBump.ImproperClassRetrievedForID");

  _peekABooBehavior = BM.FindBehaviorByID(BehaviorID::FPPeekABoo);
  DEV_ASSERT(_peekABooBehavior != nullptr &&
             _peekABooBehavior->GetClass() == BehaviorClass::PeekABoo,
             "VoiceCommandBehaviorChooser.PeekABoo.ImproperClassRetrievedForID");
  
  _laserBehavior = BM.FindBehaviorByID(BehaviorID::VC_TrackLaser);
  DEV_ASSERT(_laserBehavior != nullptr &&
             _laserBehavior->GetClass() == BehaviorClass::TrackLaser,
             "VoiceCommandBehaviorChooser.Laser.ImproperClassRetrievedForID");
  
  _pounceBehavior = BM.FindBehaviorByID(BehaviorID::VC_PounceOnMotion);
  DEV_ASSERT(_pounceBehavior != nullptr &&
             _pounceBehavior->GetClass() == BehaviorClass::PounceOnMotion,
             "VoiceCommandBehaviorChooser.PounceOnMotion.ImproperClassRetrievedForID");
  
  //Create an arbitrary animation behavior
  IBehaviorPtr playAnimPtr = BM.FindBehaviorByID(BehaviorID::PlayArbitraryAnim);
  _playAnimBehavior = std::static_pointer_cast<BehaviorPlayArbitraryAnim>(playAnimPtr);
  
  DEV_ASSERT(_playAnimBehavior != nullptr &&
             _playAnimBehavior->GetClass() == BehaviorClass::PlayArbitraryAnim,
             "VoiceCommandBehaviorChooser.BehaviorPlayAnimPointerNotSet");
  
  _goToSleepBehavior = BM.FindBehaviorByID(BehaviorID::VC_GoToSleep);
  DEV_ASSERT(_goToSleepBehavior != nullptr &&
             _goToSleepBehavior->GetClass() == BehaviorClass::ReactToOnCharger,
             "VoiceCommandBehaviorChooser.Laser.ImproperClassRetrievedForID");

  DEV_ASSERT(nullptr != _context, "ActivityVoiceCommand.Constructor.NullContext");
  
  // setup the let's play map
  IBehaviorPtr VC_Keepaway = BM.FindBehaviorByID(BehaviorID::VC_RequestKeepAway);
  IBehaviorPtr VC_QT = BM.FindBehaviorByID(BehaviorID::VC_RequestSpeedTap);
  IBehaviorPtr VC_MM = BM.FindBehaviorByID(BehaviorID::VC_RequestMemoryMatch);

  _letsPlayMap.insert(std::make_pair(UnlockId::KeepawayGame, VC_Keepaway));
  _letsPlayMap.insert(std::make_pair(UnlockId::QuickTapGame, VC_QT));
  _letsPlayMap.insert(std::make_pair(UnlockId::MemoryMatchGame, VC_MM));
  
  for(const auto& entry: _letsPlayMap){
    DEV_ASSERT_MSG(entry.second != nullptr &&
                   entry.second->GetClass() == BehaviorClass::RequestGameSimple,
                   "VoiceCommandBehaviorChooser.LetsPlayMap.InvalidBehavior",
                   "nullptr or wrong class for unlockID %s",
                   UnlockIdToString(entry.first));
  }
  
  // setup messaging
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
        const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
        BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
        return _voiceCommandBehavior;
      }
    }
    
    // Check if we have enough sparks to execute the command
    if(!HasEnoughSparksForCommand(robot, currentCommand))
    {
      CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, _voiceCommandBehavior);
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
  }
  
  switch (currentCommand)
  {
    case VoiceCommandType::LetsPlay:
    {
      auto& gameSelector = robot.GetAIComponent().GetRequestGameComponent();
      UnlockId requestGameID = gameSelector.IdentifyNextGameTypeToRequest(robot);
      
      if(requestGameID != UnlockId::Count){
        const auto& iter = _letsPlayMap.find(requestGameID);
        if(ANKI_VERIFY(iter != _letsPlayMap.end(),
                       "ActivityVoiceCommand.ChooseNextBehaviorInternal.LetsPlay",
                       "Unlock ID %s not found in map",
                       UnlockIdToString(requestGameID))){
          _voiceCommandBehavior = iter->second;
          
          const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
          BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
          RemoveSparksForCommand(robot, currentCommand);
          
        }
      }
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoADance:
    {
      _voiceCommandBehavior = _danceBehavior;
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::DoATrick:
    {
      robot.GetAIComponent().GetDoATrickSelector().RequestATrick(robot);
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
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
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
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
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
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
      BeginRespondingToCommand(currentCommand, shouldTrackLifetime);
      
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::GoToSleep:
    {
      BehaviorPreReqNone req;
      if(_goToSleepBehavior->IsRunnable(req))
      {
        _voiceCommandBehavior = _goToSleepBehavior;
      }
      else
      {
        PRINT_NAMED_ERROR("ActivityVoiceCommand.ChooseNextBehaviorInternal.GoToSleepBehaviorNotRunnable", "");
      }
      
      BeginRespondingToCommand(currentCommand);
      return _voiceCommandBehavior;
    }
    case VoiceCommandType::HowAreYouDoing:
    {
      HandleHowAreYouDoingCommand(robot, _voiceCommandBehavior);
      BeginRespondingToCommand(currentCommand);
      return _voiceCommandBehavior;
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
      
      const bool shouldTrackLifetime = (_voiceCommandBehavior != nullptr);
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
    case VoiceCommandType::HowAreYouDoing:
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
    case VoiceCommandType::HowAreYouDoing:
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

void ActivityVoiceCommand::HandleHowAreYouDoingCommand(Robot& robot, IBehaviorPtr& outputBehavior)
{
  // Maps a need and bracket to a behavior and animations
  static const std::map<std::pair<NeedId, NeedBracketId>,
                        std::pair<BehaviorID, std::vector<AnimationTrigger>>> kNeedStateToBehaviorMap = {
    {{NeedId::Energy, NeedBracketId::Warning},
     {BehaviorID::VC_HowAreYouDoing_Energy, {AnimationTrigger::NeedsMildLowEnergyRequest}}},
    {{NeedId::Energy, NeedBracketId::Critical},
     {BehaviorID::VC_HowAreYouDoing_Energy, {AnimationTrigger::NeedsSevereLowEnergyRequest}}},

    {{NeedId::Play, NeedBracketId::Warning},
      {BehaviorID::VC_HowAreYouDoing_Play, {AnimationTrigger::NeedsMildLowPlayRequest}}},
    {{NeedId::Play, NeedBracketId::Critical},
      {BehaviorID::VC_HowAreYouDoing_Play, {AnimationTrigger::NothingToDoBoredIntro,
                                            AnimationTrigger::NothingToDoBoredEvent,
                                            AnimationTrigger::NothingToDoBoredOutro}}},
    
    {{NeedId::Repair, NeedBracketId::Warning},
      {BehaviorID::VC_HowAreYouDoing_Repair, {AnimationTrigger::NeedsMildLowRepairRequest}}},
    {{NeedId::Repair, NeedBracketId::Critical},
      {BehaviorID::VC_HowAreYouDoing_Repair, {AnimationTrigger::NeedsSevereLowRepairRequest}}},
  };

  // Get the lowest need and what bracket it is in
  Anki::Cozmo::NeedsState curNeeds = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  NeedId need;
  NeedBracketId bracket;
  curNeeds.GetLowestNeedAndBracket(need, bracket);
  
  IBehaviorPtr howAreYouDoingBehavior;
  std::vector<AnimationTrigger> howAreYouDoingAnims;
  
  // If we have a matching entry for the need and bracket then setup the behavior
  // to play the specified animations
  const auto& behaviorAndAnims = kNeedStateToBehaviorMap.find({need, bracket});
  if(behaviorAndAnims != kNeedStateToBehaviorMap.end())
  {
    const BehaviorID& behaviorID = behaviorAndAnims->second.first;
    const std::vector<AnimationTrigger>& anims = behaviorAndAnims->second.second;
    
    IBehaviorPtr behavior = robot.GetBehaviorManager().FindBehaviorByID(behaviorID);
    DEV_ASSERT(behavior != nullptr,
               "ActivityVoiceCommand.HandleHowAreYouDoingCommand.NullBehavior");
    
    if(ANKI_DEV_CHEATS)
    {
      std::shared_ptr<BehaviorPlayAnimSequenceWithFace> playAnimBehavior =
        std::static_pointer_cast<BehaviorPlayAnimSequenceWithFace>(behavior);
      DEV_ASSERT(playAnimBehavior != nullptr &&
                 playAnimBehavior->GetClass() == BehaviorClass::PlayAnimWithFace,
                 "ActivityVoiceCommand.HandleHowAreYouDoingCommand.ImproperClassRetrievedForID");
    }
    
    howAreYouDoingBehavior = behavior;
    howAreYouDoingAnims = anims;
  }
  // Otherwise, based on the map, all needs are at least normal so respond with the "all good" behavior
  // and animation
  else
  {
    DEV_ASSERT((bracket == NeedBracketId::Full ||
                bracket == NeedBracketId::Normal),
               "ActivityVoiceCommand.HandleHowAreYouDoingCommand.NeedsBracketExpectedFullOrNormal");
    
    IBehaviorPtr behavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::VC_HowAreYouDoing_AllGood);
    DEV_ASSERT(behavior != nullptr,
               "ActivityVoiceCommand.HandleHowAreYouDoingCommand.AllGood.NullBehavior");
    
    howAreYouDoingBehavior = behavior;
    howAreYouDoingAnims = {AnimationTrigger::VC_HowAreYouDoing_AllGood};
  }
  
  BehaviorPreReqAnimSequence preReq(howAreYouDoingAnims);
  if(howAreYouDoingBehavior->IsRunnable(preReq))
  {
    outputBehavior = howAreYouDoingBehavior;
  }
  else
  {
    PRINT_NAMED_ERROR("ActivityVoiceCommand.HandleHowAreYouDoingCommand.BehaviorNotRunnable", "");
  }
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
      _playAnimBehavior->SetAnimationTrigger(AnimationTrigger::VC_LookDownNoLaser, 1);
      
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
