/**
 * File: behaviorManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 * Overhaul: 2016-04-18 Brad Neuman
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014-2016
 **/

#include "anki/cozmo/basestation/behaviorManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/messageHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodDebug.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/selectionBehaviorChooser.h"
#include "clad/types/behaviorChooserType.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#define DEBUG_BEHAVIOR_MGR 0

namespace Anki {
namespace Cozmo {
  
static const char* kChooserConfigKey = "chooserConfig";
  
BehaviorManager::BehaviorManager(Robot& robot)
  : _isInitialized(false)
  , _robot(robot)
  , _behaviorFactory(new BehaviorFactory())
  , _whiteboard( new AIWhiteboard(robot) )
{
}

BehaviorManager::~BehaviorManager()
{
  // everything in _reactionaryBehaviors is a factory behavior, so the factory will delete it
  _reactionaryBehaviors.clear();

  Util::SafeDelete(_selectionChooser);
  Util::SafeDelete(_freeplayBehaviorChooser);
  Util::SafeDelete(_behaviorFactory);
}
  
Result BehaviorManager::Init(const Json::Value &config)
{
  BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
    
  {
    const Json::Value& chooserConfigJson = config[kChooserConfigKey];

    Util::SafeDelete(_selectionChooser);
    Util::SafeDelete(_freeplayBehaviorChooser);

    _selectionChooser = new SelectionBehaviorChooser(_robot, chooserConfigJson);
    _freeplayBehaviorChooser = new SimpleBehaviorChooser(_robot, chooserConfigJson);
                                   
    SetBehaviorChooser( _selectionChooser );

    // TODO:(bn) load these from json? A special reactionary behaviors list?
    BehaviorFactory& behaviorFactory = GetBehaviorFactory();
    AddReactionaryBehavior(
      behaviorFactory.CreateBehavior(BehaviorType::ReactToStop,  _robot, config)->AsReactionaryBehavior() );
    AddReactionaryBehavior(
      behaviorFactory.CreateBehavior(BehaviorType::ReactToPickup, _robot, config)->AsReactionaryBehavior() );
    AddReactionaryBehavior(
      behaviorFactory.CreateBehavior(BehaviorType::ReactToCliff,  _robot, config)->AsReactionaryBehavior() );
    // AddReactionaryBehavior(
    //   behaviorFactory.CreateBehavior(BehaviorType::ReactToPoke,   _robot, config)->AsReactionaryBehavior() );
  }
    
  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
    
  if (_robot.HasExternalInterface())
  {
    IExternalInterface* externalInterface = _robot.GetExternalInterface();
    _eventHandlers.push_back(externalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ActivateBehaviorChooser,
                               [this, config] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                               {
                                 const BehaviorChooserType chooserType =
                                   event.GetData().Get_ActivateBehaviorChooser().behaviorChooserType;
                                 switch (chooserType)
                                 {
                                   case BehaviorChooserType::Freeplay:
                                   {
                                     SetBehaviorChooser( _freeplayBehaviorChooser );
                                     break;
                                   }
                                   case BehaviorChooserType::Selection:
                                   {
                                     SetBehaviorChooser( _selectionChooser );
                                     break;
                                   }
                                   default:
                                   {
                                     PRINT_NAMED_WARNING("BehaviorManager.ActivateBehaviorChooser.InvalidChooser",
                                                         "don't know how to create a chooser of type '%s'",
                                                         BehaviorChooserTypeToString(chooserType));
                                     break;
                                   }
                                 }
                               }));
  }
  _isInitialized = true;
    
