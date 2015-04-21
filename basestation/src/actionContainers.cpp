/**
 * File: actionContainers.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements containers for running actions, both as a queue and a
 *              concurrent list.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actionContainers.h"
#include "anki/cozmo/basestation/actionInterface.h"

#include "anki/common/basestation/utils/logging/logging.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark ---- ActionList ----
    
    ActionList::ActionList()
    : _slotCounter(0)
    {
      
    }
    
    ActionList::~ActionList()
    {
      Clear();
    }
    
    void ActionList::Cancel(Robot& robot, SlotHandle fromSlot, RobotActionType withType)
    {
      // Clear specified slot / type
      for(auto & q : _queues) {
        if(fromSlot == -1 || q.first == fromSlot) {
          q.second.Cancel(robot, withType);
        }
      }
    }
    
    void ActionList::Clear()
    {
      _queues.clear();
    }
    
    bool ActionList::IsEmpty() const
    {
      return _queues.empty();
    }
    
    void ActionList::Print() const
    {
      if(IsEmpty()) {
        PRINT_INFO("ActionList is empty.\n");
      } else {
        PRINT_INFO("ActionList contains %d queues:\n", _queues.size());
        for(auto const& queuePair : _queues) {
          PRINT_INFO("---");
          queuePair.second.Print();
        }
      }
      
    } // Print()
    
    Result ActionList::Update(Robot& robot)
    {
      Result lastResult = RESULT_OK;
      
      for(auto queueIter = _queues.begin(); queueIter != _queues.end(); )
      {
        lastResult = queueIter->second.Update(robot);
        
        // If the queue is complete, remove it
        if(queueIter->second.IsEmpty()) {
          queueIter = _queues.erase(queueIter);
        } else {
          ++queueIter;
        }
      } // for each actionMemberPair
      
      return lastResult;
    } // Update()
    
    
    ActionList::SlotHandle ActionList::AddAction(IActionRunner* action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_WARNING("ActionList.AddAction.NullActionPointer", "Refusing to add null action.\n");
        return _slotCounter;
      }
      
      SlotHandle currentSlot = _slotCounter;
      
      if(_queues[currentSlot].QueueAtEnd(action, numRetries) == RESULT_OK) {
        _slotCounter++;
      } else {
        PRINT_NAMED_ERROR("ActionList.AddAction.FailedToAdd", "Failed to add action to new queue.\n");
      }
      
      return currentSlot;
    }
    
    bool ActionList::IsCurrAction(const std::string& actionName)
    {
      for(auto queueIter = _queues.begin(); queueIter != _queues.end();  ++queueIter)
      {
        if (queueIter->second.GetCurrentAction()->GetName() == actionName) {
          return true;
        }
      }
      return false;
    }
    
#pragma mark ---- ActionQueue ----
    
    ActionQueue::ActionQueue()
    {
      
    }
    
    ActionQueue::~ActionQueue()
    {
      Clear();
    }
    
    void ActionQueue::Clear()
    {
      while(!_queue.empty()) {
        IActionRunner* action = _queue.front();
        CORETECH_ASSERT(action != nullptr);
        delete action;
        _queue.pop_front();
      }
    }

    void ActionQueue::Cancel(Robot &robot, RobotActionType withType)
    {
      for(auto action : _queue)
      {
        CORETECH_ASSERT(action != nullptr);
        
        if(withType == RobotActionType::ACTION_UNKNOWN || action->GetType() == withType) {
          action->Cancel(robot);
        }
      }
    }

    
    Result ActionQueue::QueueAtEnd(IActionRunner *action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueAtEnd.NullActionPointer",
                          "Refusing to queue a null action pointer.\n");
        return RESULT_FAIL;
      }
      
      action->SetNumRetries(numRetries);
      _queue.push_back(action);
      return RESULT_OK;
    }
    
    Result ActionQueue::QueueNext(IActionRunner *action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueNext.NullActionPointer",
                          "Refusing to queue a null action pointer.\n");
        return RESULT_FAIL;
      }
      
      action->SetNumRetries(numRetries);
      
      if(_queue.empty()) {
        return QueueAtEnd(action);
      }
      
      std::list<IActionRunner*>::iterator queueIter = _queue.begin();
      ++queueIter;
      _queue.insert(queueIter, action);
      
      return RESULT_OK;
    }
    
    Result ActionQueue::Update(Robot& robot)
    {
      Result lastResult = RESULT_OK;
      
      if(!_queue.empty())
      {
        IActionRunner* currentAction = GetCurrentAction();
        assert(currentAction != nullptr);
        
        VizManager::getInstance()->SetText(VizManager::ACTION, NamedColors::GREEN,
                                           "Action: %s", currentAction->GetName().c_str());
        
        const ActionResult actionResult = currentAction->Update(robot);
        
        if(actionResult != ActionResult::RUNNING) {
          // Current action just finished, pop it
          PopCurrentAction();
          
          if(actionResult != ActionResult::SUCCESS) {
            lastResult = RESULT_FAIL;
          }
          
          VizManager::getInstance()->SetText(VizManager::ACTION, NamedColors::GREEN, "");
        }
      } // if queue not empty
      
      return lastResult;
    }
    
    IActionRunner* ActionQueue::GetCurrentAction()
    {
      if(_queue.empty()) {
        return nullptr;
      }
      
      return _queue.front();
    }
    
    void ActionQueue::PopCurrentAction()
    {
      if(!IsEmpty()) {
        if(_queue.front() == nullptr) {
          PRINT_NAMED_ERROR("ActionQueue.PopCurrentAction.NullActionPointer",
                            "About to delete and pop action pointer from queue, found it to be nullptr!\n");
        } else {
          delete _queue.front();
        }
        _queue.pop_front();
      }
    }
    
    void ActionQueue::Print() const
    {
      
      if(IsEmpty()) {
        PRINT_INFO("ActionQueue is empty.\n");
      } else {
        PRINT_INFO("ActionQueue with %d actions: ", _queue.size());
        for(auto action : _queue) {
          PRINT_INFO("%s, ", action->GetName().c_str());
        }
        PRINT_INFO("\b\b\n");
      }
      
    } // Print()
    
  } // namespace Cozmo
} // namespace Anki

