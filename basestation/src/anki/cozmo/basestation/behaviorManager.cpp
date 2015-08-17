
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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"

#include "anki/cozmo/basestation/robot.h"

#include "util/logging/logging.h"

#define DEBUG_BEHAVIOR_MGR 1

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
#   if DEBUG_BEHAVIOR_MGR
    PRINT_NAMED_INFO("BehaviorManager.Init.Initializing", "");
#   endif
    
    // TODO: Set configuration data from Json...
    
    // TODO: Only load behaviors specified by Json?
    
    AddBehavior(new BehaviorOCD(_robot, config));
    //AddBehavior(new BehaviorLookForFaces(_robot, config));
    //AddBehavior(new BehaviorLookAround(_robot, config));
    AddBehavior(new BehaviorFidget(_robot, config));
    
    // These need to be set AFTER behaviors have been added or else they're invalid
    _currentBehavior = _nextBehavior = _behaviors.end();
    _isInitialized = true;
    
    _lastSwitchTime_sec = 0.f;
    
    return RESULT_OK;
  }
  
  BehaviorManager::~BehaviorManager()
  {
    for(auto behavior : _behaviors) {
      if(behavior.second == nullptr) {
        PRINT_NAMED_ERROR("BehaviorManager.Destructor.ExistingNull",
                          "Existing '%s' behavior is somehow NULL.\n",
                          behavior.first.c_str());
      } else {
#       if DEBUG_BEHAVIOR_MGR
        PRINT_NAMED_INFO("BehaviorManager.Destructor", "Deleting behavior %s.\n",
                         behavior.first.c_str());
#       endif
        
        delete behavior.second;
      }
    }
  }
  
  Result BehaviorManager::AddBehavior(IBehavior *newBehavior)
  {
    if(newBehavior == nullptr) {
      PRINT_NAMED_ERROR("BehaviorManager.AddBehavior.NullBehavior", "Refusing to add NULL behavior.");
      return RESULT_FAIL;
    }
    
    const std::string& name = newBehavior->GetName();
    
    // Make sure to delete any existing behavior with the same name before replacing
    auto existingBehavior = _behaviors.find(name);
    if(existingBehavior != _behaviors.end()) {
      PRINT_NAMED_WARNING("BehaviorManager.AddBehavior.ReplaceExisting",
                          "Replacing existing '%s' behavior.", name.c_str());
      if(existingBehavior->second == nullptr) {
        PRINT_NAMED_ERROR("BehaviorManager.AddBehavior.ExistingNull",
                          "Existing '%s' behavior is somehow NULL.",
                          existingBehavior->first.c_str());
      } else {
        delete existingBehavior->second;
      }
    }
    
#   if DEBUG_BEHAVIOR_MGR
    PRINT_NAMED_INFO("BehaviorManager.AddBehavior.Add", "Adding behavior %s", name.c_str());
#   endif
    
    _behaviors[name] = newBehavior;
    
    return RESULT_OK;
  }
  
  void BehaviorManager::SwitchToNextBehavior()
  {
    _currentBehavior = _nextBehavior;
    _nextBehavior = _behaviors.end();
  }
  
  Result BehaviorManager::Update(float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
      return RESULT_FAIL;
    }
    
    if(_currentBehavior == _behaviors.end() ||
       currentTime_sec - _lastSwitchTime_sec > _minBehaviorTime_sec)
    {
      // We've been in the current behavior long enough to consider switching
      lastResult = SelectNextBehavior(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.SelectNextFailed",
                            "Failed trying to select next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
      
#     if DEBUG_BEHAVIOR_MGR
      PRINT_NAMED_WARNING("BehaviorManager.Update.SelectedNext",
                          "Selected next behavior '%s' at t=%.1f, last was t=%.1f",
                          _nextBehavior->first.c_str(), currentTime_sec, _lastSwitchTime_sec);
#     endif
      
      _lastSwitchTime_sec = currentTime_sec;
    }
    
    if(_currentBehavior != _behaviors.end()) {
      // We have a current behavior, update it.
      IBehavior::Status status = _currentBehavior->second->Update(currentTime_sec);
     
      switch(status)
      {
        case IBehavior::Status::RUNNING:
          // Nothing to do! Just keep on truckin'....
          _currentBehavior->second->SetIsRunning(true);
          break;
          
        case IBehavior::Status::COMPLETE:
          // Behavior complete, switch to next
          _currentBehavior->second->SetIsRunning(false);
          SwitchToNextBehavior();
          break;
          
        case IBehavior::Status::FAILURE:
          PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                            "Behavior '%s' failed to Update().",
                            _currentBehavior->first.c_str());
          lastResult = RESULT_FAIL;
          _currentBehavior->second->SetIsRunning(false);
          
          // Force a re-init so if we reselect this behavior
          _forceReInit = true;
          SelectNextBehavior(currentTime_sec);
          break;
          
        default:
          PRINT_NAMED_ERROR("BehaviorManager.Update.UnknownStatus",
                            "Behavior '%s' returned unknown status %d",
                            _currentBehavior->first.c_str(), status);
          lastResult = RESULT_FAIL;
      } // switch(status)
    }
    else if(_nextBehavior != _behaviors.end()) {
      // No current behavior, but next behavior defined, so switch to it.
      SwitchToNextBehavior();
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
      initResult = _nextBehavior->second->Init();
      if(initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("BehaviorManager.InitNextBehaviorHelper.InitFailed",
                          "Failed to initialize %s behavior.",
                          _nextBehavior->second->GetName().c_str());
        _nextBehavior = _behaviors.end();
      } else if(_currentBehavior != _behaviors.end()) {
        // We've successfully initialized the next behavior to run, so interrupt
        // the current behavior that's running if there is one. It will continue
        // to run on calls to Update() until it completes and then we will switch
        // to the selected next behavior
        initResult = _currentBehavior->second->Interrupt(currentTime_sec);
        
#       if DEBUG_BEHAVIOR_MGR
        PRINT_NAMED_INFO("BehaviorManger.InitNextBehaviorHelper.Selected",
                         "Selected %s to run next.", _nextBehavior->first.c_str());
#       endif
      }
    }
    return initResult;
  }
  
  Result BehaviorManager::SelectNextBehavior(float currentTime_sec)
  {
    
    if(_behaviors.empty()) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.NoBehaviors",
                        "Empty behavior container.");
      return RESULT_FAIL;
    }
    
    //
    //
    // TODO: "Smart" behavior selection based on robot / world / emotional state
    //
    //

    /*
    // For now: random behavior that is runnable
    const s32 N = _rng.RandIntInRange(0,static_cast<s32>(_behaviors.size())-1);
    _nextBehavior = std::next(std::begin(_behaviors), N);
    */
    // Always select OCD for testing (switches to Fidget if OCD isn't runnable yet)
    _nextBehavior = _behaviors.find("OCD");
    
    // If the randomly-selected behavior isn't runnable, just select the next one that is
    if(_nextBehavior->second->IsRunnable(currentTime_sec) == false) {
      _nextBehavior = _behaviors.begin();
      while(_nextBehavior->second->IsRunnable(currentTime_sec) == false) {
        ++_nextBehavior;
        if(_nextBehavior == _behaviors.end()) {
          PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NoneRunnable", "");
          return RESULT_FAIL;
        }
      }
    }
    
    // Initialize the selected behavior
    return InitNextBehaviorHelper(currentTime_sec);
    
  } // SelectNextBehavior()
  
  Result BehaviorManager::SelectNextBehavior(const std::string& name, float currentTime_sec)
  {
    _nextBehavior = _behaviors.find(name);
    if(_nextBehavior == _behaviors.end()) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.UnknownName",
                        "No behavior named '%s'", name.c_str());
      return RESULT_FAIL;
    }
    else if(_nextBehavior->second->IsRunnable(currentTime_sec) == false) {
      PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NotRunnable",
                        "Behavior '%s' is not runnable.", name.c_str());
      return RESULT_FAIL;
    }
    
    return InitNextBehaviorHelper(currentTime_sec);
  }
  
  
  const IBehavior* BehaviorManager::GetCurrentBehavior() const
  {
    if(_currentBehavior != _behaviors.end()) {
      return _currentBehavior->second;
    } else {
      return nullptr;
    }
  }
  
} // namespace Cozmo
} // namespace Anki
