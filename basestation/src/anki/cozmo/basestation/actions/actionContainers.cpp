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

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"

#include "util/logging/logging.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark ---- ActionList ----
    
    ActionList::ActionList(Robot& robot)
    {
      _robot = &robot;
    }
    
    ActionList::~ActionList()
    {
      Clear();
    }
    
    Result ActionList::QueueAction(SlotHandle inSlot, QueueActionPosition inPosition,
                                   IActionRunner* action, u8 numRetries)
    {
      switch(inPosition)
      {
        case QueueActionPosition::NOW:
        {
          QueueActionNow(inSlot, action, numRetries);
          break;
        }
        case QueueActionPosition::NOW_AND_CLEAR_REMAINING:
        {
          // Cancel all queued actions and make this action the next thing in it
          Cancel();
          QueueActionNext(inSlot, action, numRetries);
          break;
        }
        case QueueActionPosition::NEXT:
        {
          QueueActionNext(inSlot, action, numRetries);
          break;
        }
        case QueueActionPosition::AT_END:
        {
          QueueActionAtEnd(inSlot, action, numRetries);
          break;
        }
        case QueueActionPosition::NOW_AND_RESUME:
        {
          QueueActionAtFront(inSlot, action, numRetries);
          break;
        }
        default:
        {
          PRINT_NAMED_ERROR("CozmoGameImpl.QueueActionHelper.InvalidPosition",
                            "Unrecognized 'position' %s for queuing action.",
                            EnumToString(inPosition));
          return RESULT_FAIL;
        }
      }
      
      return RESULT_OK;
    } // QueueAction()
    
    inline Result ActionList::QueueActionNext(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      action->SetRobot(*_robot);
      return _queues[atSlot].QueueNext(action, numRetries);
    }
    
    inline Result ActionList::QueueActionAtEnd(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      action->SetRobot(*_robot);
      return _queues[atSlot].QueueAtEnd(action, numRetries);
    }
    
    inline Result ActionList::QueueActionNow(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      action->SetRobot(*_robot);
      return _queues[atSlot].QueueNow(action, numRetries);
    }
    
    inline Result ActionList::QueueActionAtFront(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      action->SetRobot(*_robot);
      return _queues[atSlot].QueueAtFront(action, numRetries);
    }
    
    bool ActionList::Cancel(SlotHandle fromSlot, RobotActionType withType)
    {
      bool found = false;
      
      // Clear specified slot / type
      for(auto & q : _queues) {
        if(fromSlot == -1 || q.first == fromSlot) {
          found |= q.second.Cancel(withType);
        }
      }
      return found;
    }
    
    bool ActionList::Cancel(u32 idTag, SlotHandle fromSlot)
    {
      bool found = false;
      
      if(fromSlot == -1) {
        for(auto & q : _queues) {
          if(q.second.Cancel(idTag) == true) {
            if(found) {
              PRINT_NAMED_WARNING("ActionList.Cancel.DuplicateTags",
                                  "Multiple actions from multiple slots cancelled with idTag=%d.\n", idTag);
            }
            found = true;
          }
        }
        return found;
      } else {
        auto q = _queues.find(fromSlot);
        if(q != _queues.end()) {
          found = q->second.Cancel(idTag);
        } else {
          PRINT_NAMED_WARNING("ActionList.Cancel.NoSlot", "No slot with handle %d.\n", fromSlot);
        }
      }
      
      return found;
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
    
    Result ActionList::Update()
    {
      Result lastResult = RESULT_OK;
      
      for(auto queueIter = _queues.begin(); queueIter != _queues.end(); )
      {
        lastResult = queueIter->second.Update();
        
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
    
    bool ActionList::IsCurrAction(const std::string& actionName) const
    {
      for(auto queueIter = _queues.begin(); queueIter != _queues.end();  ++queueIter)
      {
        if (nullptr == queueIter->second.GetCurrentAction()) {
          return false;
        }
        if (queueIter->second.GetCurrentAction()->GetName() == actionName) {
          return true;
        }
      }
      return false;
    }

    bool ActionList::IsCurrAction(u32 idTag, SlotHandle fromSlot) const
    {
      const auto qIter = _queues.find(fromSlot);
      if( qIter == _queues.end() ) {
        // can't be playing if the slot doesn't exist
        return false;
      }

      if( nullptr == qIter->second.GetCurrentAction() ) {
        return false;
      }

      return qIter->second.GetCurrentAction()->GetTag() == idTag;
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

    bool ActionQueue::Cancel(RobotActionType withType)
    {
      bool found = false;
      for(auto action : _queue)
      {
        CORETECH_ASSERT(action != nullptr);
        
        if(withType == RobotActionType::UNKNOWN || action->GetType() == withType) {
          action->Cancel();
          found = true;
        }
      }
      return found;
    }
    
    bool ActionQueue::Cancel(u32 idTag)
    {
      bool found = false;
      for(auto action : _queue)
      {
        if(action->GetTag() == idTag) {
          if(found == true) {
            PRINT_NAMED_WARNING("ActionQueue.Cancel.DuplicateIdTags",
                                "Multiple actions with tag=%d found in queue.\n",
                                idTag);
          }
          action->Cancel();
          found = true;
        }
      }
      
      return found;
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
        PRINT_NAMED_DEBUG("ActionQueue.QueueNow.CancelingPrevious", "Canceling %s [%d] in favor of action %s [%d]",
                          _queue.front()->GetName().c_str(),
                          _queue.front()->GetTag(),
                          action->GetName().c_str(),
                          action->GetTag());
        _queue.front()->Cancel();
        return QueueNext(action, numRetries);
      }
    }
    
    Result ActionQueue::QueueAtFront(IActionRunner* action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueAFront.NullActionPointer",
                          "Refusing to queue a null action pointer.");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      if(_queue.empty()) {
        // Nothing in the queue, so this is the same as QueueAtEnd
        result = QueueAtEnd(action, numRetries);
      } else {
        // Try to interrupt whatever is running and put this new action in front of it
        if(_queue.front()->Interrupt()) {
          // Current front action is interruptible. Add it to the list of interrupted
          // actions to get updated once more on the next Update() call (when we'll
          // have a robot reference), and put the new action in front of it in the queue.
          PRINT_NAMED_INFO("ActionQueue.QueueAtFront.Interrupt",
                           "Interrupting %s to put %s in front of it.",
                           _queue.front()->GetName().c_str(),
                           action->GetName().c_str());
          _interruptedActions.push_back(_queue.front());
          action->SetNumRetries(numRetries);
          _queue.push_front(action);
        } else {
          // Current front action is not interruptible, so just use QueueNow and
          // cancel it
          PRINT_NAMED_INFO("ActionQueue.QueueAtFront.Interrupt",
                           "Could not interrupt %s. Will cancel and queue %s now.",
                           _queue.front()->GetName().c_str(),
                           action->GetName().c_str());
          result = QueueNow(action, numRetries);
        }
      }
      
      return result;
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
    
    Result ActionQueue::Update()
    {
      Result lastResult = RESULT_OK;
      
      // Update any interrupted actions (but leave them in the queue)
      for(auto interruptedAction : _interruptedActions) {
        ActionResult result = interruptedAction->Update();
        if(ActionResult::INTERRUPTED != result) {
          PRINT_NAMED_WARNING("ActionQueue.Update.InterruptFailed",
                              "Expecting interrupted %s action to return INTERRUPTED result on Update",
                              interruptedAction->GetName().c_str());
        }
      }
      _interruptedActions.clear();
      
      if(!_queue.empty())
      {
        IActionRunner* currentAction = GetCurrentAction();
        assert(currentAction != nullptr);
        
        VizManager::getInstance()->SetText(VizManager::ACTION, NamedColors::GREEN,
                                           "Action: %s", currentAction->GetName().c_str());
        
        const ActionResult actionResult = currentAction->Update();
        
        if(actionResult != ActionResult::RUNNING) {
          // Current action just finished, pop it
          PopCurrentAction();
          
          if(actionResult != ActionResult::SUCCESS && actionResult != ActionResult::CANCELLED) {
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

    const IActionRunner* ActionQueue::GetCurrentAction() const
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
//        std::stringstream ss;
//        ss << "ActionQueue with " << _queue.size() << " actions: ";
        PRINT_STREAM_INFO("ActionQueue.Print", "ActionQueue contains " << _queue.size() << " actions:\n");
        for(auto action : _queue) {
//          ss << action->GetName() << ", ";
          PRINT_STREAM_INFO("ActionList.Print", action->GetName() << ", ");
        }
//        PRINT_STREAM_INFO("ActionQueue.Print", ss.str());
      }
      
    } // Print()
    
  } // namespace Cozmo
} // namespace Anki

