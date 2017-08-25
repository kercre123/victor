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


#include "engine/behaviorSystem/activities/activities/activityVoiceCommand.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/doATrickSelector.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"
#include "engine/behaviorSystem/behaviors/freeplay/behaviorDriveToFace.h"

#include "engine/components/inventoryComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/voiceCommands/voiceCommandComponent.h"

#include "anki/common/basestation/utils/timer.h"
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

static const int kMaxChooseBehaviorLoop = 1000;

} // namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityVoiceCommand::VCResponseData::SetNewResponseData(ChooseNextBehaviorQueue&& responseQueue,
                                                              VoiceCommand::VoiceCommandType commandType)
{
  // Ensure data's not being overwritten improperly
  ANKI_VERIFY(_respondToVCQueue.empty() &&
              (_currentResponseType == VoiceCommand::VoiceCommandType::Invalid) &&
              _currentResponseBehavior == nullptr,
              "VCResponseData.SetNewResponseData.OverwritingData",
              "Attempting to overwrite response data with remaining que length %zu and type %s",
              _respondToVCQueue.size(),
              VoiceCommand::VoiceCommandTypeToString(_currentResponseType));
  
  _currentResponseBehavior.reset();
  _respondToVCQueue = std::move(responseQueue);
  _currentResponseType = commandType;
}
  
void ActivityVoiceCommand::VCResponseData::ClearResponseData(){
  if(_currentResponseType != VoiceCommand::VoiceCommandType::Invalid){
    using namespace ::Anki::Cozmo::VoiceCommand;
    auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
    voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandEnd(_currentResponseType));
  }
  
  _respondToVCQueue = {};
  _currentResponseBehavior.reset();
  _currentResponseType = VoiceCommand::VoiceCommandType::Invalid;

}
  

void ActivityVoiceCommand::VCResponseData::ClearResponseQueue()
{
  _respondToVCQueue = {};
}


bool ActivityVoiceCommand::VCResponseData::WillStartRespondingToCommand()
{
  return (_currentResponseBehavior == nullptr) && !_respondToVCQueue.empty();
}

IBehaviorPtr ActivityVoiceCommand::VCResponseData::ChooseNextBehavior(const IBehaviorPtr currentBehavior)
{
  using namespace ::Anki::Cozmo::VoiceCommand;

  // Check to see if we're about to start responding to a VC
  if(WillStartRespondingToCommand()){
    auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
    ActivityVoiceCommand::BeginRespondingToCommand(_context, GetCommandType());
    voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommandStart(GetCommandType()));
  }
  
  // Logic overview: During Update responses to VCs are determined and set up as
  // a queue of functions to iterate over.  Each function returns a behavior ptr
  // which should be run to completion.  Within the function any messages or
  // parameters can be set - returning nullptr is allowed as the last function in
  // the queue if events need to be sent within the lambda but no behavior needs to run
  BOUNDED_WHILE(kMaxChooseBehaviorLoop,
                (_currentResponseBehavior != nullptr) ||
                (_currentResponseBehavior == nullptr) && !_respondToVCQueue.empty())
  {
    if(_currentResponseBehavior != nullptr)
    {
      // If we already have a behavior that VC started we should keep using it while it's running
      if(_currentResponseBehavior->IsRunning())
      {
        return _currentResponseBehavior;
      }
      else
      {
        // clear out the currently running behavior
        _currentResponseBehavior = nullptr;
      }
    }
    else if(!_respondToVCQueue.empty())
    {
      {
        // Get the behavior function, get the next behavior to run, and then pop
        // the function off the queue
        auto& GetBehaviorFunc = _respondToVCQueue.front();
        _currentResponseBehavior = GetBehaviorFunc(currentBehavior);
        _respondToVCQueue.pop();
      }
      
      if(_currentResponseBehavior == nullptr)
      {
        ANKI_VERIFY(_respondToVCQueue.empty(),
                    "ActivityVoiceCommand.ChooseNextBehaviorInternal.ResponseQueueNotEmpty",
                    "Response function returned nullptr, but there are still %zu functions in the queue",
                    _respondToVCQueue.size());
      }
      else
      {
        // return the behavior so that it will start running this tick
        return _currentResponseBehavior;
      }
    } // end else if(!_respondToVCQueue.empty())
  } // end BOUNDED_WHILE
  
  // The response queue has finished, so clear all data about the last
  // command responded to
  ClearResponseData();
  
  IBehaviorPtr emptyPtr;
  return emptyPtr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
