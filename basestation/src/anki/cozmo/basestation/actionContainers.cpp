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

#include "util/logging/logging.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark ---- ActionList ----
    
    ActionList::ActionList()
    {
      
    }
    
    ActionList::~ActionList()
    {
      Clear();
    }
    
    void ActionList::Cancel(SlotHandle fromSlot, RobotActionType withType)
    {
      // Clear specified slot / type
      for(auto & q : _queues) {
        if(fromSlot == -1 || q.first == fromSlot) {
          q.second.Cancel(withType);
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
        PRINT_STREAM_INFO("ActionList.Print", "ActionList is empty.\n");
      } else {
        PRINT_STREAM_INFO("ActionList.Print", "ActionList contains " << _queues.size() << " queues:\n");
        for(auto const& queuePair : _queues) {
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
    
    
    ActionList::SlotHandle ActionList::AddConcurrentAction(IActionRunner* action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_WARNING("ActionList.AddAction.NullActionPointer", "Refusing to add null action.\n");
        return -1;
      }
      
      // Find an empty slot
      SlotHandle currentSlot = 0;
      while(_queues.find(currentSlot) != _queues.end()) {
        ++currentSlot;
      }
      
      if(_queues[currentSlot].QueueAtEnd(action, numRetries) != RESULT_OK) {
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

    void ActionQueue::Cancel(RobotActionType withType)
    {
      for(auto action : _queue)
      {
        CORETECH_ASSERT(action != nullptr);
        
        if(withType == RobotActionType::UNKNOWN || action->GetType() == withType) {
          action->Cancel();
        }
      }
    }

    Result ActionQueue::QueueNow(IActionRunner *action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueNow.NullActionPointer",
                          "Refusing to queue a null action pointer.\n");
        return RESULT_FAIL;
      }
      
      if(_queue.empty()) {
        
        // Nothing in the queue, so this is the same as QueueAtEnd
        return QueueAtEnd(action, numRetries);
        
      } else {
        
        // Cancel whatever is running now and then queue this to happen next
        // (right after any cleanup due to the cancellation completes)
        _queue.front()->Cancel();
        return QueueNext(action, numRetries);
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
        return QueueAtEnd(action, numRetries);
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
        PRINT_STREAM_INFO("ActionQueue.Print", "ActionQueue is empty.\n");
      } else {
        std::stringstream ss;
        ss << "ActionQueue with " << _queue.size() << " actions: ";
        for(auto action : _queue) {
          ss << action->GetName() << ", ";
        }
        PRINT_STREAM_INFO("ActionQueue.Print", ss.str());
      }
      
    } // Print()
    
  } // namespace Cozmo
} // namespace Anki