  return RESULT_OK;
}

// The AddReactionaryBehavior wrapper is responsible for setting up the callbacks so that important events will be
// reacted to correctly - events will be given to the Chooser which may return a behavior to force switch to
void BehaviorManager::AddReactionaryBehavior(IReactionaryBehavior* behavior)
{
  _reactionaryBehaviors.push_back(behavior);
    
  // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
  if (!_robot.HasExternalInterface()) {
    return;
  }
  
  // Subscribe our own callback to these events
  IExternalInterface* interface = _robot.GetExternalInterface();
  for (auto tag : behavior->GetEngineToGameTags())
  {
    _eventHandlers.push_back(
      interface->Subscribe(
        tag,
        std::bind(&BehaviorManager::ConsiderReactionaryBehaviorForEvent<ExternalInterface::MessageEngineToGame>,
                  this,
                  std::placeholders::_1)));
  }
  
  // Subscribe our own callback to these events
  for (auto tag : behavior->GetGameToEngineTags())
  {
    _eventHandlers.push_back(
      interface->Subscribe(
        tag,
        std::bind(&BehaviorManager::ConsiderReactionaryBehaviorForEvent<ExternalInterface::MessageGameToEngine>,
                  this,
                  std::placeholders::_1)));
  }
}

template<typename EventType>
void BehaviorManager::ConsiderReactionaryBehaviorForEvent(const AnkiEvent<EventType>& event)
{
  for (auto& behavior : _reactionaryBehaviors)
  {
    if (0 != GetReactionaryBehaviorTags<typename EventType::Tag>(behavior).count(event.GetData().GetTag()))
    {
      if ( behavior->ShouldRunForEvent(event.GetData()) &&
           behavior->IsRunnable(_robot) ) {
        PRINT_NAMED_INFO("ReactionaryBehavior.Found",
                         "found reactionary behavior '%s' in response to event of type '%s'",
                         behavior->GetName().c_str(),
                         MessageTagToString(event.GetData().GetTag()));
        SwitchToReactionaryBehavior( behavior );
        return;
      }
    }
  }
}

void BehaviorManager::SendDasTransitionMessage(IBehavior* oldBehavior, IBehavior* newBehavior)
{
  Anki::Util::sEvent("robot.behavior_transition",
                     {{DDATA,
                           nullptr != oldBehavior
                           ? oldBehavior->GetName().c_str()
                           : "NULL"}},
                     nullptr != newBehavior
                     ? newBehavior->GetName().c_str()
                     : "NULL");
}

bool BehaviorManager::SwitchToBehavior(IBehavior* nextBehavior)
{
  if( _currentBehavior == nextBehavior ) {
    // nothing to do if the behavior hasn't changed
    return false;
  }

  StopCurrentBehavior();

  if( nullptr != nextBehavior ) {
    const Result initRet = nextBehavior->Init();
    if ( initRet != RESULT_OK ) {
      PRINT_NAMED_ERROR("BehaviorManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        nextBehavior->GetName().c_str());
      // in this case, the current behavior is still running, not nextBehavior
      return false;
    }
    else {
      SendDasTransitionMessage(_currentBehavior, nextBehavior);
      _currentBehavior = nextBehavior;
      return true;
    }
  }
  else {
    // a null argument to this function means "switch to no behavior"
    _currentBehavior = nullptr;
    return true;
  }
}

void BehaviorManager::SwitchToNextBehavior()
{
  if( _behaviorToResume != nullptr ) {
    if( _currentBehavior == nullptr || _currentBehavior->ShouldResumeLastBehavior() ) {
      PRINT_NAMED_INFO("BehaviorManager.ResumeBehavior",
                       "Behavior '%s' will be resumed",
                       _behaviorToResume->GetName().c_str());
      StopCurrentBehavior();
      const Result resumeResult = _behaviorToResume->Resume();
      if( resumeResult == RESULT_OK ) {
        SendDasTransitionMessage(_currentBehavior, _behaviorToResume);
        _currentBehavior = _behaviorToResume;
        _behaviorToResume = nullptr;
        return;
      }
      else {
        PRINT_NAMED_INFO("BehaviorManager.ResumeFailed",
                         "Tried to resume behavior '%s', but failed. Selecting new behavior from chooser",
                         _behaviorToResume->GetName().c_str());
        // clear the resume behavior because we won't want to resume it after the next behavior
        _behaviorToResume = nullptr;
      }
    }
    else {
      // current behavior says we shouldn't resume, so just clear resume
      _behaviorToResume = nullptr;
    }
  }
  
  SwitchToBehavior( _behaviorChooser->ChooseNextBehavior(_robot) );
}

void BehaviorManager::SwitchToReactionaryBehavior(IBehavior* nextBehavior)
{
  // a null here means "no reaction", not "switch to the null behavior"
  if( nullptr == nextBehavior ) {
    return;
  }
  
  if( nullptr == _behaviorToResume ) {
    // only set the behavior to resume if we don't already have one. This way, if one reactionary behavior
    // interrupts another, we'll keep the original value.
    _behaviorToResume = _currentBehavior;
  }

  if( SwitchToBehavior(nextBehavior) ) {
    _runningReactionaryBehavior = true;
  }
}
  
Result BehaviorManager::Update()
{
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
    
  _behaviorChooser->Update();

  if( ! _runningReactionaryBehavior ) {
    SwitchToNextBehavior();
  }
    
  if(nullptr != _currentBehavior) {
    // We have a current behavior, update it.
    IBehavior::Status status = _currentBehavior->Update();
     
    switch(status)
    {
      case IBehavior::Status::Running:
        // Nothing to do! Just keep on truckin'....
        break;
          
      case IBehavior::Status::Complete:
        // behavior is complete, switch to null (will also handle stopping current). If it was reactionary,
        // switch now to give the last behavior a chance to resume (if appropriate)
        PRINT_NAMED_DEBUG("BehaviorManager.Update.BehaviorComplete",
                          "Behavior '%s' returned Status::Complete",
                          _currentBehavior->GetName().c_str());
        if( _runningReactionaryBehavior ) {
          _runningReactionaryBehavior = false;
          SwitchToNextBehavior();
        }
        else {
          SwitchToBehavior(nullptr);
        }
        break;
          
      case IBehavior::Status::Failure:
        PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                          "Behavior '%s' failed to Update().",
                          _currentBehavior->GetName().c_str());
        // same as the Complete case
        if( _runningReactionaryBehavior ) {
          _runningReactionaryBehavior = false;
          SwitchToNextBehavior();
        }
        else {
          SwitchToBehavior(nullptr);
        }
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorManager.Update.UnknownStatus",
                          "Behavior '%s' returned unknown status %d",
                          _currentBehavior->GetName().c_str(), status);
        lastResult = RESULT_FAIL;
        break;
    } // switch(status)
  }
    