, _vcResponseData(std::make_unique<VCResponseData>(_context))
, _lookDownObjectiveAchieved(false)
{
  
  const auto& BM = robot.GetBehaviorManager();
  // TODO: Will need to change how this works should we add more dance behaviors
  _danceBehavior = BM.FindBehaviorByID(BehaviorID::Dance_Mambo);
  DEV_ASSERT(_danceBehavior != nullptr &&
             _danceBehavior->GetClass() == BehaviorClass::Dance,
             "VoiceCommandBehaviorChooser.Dance.ImproperClassRetrievedForID");
  
  {
    IBehaviorPtr comeHereBehavior = BM.FindBehaviorByID(BehaviorID::VC_ComeHere);
    if(ANKI_DEV_CHEATS){
      _driveToFaceBehavior = std::dynamic_pointer_cast<BehaviorDriveToFace>(comeHereBehavior);

    }else{
      _driveToFaceBehavior = std::static_pointer_cast<BehaviorDriveToFace>(comeHereBehavior);

    }
    DEV_ASSERT(_driveToFaceBehavior != nullptr &&
               _driveToFaceBehavior->GetClass() == BehaviorClass::DriveToFace,
               "VoiceCommandBehaviorChooser.ComeHereBehavior.ImproperClassRetrievedForName");
  }
  
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
  
  _alrightyBehavior = BM.FindBehaviorByID(BehaviorID::VC_AlrightyResponse);
  DEV_ASSERT(_alrightyBehavior != nullptr &&
             _alrightyBehavior->GetClass() == BehaviorClass::PlayAnimWithFace,
             "VoiceCommandBehaviorChooser.AlrightyBehavior.ImproperClassRetrievedForID");
  
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityVoiceCommand::ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr emptyPtr;
  
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.ChooseNextBehavior", "VoiceCommandComponent invalid"))
  {
    return emptyPtr;
  }
  
  
  if(ANKI_VERIFY(_vcResponseData != nullptr,
                 "ActivityVoiceCommand.ChooseNextBehavior.NoResponseData",
                 "Respones data ptr is null")){
    return _vcResponseData->ChooseNextBehavior(currentRunningBehavior);
  }
  
  return emptyPtr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityVoiceCommand::Update(Robot& robot)
{
  auto* voiceCommandComponent = _context->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent != nullptr, "ActivityVoiceCommand.Update", "VoiceCommandComponent invalid"))
  {
    return Result::RESULT_FAIL;
  }
  
  // Clear out all state tracking if a reaction plays or cozmo is off his treads
  if((robot.GetBehaviorManager().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger) ||
     (robot.GetOffTreadsState() != OffTreadsState::OnTreads)){
    _lookDownObjectiveAchieved = false;
    _vcResponseData->ClearResponseData();
  }
  
  // If one of the look down objectives was achieved clear out the rest of the response
  // queue, but allow the currently running behavior to end on its own
  if(_lookDownObjectiveAchieved){
    _lookDownObjectiveAchieved = false;
    _vcResponseData->ClearResponseQueue();
  }
  
  const auto& currentCommand = voiceCommandComponent->GetPendingCommand();
  
  if(!ShouldActivityRespondToCommand(currentCommand))
  {
    return Result::RESULT_OK;
  }
  
  // The activity will respond to the command, so clear the pending command out
  // of the voice command component
  voiceCommandComponent->ClearHeardCommand();
  
  // If the command requires an unlock and it's not unlocked, we don't
  // want to respond at all
  if(!HasAppropriateUnlocksForCommand(robot, currentCommand))
  {
    return Result::RESULT_OK;
  }
  
  ///////
  // Commands that unity responds to - these we mark as responding to command
  // here - all other responses which setup response queues are handled in
  // ChooseNextBehavior
  ///////
  {
    if (currentCommand == VoiceCommandType::YesPlease)
    {
      voiceCommandComponent->BroadcastVoiceEvent(UserResponseToPrompt(true));
      BeginRespondingToCommand(_context, currentCommand);
      // nothing else to do - unity will handle the rest
      return Result::RESULT_OK;
    }
    else if (currentCommand == VoiceCommandType::NoThankYou)
    {
      voiceCommandComponent->BroadcastVoiceEvent(UserResponseToPrompt(false));
      BeginRespondingToCommand(_context, currentCommand);
      // nothing else to do - unity will handle the rest
      return Result::RESULT_OK;
    }
    else if ( currentCommand == VoiceCommandType::Continue)
    {
      // The continue command is handled by the VC Component and doesn't require
      // any special messaging
      BeginRespondingToCommand(_context, currentCommand);
      // nothing else to do - unity will handle the rest
      return Result::RESULT_OK;
    }
  }
  
  ///////
  // Commands that Engine responds to
  ///////
  {
    ChooseNextBehaviorQueue responseQueue;
    // Clear out any response data if a previous command is still responding
    _vcResponseData->ClearResponseData();

    if(ShouldCheckNeeds(currentCommand))
    {
      // Check if we should refuse to execute the command due to needs state
      const bool shouldRefuse = CheckRefusalDueToNeeds(robot, responseQueue);
      if(shouldRefuse)
      {
        _vcResponseData->SetNewResponseData(std::move(responseQueue), currentCommand);
        // nothing else to do - refusal will run in ChooseNextBehavior
        return Result::RESULT_OK;
      }
    }
    
    // Check if we have enough sparks to execute the command
    if(!HasEnoughSparksForCommand(robot, currentCommand))
    {
      CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, responseQueue);
      _vcResponseData->SetNewResponseData(std::move(responseQueue), currentCommand);
      // nothing else to do - refusal will run in ChooseNextBehavior
      return Result::RESULT_OK;
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
            
            IBehaviorPtr ptrCopy = iter->second;
            responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
            responseQueue.push([this, &robot, ptrCopy, currentCommand](const IBehaviorPtr currentBehavior){
              RemoveSparksForCommand(robot, currentCommand);
              return ptrCopy;
            });
          }
        }
        break;
      }
      case VoiceCommandType::DoADance:
      {
        responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
        responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _danceBehavior;});
        break;
      }
      case VoiceCommandType::DoATrick:
      {
        responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
        responseQueue.push([this, &robot, currentCommand](const IBehaviorPtr currentBehavior){
          RemoveSparksForCommand(robot, currentCommand);
          robot.GetAIComponent().GetDoATrickSelector().RequestATrick(robot);
          return nullptr;
        });

        break;
      }
      case VoiceCommandType::ComeHere:
      {

        SmartFaceID desiredFace;
        
        Pose3d facePose;
        const TimeStamp_t timeLastFaceObserved = robot.GetFaceWorld().GetLastObservedFace(facePose, true);
        const bool lastFaceInCurrentOrigin = &facePose.FindOrigin() == robot.GetWorldOrigin();
        if(lastFaceInCurrentOrigin){
          const auto facesObserved = robot.GetFaceWorld().GetFaceIDsObservedSince(timeLastFaceObserved);
          if(facesObserved.size() > 0){
            desiredFace = robot.GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
          }
        }
        
        // If for any reason Cozmo can't drive to the user's face,
        // we should search for a face instead
        bool shouldSearchForFaces = false;
        
        if(desiredFace.IsValid()){
          _driveToFaceBehavior->SetTargetFace(desiredFace);
          if(_driveToFaceBehavior->IsRunnable(robot)){
            // Only play the "alrighty" animation if we're actually going to drive towards the face
            const auto& face = robot.GetFaceWorld().GetFace(desiredFace);
            if(face != nullptr){
              if(!_driveToFaceBehavior->IsCozmoAlreadyCloseEnoughToFace(robot, face->GetID())){
                 responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
              }
              responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _driveToFaceBehavior;});
            }
          }else{
            shouldSearchForFaces = true;
          }
        }else{
          shouldSearchForFaces = true;
        }
        
        if(shouldSearchForFaces){
          if(ANKI_VERIFY(_searchForFaceBehavior->IsRunnable(robot),
                         "ActivityVoiceCommand.ChooseNextBehaviorInternal.SearchForFaceNotRunnable",
                         "No way to respond to the voice command")){
            responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _searchForFaceBehavior;});
            // If we find a face while searching, drive towards it
            responseQueue.push([this, &robot](const IBehaviorPtr currentBehavior){
              Pose3d facePose;
              robot.GetFaceWorld().GetLastObservedFace(facePose, true);
              const bool lastFaceInCurrentOrigin = &facePose.FindOrigin() == robot.GetWorldOrigin();
              if(lastFaceInCurrentOrigin){
                return std::static_pointer_cast<IBehavior>(_driveToFaceBehavior);
              }else{
                IBehaviorPtr emptyPtr;
                return emptyPtr;
              }
            });
          }
        }
        
        break;
      }
      case VoiceCommandType::FistBump:
      {
        //Ensure fist bump is runnable
        if(_fistBumpBehavior->IsRunnable(robot))
        {
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
          responseQueue.push([this, &robot, currentCommand](const IBehaviorPtr currentBehavior){
            RemoveSparksForCommand(robot, currentCommand);
            const bool isSoftSpark = false;
            robot.GetBehaviorManager().SetRequestedSpark(UnlockId::FistBump, isSoftSpark);
            return nullptr;
          });
        }
        else
        {
          CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, responseQueue);
        }
        break;
      }
      case VoiceCommandType::PeekABoo:
      {
        //Ensure PeekABoo is runnable
        if(_peekABooBehavior->IsRunnable(robot))
        {
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
          responseQueue.push([this, &robot, currentCommand](const IBehaviorPtr currentBehavior){
            RemoveSparksForCommand(robot, currentCommand);
            const bool isSoftSpark = false;
            robot.GetBehaviorManager().SetRequestedSpark(UnlockId::PeekABoo, isSoftSpark);
            return nullptr;
          });
        }
        else
        {
          CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, responseQueue);
        }
        break;
      }
      case VoiceCommandType::GoToSleep:
      {
        if(_goToSleepBehavior->IsRunnable(robot))
        {
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _goToSleepBehavior;});
        }
        else
        {
          PRINT_NAMED_ERROR("ActivityVoiceCommand.ChooseNextBehaviorInternal.GoToSleepBehaviorNotRunnable", "");
        }
        break;
      }
      case VoiceCommandType::HowAreYouDoing:
      {
        HandleHowAreYouDoingCommand(robot, responseQueue);
        break;
      }
      case VoiceCommandType::LookDown:
      {
        if(_laserBehavior->IsRunnable(robot))
        {
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _alrightyBehavior;});
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _laserBehavior;   });
          responseQueue.push([this](const IBehaviorPtr currentBehavior){ return _pounceBehavior;  });
          responseQueue.push([this](const IBehaviorPtr currentBehavior){
            _playAnimBehavior->SetAnimationTrigger(AnimationTrigger::VC_LookDownNoLaser, 1);
            return _playAnimBehavior;
          });
          
        }
        else
        {
          CheckAndSetupRefuseBehavior(robot, BehaviorID::VC_Refuse_Sparks, responseQueue);
        }
        break;
      }
        
      // These commands should never have made it to this part of the function,
      // it should have exited above once they were handled
      case VoiceCommandType::YesPlease:
      case VoiceCommandType::NoThankYou:
      case VoiceCommandType::Continue:
      case VoiceCommandType::HeyCozmo:
      case VoiceCommandType::Invalid:
      {
        ANKI_VERIFY(false,
                    "ActivityVoiceCommand.ChooseNextBehaviorInternal.VCNotHandled",
                    "VC %s reached response queue switch within activity",
                    VoiceCommandTypeToString(currentCommand));
        break;
      }
    }
    
    if(!responseQueue.empty()){
      _vcResponseData->SetNewResponseData(std::move(responseQueue), currentCommand);
    }
    
  } // End engine responses to commands
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityVoiceCommand::HasAppropriateUnlocksForCommand(Robot& robot, VoiceCommand::VoiceCommandType command) const
{
  UnlockId requiredID = UnlockId::Count;
  
  // If this continues to expand explore the possibility of using a VCType -> behavior
  // map for extracting the required unlock ID types
  switch(command){
    case VoiceCommandType::FistBump:
    {
      requiredID = _fistBumpBehavior->GetRequiredUnlockID();
      break;
    }
    case VoiceCommandType::PeekABoo:
    {
      requiredID = _peekABooBehavior->GetRequiredUnlockID();
      break;
    }
    default:
    {
      break;
    }
  }
  
  if(requiredID != UnlockId::Count){
    return robot.GetProgressionUnlockComponent().IsUnlocked(requiredID);
  }
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityVoiceCommand::ShouldActivityRespondToCommand(VoiceCommand::VoiceCommandType command) const
{
  switch(command)
  {
    case VoiceCommandType::Continue:
    case VoiceCommandType::ComeHere:
    case VoiceCommandType::DoADance:
    case VoiceCommandType::DoATrick:
    case VoiceCommandType::FistBump:
    case VoiceCommandType::GoToSleep:
    case VoiceCommandType::HowAreYouDoing:
    case VoiceCommandType::LetsPlay:
    case VoiceCommandType::LookDown:
    case VoiceCommandType::PeekABoo:
    case VoiceCommandType::NoThankYou:
    case VoiceCommandType::YesPlease:
    {
      return true;
    }
      // These commands will never be handled by this chooser:
    case VoiceCommandType::HeyCozmo:
    case VoiceCommandType::Invalid:
    {
      // We're intentionally not handling these types in ActivityVoiceCommand
      return false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
    case VoiceCommandType::Invalid:
    {
      return false;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityVoiceCommand::CheckRefusalDueToNeeds(Robot& robot, ChooseNextBehaviorQueue& responseQueue) const
{
  Anki::Cozmo::NeedsState& curNeedsState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(curNeedsState.IsNeedAtBracket(NeedId::Repair, NeedBracketId::Critical))
  {
    BehaviorID whichRefuse = BehaviorID::VC_Refuse_Repair;
    return CheckAndSetupRefuseBehavior(robot, whichRefuse, responseQueue);
  }
  else if(curNeedsState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical))
  {
    BehaviorID whichRefuse = BehaviorID::VC_Refuse_Energy;
    return CheckAndSetupRefuseBehavior(robot, whichRefuse, responseQueue);
  }
  
  return false;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityVoiceCommand::CheckAndSetupRefuseBehavior(Robot& robot,
                                                       BehaviorID whichRefuse,
                                                       ChooseNextBehaviorQueue& responseQueue) const
{
  IBehaviorPtr refuseBehavior = robot.GetBehaviorManager().FindBehaviorByID(whichRefuse);
  DEV_ASSERT(refuseBehavior != nullptr &&
             refuseBehavior->GetClass() == BehaviorClass::PlayAnim,
             "VoiceCommandBehaviorChooser.Refuse.ImproperClassRetrievedForID");
  
  if(refuseBehavior->IsRunnable(robot))
  {
    responseQueue.push([refuseBehavior](const IBehaviorPtr currentBehavior){ return refuseBehavior;});
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityVoiceCommand::BeginRespondingToCommand(const CozmoContext* context, VoiceCommand::VoiceCommandType command)
{
  auto* voiceCommandComponent = context->GetVoiceCommandComponent();
  
  Anki::Util::sEvent("voice_command.responding_to_command", {}, EnumToString(command));
  voiceCommandComponent->BroadcastVoiceEvent(RespondingToCommand(command));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityVoiceCommand::HandleHowAreYouDoingCommand(Robot& robot, ChooseNextBehaviorQueue& responseQueue)
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
  
  std::shared_ptr<BehaviorPlayAnimSequenceWithFace> howAreYouDoingBehavior;
  std::vector<AnimationTrigger> howAreYouDoingAnims;
  
  // If we have a matching entry for the need and bracket then setup the behavior
  // to play the specified animations
  const auto& behaviorAndAnims = kNeedStateToBehaviorMap.find({need, bracket});
  if(behaviorAndAnims != kNeedStateToBehaviorMap.end())
  {
    const BehaviorID& behaviorID = behaviorAndAnims->second.first;
    const std::vector<AnimationTrigger>& anims = behaviorAndAnims->second.second;
    
    robot.GetBehaviorManager().FindBehaviorByIDAndDowncast(behaviorID,
                                                           BehaviorClass::PlayAnimWithFace,
                                                           howAreYouDoingBehavior);
    howAreYouDoingAnims = anims;
  }
  // Otherwise, based on the map, all needs are at least normal so respond with the "all good" behavior
  // and animation
  else
  {
    DEV_ASSERT((bracket == NeedBracketId::Full ||
                bracket == NeedBracketId::Normal),
               "ActivityVoiceCommand.HandleHowAreYouDoingCommand.NeedsBracketExpectedFullOrNormal");
    
    robot.GetBehaviorManager().FindBehaviorByIDAndDowncast(BehaviorID::VC_HowAreYouDoing_AllGood,
                                                           BehaviorClass::PlayAnimWithFace,
                                                           howAreYouDoingBehavior);
    howAreYouDoingAnims = {AnimationTrigger::VC_HowAreYouDoing_AllGood};
  }
  
  howAreYouDoingBehavior->SetAnimationsToPlay(howAreYouDoingAnims);
  if(howAreYouDoingBehavior->IsRunnable(robot))
  {
    responseQueue.push([howAreYouDoingBehavior](const IBehaviorPtr currentBehavior){ return howAreYouDoingBehavior;});
  }
  else
  {
    PRINT_NAMED_ERROR("ActivityVoiceCommand.HandleHowAreYouDoingCommand.BehaviorNotRunnable", "");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivityVoiceCommand::HandleMessage(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  // If we are waiting for a laser and the TrackLaser behavior successfully tracked a laser then
  // we are no longer waiting for a laser
  if(_vcResponseData->GetCommandType() == VoiceCommandType::LookDown){
    if((msg.behaviorObjective == BehaviorObjective::LaserTracked) ||
       (msg.behaviorObjective == BehaviorObjective::PouncedAndCaught))
    {
      _lookDownObjectiveAchieved = true;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
