/**
 * File: compoundActions.cpp
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Implements compound actions, which are groups of IActions to be
 *              run together in series or in parallel.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actions/compoundActions.h"

#include "anki/common/basestation/utils/timer.h"
#include "util/helpers/templateHelpers.h"


namespace Anki {
  namespace Cozmo {
    
#pragma mark ---- ICompoundAction ----
    
    ICompoundAction::ICompoundAction(std::initializer_list<IActionRunner*> actions)
    {
      for(IActionRunner* action : actions) {
        if(action == nullptr) {
          PRINT_NAMED_WARNING("ICompoundAction.NullActionPointer",
                              "Refusing to add a null action pointer to group.\n");
        } else {
          AddAction(action);
        }
      }
    }
    
    ICompoundAction::~ICompoundAction()
    {
      DeleteActions();
    }
    
    void ICompoundAction::Reset()
    {
      for(auto & actionPair : _actions) {
        actionPair.first = false;
        actionPair.second->Reset();
      }
    }
    
    void ICompoundAction::AddAction(IActionRunner* action)
    {
      if(_actions.empty()) {
        _name = "["; // initialize with opening bracket for first action
      } else {
        _name.pop_back(); // remove last char ']'
        _name += "+";
      }
      
      // All added actions have the same message display setting as the parent
      // compound action in which they are included
      action->EnableMessageDisplay(IsMessageDisplayEnabled());
      
      // As part of a compound action this should not emit completion
      action->SetEmitCompletionSignal(false);
      
      // An action can be added to a compound action after it has been queued in which case
      // this is where its robot pointer gets set
      action->SetRobot(*GetRobot());
      
      _actions.emplace_back(false, action);
      _name += action->GetName();
      _name += "]";
    }
    
    void ICompoundAction::ClearActions()
    {
      DeleteActions();
      _actions.clear();
      Reset();
    }
    
    void ICompoundAction::SetRobot(Robot& robot)
    {
      _robot = &robot;
      for(auto actionPair : _actions)
      {
        actionPair.second->SetRobot(robot);
      }
    }
    
    void ICompoundAction::DeleteActions()
    {
      for(auto iter = _actions.begin(); iter != _actions.end();)
      {
          assert((*iter).second != nullptr);
          // TODO: issue a warning when a group is deleted without all its actions completed?
          // Ensure all actions have valid robot pointers since it may be possible they haven't been set
          (*iter).second->SetRobot(*GetRobot());
          Util::SafeDelete((*iter).second);
          iter = _actions.erase(iter);
      }
    }
    
#pragma mark ---- CompoundActionSequential ----
    
    CompoundActionSequential::CompoundActionSequential()
    : CompoundActionSequential({})
    {
      
    }
    
    CompoundActionSequential::CompoundActionSequential(std::initializer_list<IActionRunner*> actions)
    : ICompoundAction(actions)
    , _delayBetweenActionsInSeconds(0)
    {
      Reset();
    }
    
    void CompoundActionSequential::Reset()
    {
      ICompoundAction::Reset();
      _waitUntilTime = -1.f;
      _currentActionPair = _actions.begin();
      _wasJustReset = true;
    }
    
    ActionResult CompoundActionSequential::UpdateInternal()
    {
      SetStatus(GetName());
      
      if(_wasJustReset) {
        // In case actions were added after construction/reset
        _currentActionPair = _actions.begin();
        _wasJustReset = false;
      }
      
      if(_currentActionPair != _actions.end())
      {
        bool& isDone = _currentActionPair->first;
        assert(isDone == false);
        
        IActionRunner* currentAction = _currentActionPair->second;
        assert(currentAction != nullptr); // should not have been allowed in by constructor
        currentAction->SetRobot(*GetRobot());
        
        double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if(_waitUntilTime < 0.f || currentTime >= _waitUntilTime)
        {
          ActionResult subResult = currentAction->Update();
          SetStatus(currentAction->GetStatus());
          switch(subResult)
          {
            case ActionResult::SUCCESS:
            {
              // Finished the current action, move ahead to the next
              isDone = true; // mark as done (not strictly necessary)
              
              if(_delayBetweenActionsInSeconds > 0.f) {
                // If there's a delay specified, figure out how long we need to
                // wait from now to start next action
                _waitUntilTime = currentTime + _delayBetweenActionsInSeconds;
              }
              
              _currentActionPair->second->UnlockTracks();
              ++_currentActionPair;
              
              // if that was the last action, we're done
              if(_currentActionPair == _actions.end()) {
#               if USE_ACTION_CALLBACKS
                RunCallbacks(ActionResult::SUCCESS);
#               endif
                return ActionResult::SUCCESS;
              } else if(currentTime >= _waitUntilTime) {
                PRINT_NAMED_INFO("CompoundActionSequential.Update.NextAction",
                                 "Moving to action %s", _currentActionPair->second->GetName().c_str());
                
                // Otherwise, we are still running. Go ahead and immediately do an
                // update on the next action now to get its initialization and
                // precondition checking going, to reduce lag between actions.
                _currentActionPair->second->SetRobot(*GetRobot());
                subResult = _currentActionPair->second->Update();
                
                // In the special case that the sub-action sucessfully completed
                // immediately, don't return SUCCESS if there are more actions left!
                if(ActionResult::SUCCESS == subResult) {
                
                  _currentActionPair->second->UnlockTracks();
                  ++_currentActionPair;
                  
                  if(_currentActionPair == _actions.end()) {
                    // no more actions, safe to return success for the compound action
#                   if USE_ACTION_CALLBACKS
                    RunCallbacks(ActionResult::SUCCESS);
#                   endif
                    return ActionResult::SUCCESS;
                  } else {
                    // more actions, just say we're still running
                    subResult = ActionResult::RUNNING;
                  }
                }
              }
              else {
                // this sub-action finished, but we still have others that we are waiting to run, probably due
                // to delay between actions, so return running
                return ActionResult::RUNNING;
              }
              
              return subResult;
            }
              
            case ActionResult::FAILURE_RETRY:
              // A constituent action failed . Reset all the constituent actions
              // and try again as long as there are retries remaining
              if(RetriesRemain()) {
                PRINT_NAMED_INFO("CompoundActionSequential.Update.Retrying",
                                 "%s triggered retry.\n", currentAction->GetName().c_str());
                Reset();
                return ActionResult::RUNNING;
              }
              // No retries remaining. Fall through:
              
            case ActionResult::RUNNING:
            case ActionResult::FAILURE_ABORT:
            case ActionResult::FAILURE_TIMEOUT:
            case ActionResult::FAILURE_PROCEED:
            case ActionResult::FAILURE_TRACKS_LOCKED:
            case ActionResult::CANCELLED:
            case ActionResult::INTERRUPTED:
#             if USE_ACTION_CALLBACKS
              RunCallbacks(subResult);
#             endif
              return subResult;
              
          } // switch(result)
        } else {
          return ActionResult::RUNNING;
        } // if/else waitUntilTime
      } // if currentAction != actions.end()
      
      // Shouldn't normally get here, but this means we've completed everything
      // and are done
      return ActionResult::SUCCESS;
      
    } // CompoundActionSequential::Update()

    
#pragma mark ---- CompoundActionParallel ----
    
    CompoundActionParallel::CompoundActionParallel()
    : CompoundActionParallel({})
    {
      
    }
    
    CompoundActionParallel::CompoundActionParallel(std::initializer_list<IActionRunner*> actions)
    : ICompoundAction(actions)
    {
      
    }
    
    ActionResult CompoundActionParallel::UpdateInternal()
    {
      // Return success unless we encounter anything still running or failed in loop below.
      // Note that we will return SUCCESS on the call following the one where the
      // last action actually finishes.
      ActionResult result = ActionResult::SUCCESS;
      
      SetStatus(GetName());
      
      for(auto & currentActionPair : _actions)
      {
        bool& isDone = currentActionPair.first;
        if(!isDone) {
          IActionRunner* currentAction = currentActionPair.second;
          assert(currentAction != nullptr); // should not have been allowed in by constructor

          currentAction->SetRobot(*GetRobot());
          const ActionResult subResult = currentAction->Update();
          SetStatus(currentAction->GetStatus());
          switch(subResult)
          {
            case ActionResult::SUCCESS:
              // Just finished this action, mark it as done and unlock its tracks
              isDone = true;
              currentAction->UnlockTracks();
              break;
              
            case ActionResult::RUNNING:
              // If any action is still running the group is still running
              result = ActionResult::RUNNING;
              break;
              
            case ActionResult::FAILURE_RETRY:
              // If any retries are left, reset the group and try again.
              if(RetriesRemain()) {
                PRINT_NAMED_INFO("CompoundActionParallel.Update.Retrying",
                                 "%s triggered retry.\n", currentAction->GetName().c_str());
                Reset();
                return ActionResult::RUNNING;
              }
              
              // If not, just fall through to other failure handlers:
            case ActionResult::FAILURE_ABORT:
            case ActionResult::FAILURE_PROCEED:
            case ActionResult::FAILURE_TIMEOUT:
            case ActionResult::FAILURE_TRACKS_LOCKED:
              // Return failure, aborting updating remaining actions the group
#             if USE_ACTION_CALLBACKS
              RunCallbacks(subResult);
#             endif
              return subResult;
              
            default:
              PRINT_NAMED_ERROR("CompoundActionParallel.Update.UnknownResultCase", "\n");
              assert(false);
              return ActionResult::FAILURE_ABORT;
              
          } // switch(subResult)
          
        } // if(!isDone)
      } // for each action in the group
      
#     if USE_ACTION_CALLBACKS
      if(result != ActionResult::RUNNING) {
        RunCallbacks(result);
      }
#     endif
      
      return result;
    } // CompoundActionParallel::Update()
    
  } // namespace Cozmo
} // namespace Anki
