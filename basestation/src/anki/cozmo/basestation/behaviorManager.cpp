
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

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
  
  BehaviorManager::BehaviorManager()
  : _currentBehavior(_behaviors.end())
  , _nextBehavior(_behaviors.end())
  {

  }
  
  BehaviorManager::~BehaviorManager()
  {
    for(auto behavior : _behaviors) {
      if(behavior.second == nullptr) {
        PRINT_NAMED_ERROR("BehaviorManager.Destructor.ExistingNull",
                          "Existing '%s' behavior is somehow NULL.\n",
                          behavior.first.c_str());
      } else {
        delete behavior.second;
      }
    }
  }
  
  Result BehaviorManager::AddBehavior(const std::string &name, IBehavior *newBehavior)
  {
    if(newBehavior == nullptr) {
      PRINT_NAMED_ERROR("BehaviorManager.AddBehavior.NullBehavior", "Refusing to add NULL behavior.\n");
      return RESULT_FAIL;
    }
    
    // Make sure to delete any existing behavior with the same name before replacing
    auto existingBehavior = _behaviors.find(name);
    if(existingBehavior != _behaviors.end()) {
      PRINT_NAMED_WARNING("BehaviorManager.AddBehavior.ReplaceExisting",
                          "Replacing existing '%s' behavior.\n", name.c_str());
      if(existingBehavior->second == nullptr) {
        PRINT_NAMED_ERROR("BehaviorManager.AddBehavior.ExistingNull",
                          "Existing '%s' behavior is somehow NULL.\n",
                          existingBehavior->first.c_str());
      } else {
        delete existingBehavior->second;
      }
    }
    
    _behaviors[name] = newBehavior;
    
    return RESULT_OK;
  }
  
  void BehaviorManager::SwitchFromCurrentToNext()
  {
    _currentBehavior = _nextBehavior;
    _nextBehavior = _behaviors.end();
  }
  
  Result BehaviorManager::Update()
  {
    Result lastResult = RESULT_OK;
    
    if(_currentBehavior != _behaviors.end()) {
      // We have a current behavior, update it.
      IBehavior::Status status = _currentBehavior->second->Update();
     
      switch(status)
      {
        case IBehavior::Status::RUNNING:
          // Nothing to do! Just keep on truckin'....
          break;
          
        case IBehavior::Status::COMPLETE:
          // Behavior complete, switch to next
          SwitchFromCurrentToNext();
          break;
          
        case IBehavior::Status::FAILURE:
          PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                            "Behavior '%s' failed to Update().\n",
                            _currentBehavior->first.c_str());
          lastResult = RESULT_FAIL;
          break;
          
        default:
          PRINT_NAMED_ERROR("BehaviorManager.Update.UnknownStatus",
                            "Behavior '%s' returned unknown status %d\n",
                            _currentBehavior->first.c_str(), status);
          lastResult = RESULT_FAIL;
      } // switch(status)
    }
    else if(_nextBehavior != _behaviors.end()) {
      // No current behavior, but next behavior defined, so switch to it.
      SwitchFromCurrentToNext();
    }
    
    return lastResult;
  } // Update()
  
  
  Result BehaviorManager::SelectNextBehavior()
  {
    
    if(_behaviors.empty()) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.NoBehaviors",
                        "Empty behavior container.\n");
      return RESULT_FAIL;
    }
    
    //
    //
    // TODO: "Smart" behavior selection based on robot / world / emotional state
    //
    //

    // For now: random behavior that is runnable
    std::uniform_int_distribution<> dist(0,_behaviors.size());
    const size_t maxAttempts = 2*_behaviors.size();
    size_t attempts = 0;
    do {
      _nextBehavior = std::next(std::begin(_behaviors), dist(_randomGenerator));
    } while(attempts++ < maxAttempts && _nextBehavior->second->IsRunnable() == false);
    
    if(attempts == maxAttempts) {
      _nextBehavior = _behaviors.end();
      
      PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NoneRunnable",
                        "Unable to find a runnable next behavior after %lu attempts.\n", attempts);
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  Result BehaviorManager::SelectNextBehavior(const std::string& name)
  {
    _nextBehavior = _behaviors.find(name);
    if(_nextBehavior == _behaviors.end()) {
      PRINT_NAMED_ERROR("BehaviorManager.SelectNextBehavior.UnknownName",
                        "No behavior named '%s'\n", name.c_str());
      return RESULT_FAIL;
    }
    else if(_nextBehavior->second->IsRunnable() == false) {
      PRINT_NAMED_ERROR("BehaviorManager.SelecteNextBehavior.NotRunnable",
                        "Behavior '%s' is not runnable.\n", name.c_str());
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  
} // namespace Cozmo
} // namespace Anki
