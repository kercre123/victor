
/**
 * File: behaviorManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/demoBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/behaviorReactToPickup.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#define DEBUG_BEHAVIOR_MGR 0

namespace Anki {
namespace Cozmo {
  
  
  BehaviorManager::BehaviorManager(Robot& robot)
  : _isInitialized(false)
  , _forceReInit(false)
  , _robot(robot)
  , _minBehaviorTime_sec(5)
  {

  }
  
  Result BehaviorManager::Init(const Json::Value &config)
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
    
    // TODO: Set configuration data from Json...
    
    // TODO: Only load behaviors specified by Json?
    
    SetupBehaviorChooser(config);
    
    _isInitialized = true;
    
    _lastSwitchTime_sec = 0.f;
    
    return RESULT_OK;
  }
  
  void BehaviorManager::SetupBehaviorChooser(const Json::Value &config)
  {
    DemoBehaviorChooser* newDemoChooser = new DemoBehaviorChooser(_robot, config);
    _behaviorChooser = newDemoChooser;
    
    AddReactionaryBehavior(new BehaviorReactToPickup(_robot, config));
  }
  
  // The AddReactionaryBehavior wrapper is responsible for setting up the callbacks so that important events will be
  // reacted to correctly - events will be given to the Chooser which may return a behavior to force switch to
  void BehaviorManager::AddReactionaryBehavior(IReactionaryBehavior* behavior)
  {
    // We map reactionary behaviors to the tag types they're going to care about
    _behaviorChooser->AddReactionaryBehavior(behavior);
    
    // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
    if (!_robot.HasExternalInterface()) {
      return;
    }
    
    // Callback for EngineToGame event that a reactionary behavior (possibly) cares about
    auto reactionsEngineToGameCallback = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
    {
      _forceSwitchBehavior = _behaviorChooser->GetReactionaryBehavior(event);
    };
    
    // Subscribe our own callback to these events
    IExternalInterface* interface = _robot.GetExternalInterface();
    for (auto tag : behavior->GetEngineToGameTags())
    {
      _eventHandlers.push_back(interface->Subscribe(tag, reactionsEngineToGameCallback));
    }
    
    // Callback for GameToEngine event that a reactionary behavior (possibly) cares about
    auto reactionsGameToEngineCallback = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
    {
      _forceSwitchBehavior = _behaviorChooser->GetReactionaryBehavior(event);
    };
    
    // Subscribe our own callback to these events
    for (auto tag : behavior->GetGameToEngineTags())
    {
      _eventHandlers.push_back(interface->Subscribe(tag, reactionsGameToEngineCallback));
    }
  }
  
  BehaviorManager::~BehaviorManager()
  {
    Util::SafeDelete(_behaviorChooser);
  }
  
  void BehaviorManager::SwitchToNextBehavior(double currentTime_sec)
  {
    // If we're currently running our forced behavior but now switching away, clear it
    if (_currentBehavior == _forceSwitchBehavior)
    {
      _forceSwitchBehavior = nullptr;
    }
    
    // Initialize next behavior and make it the current one
    if (nullptr != _nextBehavior && _currentBehavior != _nextBehavior) {
      if (_nextBehavior->Init(currentTime_sec) != RESULT_OK) {
        PRINT_NAMED_ERROR("BehaviorManager.SwitchToNextBehavior.InitFailed",
                          "Failed to initialize %s behavior.",
                          _nextBehavior->GetName().c_str());
      }
      _currentBehavior = _nextBehavior;
      _nextBehavior = nullptr;
    }
  }
  
  Result BehaviorManager::Update(double currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
      return RESULT_FAIL;
    }
    
    _behaviorChooser->Update(currentTime_sec);
    
    // If we happen to have a behavior we really want to switch to, do so
    if (nullptr != _forceSwitchBehavior && _currentBehavior != _forceSwitchBehavior)
    {
      _nextBehavior = _forceSwitchBehavior;
      
      lastResult = InitNextBehaviorHelper(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.InitForcedBehavior",
                            "Failed trying to force next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
    }
    else if (nullptr == _currentBehavior ||
       currentTime_sec - _lastSwitchTime_sec > _minBehaviorTime_sec)
    {
      // We've been in the current behavior long enough to consider switching
      lastResult = SelectNextBehavior(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.SelectNextFailed",
                            "Failed trying to select next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
      
      
      if (_currentBehavior != _nextBehavior && nullptr != _nextBehavior)
      {
        std::string nextName = _nextBehavior->GetName();
        BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Update.SelectedNext",
                               "Selected next behavior '%s' at t=%.1f, last was t=%.1f",
                               nextName.c_str(), currentTime_sec, _lastSwitchTime_sec);
        
        _lastSwitchTime_sec = currentTime_sec;
      }
    }
    
    if(nullptr != _currentBehavior) {
      // We have a current behavior, update it.
      IBehavior::Status status = _currentBehavior->Update(currentTime_sec);
     
      switch(status)
      {
        case IBehavior::Status::Running:
          // Nothing to do! Just keep on truckin'....
          _currentBehavior->SetIsRunning(true);
          break;
          
        case IBehavior::Status::Complete:
          // Behavior complete, switch to next
          _currentBehavior->SetIsRunning(false);
          SwitchToNextBehavior(currentTime_sec);
          break;
          
        case IBehavior::Status::Failure:
          PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                            "Behavior '%s' failed to Update().",
                            _currentBehavior->GetName().c_str());
          lastResult = RESULT_FAIL;
          _currentBehavior->SetIsRunning(false);
          
          // Force a re-init so if we reselect this behavior
          _forceReInit = true;
          SelectNextBehavior(currentTime_sec);
          break;
          
        default:
          PRINT_NAMED_ERROR("BehaviorManager.Update.UnknownStatus",
                            "Behavior '%s' returned unknown status %d",
                            _currentBehavior->GetName().c_str(), status);
          lastResult = RESULT_FAIL;
      } // switch(status)
    }
    else if(nullptr != _nextBehavior) {
      // No current behavior, but next behavior defined, so switch to it.
      SwitchToNextBehavior(currentTime_sec);
    }
    
    return lastResult;
  } // Update()
  
  
  Result BehaviorManager::InitNextBehaviorHelper(float currentTime_sec)
  {
    Result initResult = RESULT_OK;
    
    // Initialize the selected behavior, if it's not the one we're already running
    if(_nextBehavior != _currentBehavior || _forceReInit)
    {
      _forceReInit = false;
      if(nullptr != _currentBehavior) {
        // Interrupt the current behavior that's running if there is one. It will continue
        // to run on calls to Update() until it completes and then we will switch
        // to the selected next behavior
        initResult = _currentBehavior->Interrupt(currentTime_sec);
        
        if (nullptr != _nextBehavior)
        {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManger.InitNextBehaviorHelper.Selected",
                                 "Selected %s to run next.", _nextBehavior->GetName().c_str());
        }
      }
    }
    return initResult;
  }
  
  Result BehaviorManager::SelectNextBehavior(double currentTime_sec)
  {
    
    _nextBehavior = _behaviorChooser->ChooseNextBehavior(currentTime_sec);
    if(nullptr == _nextBehavior) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.NoneRunnable", "");
      return RESULT_FAIL;
    }
    
    // Initialize the selected behavior
    return InitNextBehaviorHelper(currentTime_sec);
    
  } // SelectNextBehavior()
  
  Result BehaviorManager::SelectNextBehavior(const std::string& name, double currentTime_sec)
  {
    _nextBehavior = _behaviorChooser->GetBehaviorByName(name);
    if(nullptr == _nextBehavior) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.UnknownName",
                        "No behavior named '%s'", name.c_str());
      return RESULT_FAIL;
    }
    else if(_nextBehavior->IsRunnable(currentTime_sec) == false) {
      PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NotRunnable",
                        "Behavior '%s' is not runnable.", name.c_str());
      return RESULT_FAIL;
    }
    
    return InitNextBehaviorHelper(currentTime_sec);
  }
  
  void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
  {
    Util::SafeDelete(_behaviorChooser);
    _behaviorChooser = newChooser;
  }
  
} // namespace Cozmo
} // namespace Anki
