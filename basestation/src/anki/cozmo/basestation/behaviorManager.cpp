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
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "anki/cozmo/basestation/messageHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodDebug.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/types/behaviorChooserType.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#define DEBUG_BEHAVIOR_MGR 0

namespace Anki {
namespace Cozmo {
  
static const char* kDemoChooserConfigKey = "demoBehaviorChooserConfig";
static const char* kSelectionChooserConfigKey = "selectionBehaviorChooserConfig";
static const char* kFreeplayChooserConfigKey = "freeplayBehaviorChooserConfig";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::BehaviorManager(Robot& robot)
  : _robot(robot)
  , _behaviorFactory(new BehaviorFactory())
  , _lastChooserSwitchTime(-1.0f)
  , _whiteboard( new AIWhiteboard(robot) )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::~BehaviorManager()
{
  // everything in _reactionaryBehaviors is a factory behavior, so the factory will delete it
  _reactionaryBehaviors.clear();
  
  // destroy choosers before factory
  Util::SafeDelete(_freeplayChooser);
  Util::SafeDelete(_selectionChooser);
  Util::SafeDelete(_demoChooser);
  Util::SafeDelete(_behaviorFactory);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::InitConfiguration(const Json::Value &config)
{
  BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
  
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  ASSERT_NAMED(!_isInitialized, "BehaviorManager.InitConfiguration.AlreadyInitialized");

  // create choosers
  if ( !config.isNull() )
  {
    // selection chooser - to force one specific behavior
    const Json::Value& selectionChooserConfigJson = config[kSelectionChooserConfigKey];
    _selectionChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, selectionChooserConfigJson);

    // demo chooser - for scripted demos/setups (investor, etc)
    const Json::Value& demoChooserConfigJson = config[kDemoChooserConfigKey];
    _demoChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, demoChooserConfigJson);
    
    // freeplay chooser - AI controls cozmo
    const Json::Value& freeplayChooserConfigJson = config[kFreeplayChooserConfigKey];
    _freeplayChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, freeplayChooserConfigJson);

    // start with selection that defaults to NoneBehavior
    SetBehaviorChooser( _selectionChooser );

    BehaviorFactory& behaviorFactory = GetBehaviorFactory();

    for( const auto& it : behaviorFactory.GetBehaviorMap() ) {
      const auto& behaviorName = it.first;
      const auto& behaviorPtr = it.second;
      
      if( behaviorPtr->IsBehaviorGroup( BehaviorGroup::Reactionary ) ) {
        PRINT_NAMED_DEBUG("BehaviorManager.EnableReactionaryBehavior",
                          "Adding behavior '%s' as reactionary",
                          behaviorName.c_str());
        IReactionaryBehavior*  reactionaryBehavior = behaviorPtr->AsReactionaryBehavior();
        if( nullptr == reactionaryBehavior ) {
          PRINT_NAMED_ERROR("BehaviorManager.ReactionaryBehaviorNotReactionary",
                            "behavior %s is in the factory (ptr %p), but isn't reactionry",
                            behaviorName.c_str(),
                            behaviorPtr);
          continue;
        }
        AddReactionaryBehavior(reactionaryBehavior);
      }
    }
  }
  
  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
  