  return lastResult;
} // Update()

void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
{
  if( _behaviorChooser == newChooser ) {
    return;
  }

  // The behavior pointers may no longer be valid, so clear them
  SwitchToBehavior(nullptr);
  _behaviorToResume = nullptr;
  _runningReactionaryBehavior = false;

  _behaviorChooser = newChooser;

  if( _behaviorChooser == _freeplayBehaviorChooser ) {
    _robot.GetProgressionUnlockComponent().IterateUnlockedFreeplayBehaviors(
      [this](BehaviorGroup group, bool enabled){
        _behaviorChooser->EnableBehaviorGroup(group, enabled);
      });
  }

  // force the new behavior chooser to select something now, instead of waiting for the next tick
  SwitchToNextBehavior();
}

void BehaviorManager::StopCurrentBehavior()
{
  if ( nullptr != _currentBehavior && _currentBehavior->IsRunning() ) {
    _currentBehavior->Stop();
  }
}

IBehavior* BehaviorManager::LoadBehaviorFromJson(const Json::Value& behaviorJson)
{
  IBehavior* newBehavior = _behaviorFactory->CreateBehavior(behaviorJson, _robot);
  return newBehavior;
}
  
void BehaviorManager::ClearAllBehaviorOverrides()
{
  const BehaviorFactory::NameToBehaviorMap& nameToBehaviorMap = _behaviorFactory->GetBehaviorMap();
  for(const auto& it : nameToBehaviorMap)
  {
    IBehavior* behavior = it.second;
    behavior->SetOverrideScore(-1.0f);
  }
}
  
bool BehaviorManager::OverrideBehaviorScore(const std::string& behaviorName, float newScore)
{
  IBehavior* behavior = _behaviorFactory->FindBehaviorByName(behaviorName);
  if (behavior)
  {
    behavior->SetOverrideScore(newScore);
    return true;
  }
  return false;
}
  
void BehaviorManager::HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message)
{
  switch (message.GetTag())
  {
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableAllBehaviors:
    {
      const auto& msg = message.Get_SetEnableAllBehaviors();
      if (_behaviorChooser)
      {
        PRINT_NAMED_DEBUG("BehaviorManager.HandleMessage.SetEnableAllBehaviors", "%s",
                          msg.enable ? "true" : "false");
        _behaviorChooser->EnableAllBehaviors(msg.enable);
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.SetEnableAllBehaviorGroups.NullChooser",
                            "Ignoring EnableAllBehaviorGroups(%d)", (int)msg.enable);
      }
      break;
    }
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableBehaviorGroup:
    {
      const auto& msg = message.Get_SetEnableBehaviorGroup();
      if (_behaviorChooser)
      {
        PRINT_NAMED_DEBUG("BehaviorManager.HandleMessage.SetEnableBehaviorGroup", "%s: %s",
                          BehaviorGroupToString(msg.behaviorGroup),
                          msg.enable ? "true" : "false");

        _behaviorChooser->EnableBehaviorGroup(msg.behaviorGroup, msg.enable);
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.SetEnableBehaviorGroup.NullChooser",
                            "Ignoring EnableBehaviorGroup('%s', %d)", BehaviorGroupToString(msg.behaviorGroup), (int)msg.enable);
      }
      break;
    }
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetEnableBehavior:
    {
      const auto& msg = message.Get_SetEnableBehavior();
      if (_behaviorChooser)
      {
        PRINT_NAMED_DEBUG("BehaviorManager.HandleMessage.SetEnableBehavior", "%s: %s",
                          msg.behaviorName.c_str(),
                          msg.enable ? "true" : "false");
        _behaviorChooser->EnableBehavior(msg.behaviorName, msg.enable);
      }
      else
      {
        PRINT_NAMED_WARNING("BehaviorManager.HandleEvent.DisableBehaviorGroup.NullChooser",
                            "Ignoring DisableBehaviorGroup('%s', %d)", msg.behaviorName.c_str(), (int)msg.enable);
      }
      break;
    }
    case ExternalInterface::BehaviorManagerMessageUnionTag::ClearAllBehaviorScoreOverrides:
    {
      ClearAllBehaviorOverrides();
      break;
    }
    case ExternalInterface::BehaviorManagerMessageUnionTag::OverrideBehaviorScore:
    {
      const auto& msg = message.Get_OverrideBehaviorScore();
      OverrideBehaviorScore(msg.behaviorName, msg.newScore);
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorManager.HandleEvent.UnhandledMessageUnionTag",
                        "Unexpected tag %u '%s'", (uint32_t)message.GetTag(),
                        BehaviorManagerMessageUnionTagToString(message.GetTag()));
      assert(0);
      break;
    }
  }
}
  
} // namespace Cozmo
} // namespace Anki
