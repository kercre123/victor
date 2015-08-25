
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
#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"

#include "anki/cozmo/basestation/robot.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#define DEBUG_BEHAVIOR_MGR 0

namespace Anki {
namespace Cozmo {
  
  
  BehaviorManager::BehaviorManager(Robot& robot)
  : _isInitialized(false)
  , _forceReInit(false)
  , _robot(robot)
  , _behaviorChooser(new SimpleBehaviorChooser)
  , _minBehaviorTime_sec(5)
  {

  }
  
  Result BehaviorManager::Init(const Json::Value &config)
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
    
    // TODO: Set configuration data from Json...
    
    // TODO: Only load behaviors specified by Json?
    
    _behaviorChooser->AddBehavior(new BehaviorOCD(_robot, config));
    //AddBehavior(new BehaviorLookForFaces(_robot, config));
    _behaviorChooser->AddBehavior(new BehaviorLookAround(_robot, config));
    _behaviorChooser->AddBehavior(new BehaviorFidget(_robot, config));
    
    _isInitialized = true;
    
    _lastSwitchTime_sec = 0.f;
    
    return RESULT_OK;
  }
  
  BehaviorManager::~BehaviorManager()
  {
    Util::SafeDelete(_behaviorChooser);
  }
  
  void BehaviorManager::SwitchToNextBehavior()
  {
    _currentBehavior = _nextBehavior;
    _nextBehavior = nullptr;
  }
  
  Result BehaviorManager::Update(float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
      return RESULT_FAIL;
    }
    
    if(nullptr == _currentBehavior ||
       currentTime_sec - _lastSwitchTime_sec > _minBehaviorTime_sec)
    {
      // We've been in the current behavior long enough to consider switching
      lastResult = SelectNextBehavior(currentTime_sec);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorManager.Update.SelectNextFailed",
                            "Failed trying to select next behavior, continuing with current.");
        lastResult = RESULT_OK;
      }
      
      std::string nextName("none");
      if (nullptr != _nextBehavior)
      {
        nextName = _nextBehavior->GetName();
      }
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Update.SelectedNext",
                          "Selected next behavior '%s' at t=%.1f, last was t=%.1f",
                          nextName.c_str(), currentTime_sec, _lastSwitchTime_sec);
      
      _lastSwitchTime_sec = currentTime_sec;
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
          SwitchToNextBehavior();
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
      initResult = _nextBehavior->Init();
      if(initResult != RESULT_OK) {
        PRINT_NAMED_ERROR("BehaviorManager.InitNextBehaviorHelper.InitFailed",
                          "Failed to initialize %s behavior.",
                          _nextBehavior->GetName().c_str());
        _nextBehavior = nullptr;
      } else if(nullptr != _currentBehavior) {
        // We've successfully initialized the next behavior to run, so interrupt
        // the current behavior that's running if there is one. It will continue
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
  
  Result BehaviorManager::SelectNextBehavior(float currentTime_sec)
  {
    
    _nextBehavior = _behaviorChooser->ChooseNextBehavior(currentTime_sec);
    if(nullptr == _nextBehavior) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.NoneRunnable", "");
      return RESULT_FAIL;
    }
    
    // Initialize the selected behavior
    return InitNextBehaviorHelper(currentTime_sec);
    
  } // SelectNextBehavior()
  
  Result BehaviorManager::SelectNextBehavior(const std::string& name, float currentTime_sec)
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