  if (_robot.HasExternalInterface())
  {
    IExternalInterface* externalInterface = _robot.GetExternalInterface();
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::EnableReactionaryBehaviors,
                          [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                          {
                            _reactionsEnabled = event.GetData().Get_EnableReactionaryBehaviors().enabled;
                            PRINT_NAMED_INFO("EnableReactionaryBehaviors", (_reactionsEnabled ? "Enabled" : "Disabled"));
                            if(!_reactionsEnabled && _runningReactionaryBehavior)
                            {
                              PRINT_NAMED_INFO("EnableReactionaryBehaviors", "Disabling reactionary behaviors stopping currently running one");
                              SwitchToBehavior(nullptr);
                            }
                          }));
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
                                     SetBehaviorChooser( _freeplayChooser );
                                     _robot.GetLightsComponent().SetEnableComponent(true);
                                     break;
                                   }
                                   case BehaviorChooserType::Selection:
                                   {
                                     SetBehaviorChooser( _selectionChooser );
                                     // TODO:(bn) do this elsewhere?
                                     _robot.GetLightsComponent().SetEnableComponent(false);
                                     break;
                                   }
                                   case BehaviorChooserType::Demo:
                                   {
                                     SetBehaviorChooser(_demoChooser);
                                     _robot.GetLightsComponent().SetEnableComponent(true);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::CreateBehaviorFromConfiguration(const Json::Value& behaviorJson)
{
  // try to create behavior, name should be unique here
  IBehavior* newBehavior = _behaviorFactory->CreateBehavior(behaviorJson, _robot, BehaviorFactory::NameCollisionRule::Fail);
  const Result ret = (nullptr != newBehavior) ? RESULT_OK : RESULT_FAIL;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EventType>
void BehaviorManager::ConsiderReactionaryBehaviorForEvent(const AnkiEvent<EventType>& event)
{
  if( !_reactionsEnabled ) {
    return;
  }
  for (auto& behavior : _reactionaryBehaviors)
  {
    if (0 != GetReactionaryBehaviorTags<typename EventType::Tag>(behavior).count(event.GetData().GetTag()))
    {
      if ( behavior->ShouldRunForEvent(event.GetData(),_robot) &&
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SendDasTransitionMessage(IBehavior* oldBehavior, IBehavior* newBehavior)
{
  // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
  if (!_robot.HasExternalInterface()) {
    return;
  }
  
  const std::string& oldBehaviorName = nullptr != oldBehavior ? oldBehavior->GetName() : "NULL";
  const std::string& newBehaviorName = nullptr != newBehavior ? newBehavior->GetName() : "NULL";
  BehaviorType oldBehaviorType = nullptr != oldBehavior ? oldBehavior->GetType() : BehaviorType::NoneBehavior;
  BehaviorType newBehaviorType = nullptr != newBehavior ? newBehavior->GetType() : BehaviorType::NoneBehavior;
  bool oldBehaviorIsReactionary = nullptr != oldBehavior ? oldBehavior->IsReactionary() : false;
  bool newBehaviorIsReactionary = nullptr != newBehavior ? newBehavior->IsReactionary() : false;

  Anki::Util::sEvent("robot.behavior_transition",
                     {{DDATA, oldBehaviorName.c_str()}},
                     newBehaviorName.c_str());
  
  ExternalInterface::BehaviorTransition msg;
  msg.oldBehavior = oldBehaviorName;
  msg.newBehavior = newBehaviorName;
  msg.oldBehaviorType = oldBehaviorType;
  msg.newBehaviorType = newBehaviorType;
  msg.isOldReactionary = oldBehaviorIsReactionary;
  msg.isNewReactionary = newBehaviorIsReactionary;
  
  _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::BehaviorTransition>(msg);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::SwitchToBehavior(IBehavior* nextBehavior)
{
  if( _currentBehavior == nextBehavior ) {
    // nothing to do if the behavior hasn't changed
    return false;
  }

  StopCurrentBehavior();
  bool restartSuccess = true;
  IBehavior* oldBehavior = _currentBehavior;
  _currentBehavior = nullptr; // immediately clear current since we just stopped it

  if( nullptr != nextBehavior ) {
    const Result initRet = nextBehavior->Init();
    if ( initRet != RESULT_OK ) {
      // the previous behavior has been told to stop, but no new behavior has been started
      PRINT_NAMED_ERROR("BehaviorManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        nextBehavior->GetName().c_str());
      nextBehavior = nullptr;
      restartSuccess = false;
    }
    else {
      _currentBehavior = nextBehavior;
    }
  }
  
  // a null argument to this function means "switch to no behavior"
  SendDasTransitionMessage(oldBehavior, nextBehavior);
  return restartSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SwitchToNextBehavior()
{
  if( _behaviorToResume != nullptr ) {
    if( _shouldResumeBehaviorAfterReaction ) {
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
  
  SwitchToBehavior( _currentChooserPtr->ChooseNextBehavior(_robot) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SwitchToReactionaryBehavior(IReactionaryBehavior* nextBehavior)
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
    if( !_runningReactionaryBehavior ) {
      // by default, we do want to resume this behavior
      _shouldResumeBehaviorAfterReaction = true;
    }
    
    // if any reactionary behavior says that we shouldn't resume, then we won't
    _shouldResumeBehaviorAfterReaction = _shouldResumeBehaviorAfterReaction && nextBehavior->ShouldResumeLastBehavior();

    _runningReactionaryBehavior = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RequestEnableReactionaryBehavior(std::string requesterID, BehaviorType behavior, bool enable)
{
  // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
  if (!_robot.HasExternalInterface()) {
    return;
  }
  
  _robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::RequestEnableReactionaryBehavior>(requesterID,
                                                                                                        behavior,
                                                                                                        enable);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::Update()
{
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
    
  _currentChooserPtr->Update();

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
                          "Behavior '%s' returned  Status::Complete",
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
{
  if( _currentChooserPtr == newChooser ) {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.SetBehaviorChooser", "Null behavior chooser. Ignoring set (previous will continue)");
    return;
  }
  
  // notify all chooser it's no longer selected
  if ( _currentChooserPtr ) {
    _currentChooserPtr->OnDeselected();
  }

  // The behavior pointers may no longer be valid, so clear them
  SwitchToBehavior(nullptr);
  _behaviorToResume = nullptr;
  _runningReactionaryBehavior = false;

  _currentChooserPtr = newChooser;
  _currentChooserPtr->OnSelected();
  
  // mark the time at which the change happened (this is checked by behaviors)
  _lastChooserSwitchTime = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );

  // force the new behavior chooser to select something now, instead of waiting for the next tick
  SwitchToNextBehavior();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::ForceStopCurrentBehavior(const std::string& stoppedByWhom)
{
  PRINT_CH_INFO("Behaviors",
                "BehaviorManager.ForceStopCurrentBehavior",
                "Forcing current behavior to stop: %s",
                stoppedByWhom.c_str());
  SwitchToBehavior(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::StopCurrentBehavior()
{
  if ( nullptr != _currentBehavior && _currentBehavior->IsRunning() ) {
    _currentBehavior->Stop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message)
{
  switch (message.GetTag())
  {
    // Available games
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetAvailableGames:
    {
      const auto& msg = message.Get_SetAvailableGames();
      SetAvailableGame(msg.availableGames);
      break;
    }
    
    // Sparks
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetActiveSpark:
    {
      const auto& msg = message.Get_SetActiveSpark();
      SetActiveSpark(msg.behaviorSpark);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetActiveSpark(UnlockId spark)
{
  _activeSpark = spark;
}
  
} // namespace Cozmo
} // namespace Anki
