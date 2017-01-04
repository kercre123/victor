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
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoal.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalEvaluator.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/messageHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodDebug.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/behaviorChooserType.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#define DEBUG_BEHAVIOR_MGR 0

#if ANKI_DEV_CHEATS
  #define VIZ_BEHAVIOR_SELECTION  1
#else
  #define VIZ_BEHAVIOR_SELECTION  0
#endif // ANKI_DEV_CHEATS

#if VIZ_BEHAVIOR_SELECTION
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)  exp
#else
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)
#endif // VIZ_BEHAVIOR_SELECTION

namespace Anki {
namespace Cozmo {
  
static const char* kSelectionChooserConfigKey = "selectionBehaviorChooserConfig";
static const char* kFreeplayChooserConfigKey = "freeplayBehaviorChooserConfig";
static const char* kMeetCozmoChooserConfigKey = "meetCozmoBehaviorChooserConfig";
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::BehaviorManager(Robot& robot)
: _robot(robot)
, _defaultHeadAngle(kIgnoreDefaultHeandAndLiftState)
, _defaultLiftHeight(kIgnoreDefaultHeandAndLiftState)
, _behaviorFactory(new BehaviorFactory())
, _lastChooserSwitchTime(-1.0f)
, _audioClient( new Audio::BehaviorAudioClient(robot) )
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
  Util::SafeDelete(_behaviorFactory);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::InitConfiguration(const Json::Value &config)
{
  BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
  
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(!_isInitialized, "BehaviorManager.InitConfiguration.AlreadyInitialized");

  // create choosers
  if ( !config.isNull() )
  {
    // selection chooser - to force one specific behavior
    const Json::Value& selectionChooserConfigJson = config[kSelectionChooserConfigKey];
    _selectionChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, selectionChooserConfigJson);
    
    // freeplay chooser - AI controls cozmo
    const Json::Value& freeplayChooserConfigJson = config[kFreeplayChooserConfigKey];
    _freeplayChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, freeplayChooserConfigJson);

    // meetCozmo chooser
    const Json::Value& meetCozmoChooserConfigJson = config[kMeetCozmoChooserConfigKey];
    _meetCozmoChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, meetCozmoChooserConfigJson);
    
    // start with selection that defaults to NoneBehavior
    SetBehaviorChooser( _selectionChooser );

    BehaviorFactory& behaviorFactory = GetBehaviorFactory();
    
    uint8_t numEntriesOfExecutableType[(size_t)ExecutableBehaviorType::Count] = {0};

    for( const auto& it : behaviorFactory.GetBehaviorMap() ) {
      const auto& behaviorName = it.first;
      const auto& behaviorPtr = it.second;
      
      {
        const ExecutableBehaviorType executableBehaviorType = behaviorPtr->GetExecutableType();
        const size_t eBT = (size_t)executableBehaviorType;
        if (eBT < (size_t)ExecutableBehaviorType::Count)
        {
          DEV_ASSERT_MSG((numEntriesOfExecutableType[eBT] == 0), "ExecutableBehaviorType.NotUnique",
                         "Multiple behaviors marked as %s including '%s'",
                         EnumToString(executableBehaviorType), behaviorName.c_str());
          ++numEntriesOfExecutableType[eBT];
        }
      }
    
      if( behaviorPtr->IsBehaviorGroup( BehaviorGroup::Reactionary ) ) {
        PRINT_NAMED_DEBUG("BehaviorManager.EnableReactionaryBehavior",
                          "Adding behavior '%s' as reactionary",
                          behaviorName.c_str());
        IReactionaryBehavior*  reactionaryBehavior = behaviorPtr->AsReactionaryBehavior();
        if( nullptr == reactionaryBehavior ) {
          PRINT_NAMED_ERROR("BehaviorManager.ReactionaryBehaviorNotReactionary",
                            "behavior %s is in the factory (ptr %p), but isn't reactionary",
                            behaviorName.c_str(),
                            behaviorPtr);
          continue;
        }
        AddReactionaryBehavior(reactionaryBehavior);
      }
    }
    
    #if (DEV_ASSERT_ENABLED)
    for( size_t i = 0; i < (size_t)ExecutableBehaviorType::Count; ++i)
    {
      const ExecutableBehaviorType executableBehaviorType = (ExecutableBehaviorType)i;
      DEV_ASSERT_MSG((numEntriesOfExecutableType[i] == 1), "ExecutableBehaviorType.NotExactlyOne",
                     "Should be exactly 1 behavior marked as %s but found %u",
                     EnumToString(executableBehaviorType), numEntriesOfExecutableType[i]);
    }
    #endif
    
  }
    
  if (_robot.HasExternalInterface())
  {
    IExternalInterface* externalInterface = _robot.GetExternalInterface();
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::EnableReactionaryBehaviors,
                          [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                          {
                            SetReactionaryBehaviorsEnabled(event.GetData().Get_EnableReactionaryBehaviors().enabled, true);
                          }));
    _eventHandlers.push_back(externalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ActivateBehaviorChooser,
                               [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                               {
                                 const BehaviorChooserType chooserType =
                                   event.GetData().Get_ActivateBehaviorChooser().behaviorChooserType;
                                 switch (chooserType)
                                 {
                                   case BehaviorChooserType::Freeplay:
                                   {
                                     if( _firstTimeFreeplayStarted < 0 ) {
                                       const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                                       _firstTimeFreeplayStarted = currTime_s;
                                     }
                                     SetBehaviorChooser( _freeplayChooser );
                                     break;
                                   }
                                   case BehaviorChooserType::Selection:
                                   {
                                     SetBehaviorChooser( _selectionChooser );
                                     break;
                                   }
                                   case BehaviorChooserType::MeetCozmoFindFaces:
                                   {
                                     SetBehaviorChooser(_meetCozmoChooser);
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
    
    _eventHandlers.push_back(externalInterface->Subscribe(
                                ExternalInterface::MessageGameToEngineTag::SetDefaultHeadAndLiftState,
                                [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                                {
                                  bool enable = event.GetData().Get_SetDefaultHeadAndLiftState().enable;
                                  float headAngle = event.GetData().Get_SetDefaultHeadAndLiftState().headAngle;
                                  float liftHeight = event.GetData().Get_SetDefaultHeadAndLiftState().liftHeight;
                                  SetDefaultHeadAndLiftState(enable, headAngle, liftHeight);
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
  
  // Subscribe our own callback to these events
  RobotInterface::MessageHandler* robotInterface = _robot.GetRobotMessageHandler();
  for (auto tag : behavior->GetRobotToEngineTags())
  {
    _eventHandlers.push_back(
      robotInterface->Subscribe(_robot.GetID(),
                                tag,
                                std::bind(&BehaviorManager::ConsiderReactionaryBehaviorForEvent<RobotInterface::RobotToEngine>,
                                          this,
                                          std::placeholders::_1)));
  }
  
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetDefaultHeadAndLiftState(bool enable, f32 headAngle, f32 liftHeight)
{
  if(enable){
    _defaultHeadAngle = headAngle;
    _defaultLiftHeight = liftHeight;
    
    if(_robot.GetActionList().GetNumQueues() > 0 && _robot.GetActionList().GetQueueLength(0) == 0) {
      IActionRunner* moveHeadAction = new MoveHeadToAngleAction(_robot, _defaultHeadAngle);
      IActionRunner* moveLiftAction = new MoveLiftToHeightAction(_robot, _defaultLiftHeight);
      _robot.GetActionList().QueueAction(QueueActionPosition::NOW,
                                         new CompoundActionParallel(_robot, {moveHeadAction, moveLiftAction}));
    }
    
  }else{
    _defaultHeadAngle = kIgnoreDefaultHeandAndLiftState;
    _defaultLiftHeight = kIgnoreDefaultHeandAndLiftState;
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
      if ( !behavior->IsRunning() &&
           ( !_runningReactionaryBehavior || behavior->ShouldInterruptOtherReactionaryBehavior() ) && 
           behavior->ShouldRunForEvent(event.GetData(),_robot) &&
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
  msg.newBehaviorDisplayKey = newBehavior ? newBehavior->GetDisplayNameKey() : "";
  
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

  if( nullptr != nextBehavior ) {
    VIZ_BEHAVIOR_SELECTION_ONLY(
      _robot.GetContext()->GetVizManager()->SendNewBehaviorSelected(
        VizInterface::NewBehaviorSelected( nextBehavior->GetName() )));
  }
  
  // a null argument to this function means "switch to no behavior"
  SendDasTransitionMessage(oldBehavior, nextBehavior);
  return restartSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::ChooseNextBehaviorAndSwitch()
{
  // we can't call ChooseNextBehaviorAndSwitch while there's a behavior to resume pending. Call TryToResumeBehavior instead
  DEV_ASSERT( _behaviorToResume == nullptr,
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CalledWithResumeBehaviorPending");

  // shouldn't call ChooseNextBehaviorAndSwitch after a reactionary. Call TryToResumeBehavior instead
  DEV_ASSERT( (_currentBehavior == nullptr) || !_currentBehavior->IsReactionary(),
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CantSelectAfterReactionary");

  // the current behavior has to be running. Otherwise current should be nullptr
  DEV_ASSERT( (_currentBehavior == nullptr) || _currentBehavior->IsRunning(),
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CurrentBehaviorIsNotRunning");
 
  // ask the current chooser for the next behavior
  IBehavior* nextBehavior = _currentChooserPtr->ChooseNextBehavior(_robot, _currentBehavior);
  SwitchToBehavior( nextBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::TryToResumeBehavior()
{
  // Return the robot's head and lift to their height when the behavior that's being resumed was interrupted
  // this is a convenience for behaviors like MeetCozmo when interrupted by reactions with animations
  // we don't care if it actually completes so if the behavior immediately overrides these that's fine
  // if a behavior checks the head or lift angle right as they start this may introduce a race condition
  // blame Brad
  
  if(AreDefaultHeandAndLiftStateSet() &&
     (_robot.GetActionList().GetNumQueues() > 0 && _robot.GetActionList().GetQueueLength(0) == 0))
  {
    IActionRunner* moveHeadAction = new MoveHeadToAngleAction(_robot, _defaultHeadAngle);
    IActionRunner* moveLiftAction = new MoveLiftToHeightAction(_robot, _defaultLiftHeight);
    _robot.GetActionList().QueueAction(QueueActionPosition::NOW,
                                       new CompoundActionParallel(_robot, {moveHeadAction, moveLiftAction}));
  }
    
  if ( _behaviorToResume != nullptr )
  {
    if ( _shouldResumeBehaviorAfterReaction )
    {
      StopCurrentBehavior();
      BehaviorType resumingFromType = BehaviorType::NoneBehavior;
      if(nullptr != _currentBehavior){
        resumingFromType = _currentBehavior->GetType();
      }
      const Result resumeResult = _behaviorToResume->Resume(resumingFromType);
      if( resumeResult == RESULT_OK )
      {
        PRINT_CH_INFO("Behaviors", "BehaviorManager.ResumeBehavior", "Successfully resumed '%s'",
          _behaviorToResume->GetName().c_str());
        // the behavior can resume, set as current again
        SendDasTransitionMessage(_currentBehavior, _behaviorToResume);
        _currentBehavior  = _behaviorToResume;
        _behaviorToResume = nullptr;
        
        // Successfully resumed the previous behavior, return here
        return;
      }
      else {
        PRINT_CH_INFO("Behaviors", "BehaviorManager.ResumeFailed",
          "Tried to resume behavior '%s', but failed. Clearing current behavior",
          _behaviorToResume->GetName().c_str() );
      }
    }
    
    // we didn't resume, clear the resume behavior because we won't want to resume it after the next behavior
    _behaviorToResume = nullptr;
  }
  
  // we did not resume, clear current behavior
  SwitchToBehavior( nullptr );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SwitchToReactionaryBehavior(IReactionaryBehavior* nextBehavior)
{
  // a null here means "no reaction", not "switch to the null behavior"
  if( nullptr == nextBehavior ) {
    return;
  }
  
  if( nullptr == _behaviorToResume &&
      nullptr != _currentBehavior &&
      !_currentBehavior->IsReactionary() ) {
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
void BehaviorManager::RequestEnableReactionaryBehavior(const std::string& requesterID, BehaviorType behavior, bool enable)
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
Result BehaviorManager::Update(Robot& robot)
{
  ANKI_CPU_PROFILE("BehaviorManager::Update");
  
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
    
  _currentChooserPtr->Update(robot);

  UpdateTappedObject();
  
  // Update the current behavior if a new object was double tapped
  if(_needToHandleObjectTapped)
  {
    UpdateBehaviorWithObjectTapInteraction();
    _needToHandleObjectTapped = false;
  }

  if( !_runningReactionaryBehavior )
  {
    ChooseNextBehaviorAndSwitch();
  }
  
  // Allow reactionary behaviors to request a switch without a message
  CheckForComputationalSwitch();
    
  if ( nullptr != _currentBehavior ) {
    
    // We have a current behavior, update it.
    const IBehavior::Status status = _currentBehavior->Update();
     
    switch(status)
    {
      case IBehavior::Status::Running:
        // Nothing to do! Just keep on truckin'....
        break;
          
      case IBehavior::Status::Complete:
        // behavior is complete, switch to null (will also handle stopping current). If it was reactionary,
        // switch now to give the last behavior a chance to resume (if appropriate)
        PRINT_CH_DEBUG("Behaviors", "BehaviorManager.Update.BehaviorComplete",
                          "Behavior '%s' returned  Status::Complete",
                          _currentBehavior->GetName().c_str());
        if ( _runningReactionaryBehavior ) {
          _runningReactionaryBehavior = false;
          TryToResumeBehavior();
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
          TryToResumeBehavior();
        }
        else {
          SwitchToBehavior(nullptr);
        }
        break;
    } // switch(status)
  }
    
  return lastResult;
} // Update()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
{
  if( _currentChooserPtr == newChooser) {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.SetBehaviorChooser",
      "BehaviorChooser '%s' already set",
      newChooser ? newChooser->GetName() : "null");
    return;
  }
  
  // notify all chooser it's no longer selected
  if ( _currentChooserPtr ) {
    _currentChooserPtr->OnDeselected();
  }
  
  // default head and lift states should not be preserved between choosers
  DEV_ASSERT(!AreDefaultHeandAndLiftStateSet(),
             "BehaviorManager.ChooseNextBehaviorAndSwitch.DefaultHeadAndLiftStatesStillSet");

  bool currentNotReactionary = !(_currentBehavior != nullptr && _currentBehavior->IsReactionary());
  
  // The behavior pointers may no longer be valid, so clear them
  if(currentNotReactionary){
    SwitchToBehavior(nullptr);
    _runningReactionaryBehavior = false;
  }
  
  _behaviorToResume = nullptr;
  
  // ensure spraks are re-set when the chooser changes
  _activeSpark = UnlockId::Count;
  _lastRequestedSpark = UnlockId::Count;

  // channeled log and event
  LOG_EVENT("BehaviorManager.SetBehaviorChooser", "Switching behavior chooser from '%s' to '%s'",
            _currentChooserPtr ? _currentChooserPtr->GetName() : "null",
            newChooser->GetName());
  
  PRINT_CH_INFO("Behaviors",
                "BehaviorManager.SetBehaviorChooser",
                "Switching behavior chooser from '%s' to '%s'",
                _currentChooserPtr ? _currentChooserPtr->GetName() : "null",
                newChooser->GetName());

  _currentChooserPtr = newChooser;
  _currentChooserPtr->OnSelected();
  
  // mark the time at which the change happened (this is checked by behaviors)
  _lastChooserSwitchTime = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );

  // force the new behavior chooser to select something now, instead of waiting for the next tick
  if(currentNotReactionary){
    ChooseNextBehaviorAndSwitch();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RequestCurrentBehaviorEndImmediately(const std::string& stoppedByWhom)
{
  PRINT_CH_INFO("Behaviors",
                "BehaviorManager.RequestCurrentBehaviorEndImmediately",
                "Forcing current behavior to stop: %s",
                stoppedByWhom.c_str());
  SwitchToBehavior(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IReactionaryBehavior* BehaviorManager::GetReactionaryBehaviorByType(BehaviorType behaviorType)
{
  for(IReactionaryBehavior* reactionaryBehavior : _reactionaryBehaviors)
  {
    if(reactionaryBehavior->GetType() == behaviorType)
    {
      return reactionaryBehavior;
    }
  }
  
  return nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::StopCurrentBehavior()
{
  if ( nullptr != _currentBehavior && _currentBehavior->IsRunning() ) {
    _currentBehavior->Stop();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::CheckForComputationalSwitch()
{
  if( !_reactionsEnabled )
  {
    return;
  }
  //Check to see if any reactionary behaviors want to perform a computational switch
  bool hasSwitched = false;
  for( auto rBehavior: _reactionaryBehaviors){
    if(!rBehavior->IsRunning()
       && rBehavior->IsRunnable(_robot)
       && ( !_runningReactionaryBehavior || rBehavior->ShouldInterruptOtherReactionaryBehavior() )
       && rBehavior->ShouldComputationallySwitch(_robot)){
          SwitchToReactionaryBehavior(rBehavior);
          if(hasSwitched){
            PRINT_NAMED_WARNING("BehaviorManager.Update.ReactionaryBehaviors",
                              "Multiple behaviors switched to in a single basestation tick");
          }
          hasSwitched = true;
    }
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
    case ExternalInterface::BehaviorManagerMessageUnionTag::ActivateSpark:
    {
      const auto& msg = message.Get_ActivateSpark();
      SetRequestedSpark(msg.behaviorSpark, false);
      if(msg.behaviorSpark == UnlockId::Count){
        _didGameRequestSparkEnd = true;
      }
      break;
    }
    
    case ExternalInterface::BehaviorManagerMessageUnionTag::SparkUnlocked:
    {
      const auto& msg = message.Get_SparkUnlocked();
      SetRequestedSpark(msg.behaviorSpark, true);
      break;
    }
      
    case ExternalInterface::BehaviorManagerMessageUnionTag::ActivateSparkedMusic:
    {
      const auto& msg = message.Get_ActivateSparkedMusic();
      if ( !_audioClient->ActivateSparkedMusic(msg.behaviorSpark, msg.musicSate, msg.sparkedMusicState) ) {
        PRINT_NAMED_ERROR("BehaviorManager.HandleMessage.ActivateSparkedMusic.Failed",
                          "UnlockId %s", EnumToString(msg.behaviorSpark));
      }
      break;
    }
      
    case ExternalInterface::BehaviorManagerMessageUnionTag::DeactivateSparkedMusic:
    {
      const auto& msg = message.Get_DeactivateSparkedMusic();
      if ( !_audioClient->DeactivateSparkedMusic(msg.behaviorSpark, msg.musicSate) ) {
        PRINT_NAMED_ERROR("BehaviorManager.HandleMessage.DeactivateSparkedMusic.Failed",
                          "UnlockId %s", EnumToString(msg.behaviorSpark));
      }
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
void BehaviorManager::CalculateFreeplayGoalFromObjects()
{
  // design: the choosers are generic, but this only makes sense if the freeplay chooser is an AIGoalEvaluator. I
  // think that we should not be able to data-drive it, but there may be a case for it, and this method
  // should just not work in that case. Right now I feel like using dynamic_cast, but we could provide default
  // implementation in chooser interface that asserts to not be used.
  AIGoalEvaluator* freeplayGoalEvaluator = dynamic_cast<AIGoalEvaluator*>(_freeplayChooser);
  if ( nullptr != freeplayGoalEvaluator )
  {
    freeplayGoalEvaluator->CalculateDesiredGoalFromObjects();
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorManager.CalculateFreeplayGoalFromObjects.NotAGoalEvaluator",
      "The current freeplay chooser is not a goal evaluator.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetRequestedSpark(UnlockId spark, bool softSpark)
{
  _lastRequestedSpark = spark;
  _isRequestedSparkSoft = softSpark;

  PRINT_CH_INFO("Behaviors", "BehaviorManager.SetRequestedSpark",
                "requested %s spark is '%s'",
                _isRequestedSparkSoft ? "soft" : "hard",
                UnlockIdToString(_lastRequestedSpark));
}
  
const UnlockId BehaviorManager::SwitchToRequestedSpark()
{
  PRINT_CH_INFO("Behaviors", "BehaviorManager.SwitchToRequestedSpark",
                "switching active spark from '%s' -> '%s'. From %s -> %s",
                UnlockIdToString(_activeSpark),
                UnlockIdToString(_lastRequestedSpark),
                _isSoftSpark ? "soft" : "hard",
                _isRequestedSparkSoft ? "soft" : "hard");

  _lastChooserSwitchTime = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );
  
  _activeSpark = _lastRequestedSpark;
  _isSoftSpark = _isRequestedSparkSoft;
  _didGameRequestSparkEnd = false;
  
  return _activeSpark;
}
  
void BehaviorManager::SetReactionaryBehaviorsEnabled(bool isEnabled, bool stopCurrent)
{
  _reactionsEnabled = isEnabled;
  PRINT_CH_INFO("Behaviors", "BehaviorManager.SetReactionaryBehaviorsEnabled", (_reactionsEnabled ? "Enabled" : "Disabled"));
  
  if(stopCurrent && !_reactionsEnabled && _runningReactionaryBehavior)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.SetReactionaryBehaviorsEnabled", "Disabling reactionary behaviors stopping currently running one");
    SwitchToBehavior(nullptr);
  }
  
}
  
void BehaviorManager::RequestCurrentBehaviorEndOnNextActionComplete()
{
  if(nullptr != _currentBehavior){
    _currentBehavior->StopOnNextActionComplete();
  }
}

void BehaviorManager::HandleObjectTapInteraction(const ObjectID& objectID)
{
  // Have to be in freeplay and not picking or placing and flat on the ground
  if(_currentChooserPtr != _freeplayChooser ||
     _robot.IsPickingOrPlacing() ||
     _robot.GetOffTreadsState() != OffTreadsState::OnTreads)
  {
    PRINT_CH_INFO("Behaviors", "HandleObjectTapInteraction.CantRun",
                  "Robot is not in freeplay chooser or is picking and placing or not on treads");
    return;
  }

  if(_activeSpark != UnlockId::Count)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.InSpark",
                  "Currently in spark not switching to double tap");
    return;
  }
  
  // We can only interrupt the react to double tap reactionary behavior
  if(_runningReactionaryBehavior &&
      _currentBehavior->GetType() != BehaviorType::ReactToDoubleTap)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.InReaction",
                  "Currently in reaction not switching to double tap");
    return;
  }

  if( !_tapInteractionDisabledIDs.empty() ) {
    PRINT_CH_INFO("Behavior", "BehaviorManager.HandleObjectTapInteraction.Disabled",
                  "Object tap interaction disabled, ignoring tap");
    return;
  }
  
  _needToHandleObjectTapped = true;
  _pendingDoubleTappedObject = objectID;
  
}

void BehaviorManager::LeaveObjectTapInteraction()
{
  if(!_robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.LeaveObjectTapInteraction.NoTapIntent", "");
    return;
  }
  
  if(_currentChooserPtr != nullptr &&
    _currentChooserPtr->SupportsObjectTapInteractions())
  {
    PRINT_CH_INFO("Behaviors", "LeaveObjectTapInteration", "");
    
    auto* chooser = dynamic_cast<AIGoalEvaluator*>(_currentChooserPtr);
    
    // It is possible that we have requested the ObjectTapInteraction goal but it hasn't actually
    // been selected so we need to clear the requested goal
    if(chooser != nullptr)
    {
      chooser->ClearObjectTapInteractionRequestedGoal();
    }
    else
    {
      PRINT_NAMED_ERROR("BehaviorManager.LeaveObjectTapInteraction.NullChooser",
                        "Current chooser is not an AIGoalEvaluator but supports object tap interactions");
    }
  }
  
  _robot.GetAIComponent().GetWhiteboard().ClearObjectTapInteraction();
  
  _lastDoubleTappedObject.UnSet();
  _currDoubleTappedObject.UnSet();
}

void BehaviorManager::RequestEnableTapInteraction(const std::string& requesterID, bool enable)
{
  if( enable ) {
    size_t countRemoved = _tapInteractionDisabledIDs.erase(requesterID);
    if( countRemoved == 0 ) {
      PRINT_NAMED_WARNING("BehaviorManager.RequestEnableTapInteraction.IDNotFound",
                          "Attempted to enable tap interaction with invalid id '%s'",
                          requesterID.c_str());
    }
  }
  else {
    size_t countInList = _tapInteractionDisabledIDs.count(requesterID);
    if( countInList > 0 ) {
      PRINT_NAMED_WARNING("BehaviorManager.RequestEnableTapInteraction.DuplicateID",
                          "Trying to disable tap interaction with ID previously registered: '%s'",
                          requesterID.c_str());
    }
    _tapInteractionDisabledIDs.insert(requesterID);
  }
}

void BehaviorManager::UpdateTappedObject()
{
  // If the tapped objects pose becomes unknown and we aren't currently in ReactToDoubleTap
  // (we expect the pose to be unknown/dirty when ReactToDoubleTap is running) then give up and
  // leave object tap interaction
  if(_robot.GetAIComponent().GetWhiteboard().HasTapIntent() &&
     _currentBehavior != nullptr &&
     _currentBehavior->GetType() != BehaviorType::ReactToDoubleTap &&
     _tapInteractionDisabledIDs.empty())
  {
    const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_currDoubleTappedObject);
    if(object != nullptr && object->IsPoseStateUnknown())
    {
      LeaveObjectTapInteraction();
    }
  }
}

void BehaviorManager::UpdateBehaviorWithObjectTapInteraction()
{
  // Copy pending object to be assigned to current double tapped object
  const ObjectID objectID = _pendingDoubleTappedObject;

  if(!_currentChooserPtr->SupportsObjectTapInteractions())
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.SupportTapInteraction",
                  "Current chooser does not support object tap interactions");
    return;
  }
  
  auto* chooser = dynamic_cast<AIGoalEvaluator*>(_currentChooserPtr);
  
  if(chooser == nullptr)
  {
    PRINT_NAMED_ERROR("BehaviorManager.HandleObjectTapInteraction.NullChooser",
                      "Freeplay chooser is not an AIGoalEvaluator or is null");
    return;
  }
  
  // Tell whiteboard we have a object with tap intention
  _robot.GetAIComponent().GetWhiteboard().SetObjectTapInteraction(objectID);
  
  if(_currDoubleTappedObject == objectID)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.CurrTappedObject",
                  "Tapped object %u is the current tapped object not doing anything",
                  objectID.GetValue());
    return;
  }
  
  // If the current goal is not the ObjectTapInteraction goal
  // then update the behavior chooser and request the ObjectTapInteraction goal
  if(!chooser->IsCurrentGoalObjectTapInteraction())
  {
    // Switch to ObjectTapInteraction goal
    chooser->SwitchToObjectTapInteractionGoal();
    
    _lastDoubleTappedObject = _currDoubleTappedObject;
    _currDoubleTappedObject = objectID;
  }
  // Otherwise we are already in the ObjectTapInteraction goal
  else
  {
    // The current behavior of the goal is not NoneBehavior
    if(nullptr != _currentBehavior && _currentBehavior->GetType() != BehaviorType::NoneBehavior)
    {
      // Update the interaction object
      _lastDoubleTappedObject = _currDoubleTappedObject;
      _currDoubleTappedObject = objectID;
      
      // If the current behavior can't handle the new tapped object
      if(!_currentBehavior->HandleNewDoubleTap(_robot))
      {
        PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.BehaviorNotRunnable",
                      "%s is no longer runnable after updating target blocks",
                      _currentBehavior->GetName().c_str());
        
        // If the current behavior is a reactionary behavior (ReactToDoubleTap) then
        // make sure to update _runningReactionaryBehavior and try to resume the last behavior
        if(_currentBehavior->IsReactionary())
        {
          // The only possible reactionary behavior that can be interrupted with a double tap is
          // ReactToDouble
          DEV_ASSERT(_currentBehavior->GetType() == BehaviorType::ReactToDoubleTap,
                     "Current reactionary behavior should only be ReactToDoubleTap");
          
          _runningReactionaryBehavior = false;
          
          TryToResumeBehavior();
          
          // The behavior to resume becomes the currentBehavior so we need to stop, updateTargetBlocks,
          // and re-init it to make sure the resumed behavior has correct target blocks and the cube lights get
          // updated
          // If we are resuming NoneBehavior then just quit object tap interaction
          if(_currentBehavior != nullptr && _currentBehavior->GetType() != BehaviorType::NoneBehavior)
          {
            if(!_currentBehavior->HandleNewDoubleTap(_robot))
            {
              PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.ResumeFailed",
                            "%s can't run after being resumed",
                            _currentBehavior->GetName().c_str());
              LeaveObjectTapInteraction();
            }
          }
          else
          {
            PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.ResumingNoneBehavior",
                          "Was in ReactToDoubleTap, got another double tap and am now trying to resume NoneBehavior");
            LeaveObjectTapInteraction();
          }
          return;
        }
        
        // If, for some reason, the object tap interaction behavior that was running is no longer runnable
        // then we should start the ReactToDoubleTap behavior to try to find the tapped object.
        // This is necessary because otherwise the ObjectTapInteraction goal will exit due to not being able
        // to pick a behavior (all of the ObjectTapInteraction behaviors have similar requirements to run) so
        // if the current one can't run then the others probably won't be able to run either.
        auto reactToDoubleTap = _robot.GetBehaviorFactory().FindBehaviorByType(BehaviorType::ReactToDoubleTap);
        if(reactToDoubleTap->IsRunnable(_robot))
        {
          PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.StartingReactToDoubleTap",
                        "Forcing ReactToDoubleTap to run because %s can't run with newly double tapped object",
                        _currentBehavior->GetName().c_str());
          
          _currentBehavior = reactToDoubleTap;
          _currentBehavior->Init();
          _runningReactionaryBehavior = true;
          return;
        }
        
        // This will cause the current tapInteraction behavior to end and a new one to be picked
        _currentBehavior = nullptr;
        return;
      }
    }
  }

}
  
} // namespace Cozmo
} // namespace Anki
