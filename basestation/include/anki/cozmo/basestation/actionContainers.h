/**
 * File: actionContainers.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Defines containers for running actions, both as a queue and a 
 *              concurrent list.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ACTION_CONTAINERS_H
#define ANKI_COZMO_ACTION_CONTAINERS_H

#include "anki/common/types.h"

#include "clad/types/actionTypes.h"

#include <list>
#include <map>

// TODO: Is this Cozmo-specific or can it be moved to coretech?
// (Note it does require a Robot, which is currently only present in Cozmo)

namespace Anki {
  
  namespace Cozmo {

    // Forward declarations:
    class Robot;
    class IActionRunner;
    
    // This is an ordered list of actions to be run. It is similar to an
    // CompoundActionSequential, but actions can be added to it dynamically,
    // either "next" or at the end of the queue. As actions are completed,
    // they are popped off the queue. Thus, when it is empty, it is "done".
    class ActionQueue
    {
    public:
      ActionQueue();
      
      ~ActionQueue();
      
      Result   Update(Robot& robot);
      
      Result   QueueNext(IActionRunner  *action,  u8 numRetries = 0);
      Result   QueueAtEnd(IActionRunner *action, u8 numRetires = 0);
      Result   QueueNow(IActionRunner   *action,   u8 numRetries = 0);
      
      // Blindly clear the queue
      void     Clear();
      
      bool     Cancel(RobotActionType withType = RobotActionType::UNKNOWN);
      bool     Cancel(u32 idTag);
      
      bool     IsEmpty() const { return _queue.empty(); }
      
      size_t   Length() const { return _queue.size(); }
      
      IActionRunner* GetCurrentAction();
      void           PopCurrentAction();
      
      void Print() const;
      
    protected:
      std::list<IActionRunner*> _queue;
      
    }; // class ActionQueue
    
    
    // This is a list of concurrent actions to be run, addressable by ID handle.
    // Each slot in the list is really a queue, to which new actions can be added
    // using that slot's ID handle. When a slot finishes, it is popped.
    class ActionList
    {
    public:
      using SlotHandle = s32;
      
      ActionList();
      ~ActionList();
      
      // Updates the current action of each queue in each slot
      Result     Update(Robot& robot);
      
      // Add a new action to be run concurrently, generating a new slot, whose
      // handle is returned. If there is no desire to queue anything to run after
      // this action, the returned SlotHandle can be ignored.
      SlotHandle AddConcurrentAction(IActionRunner* action, u8 numRetries = 0);
      
      // Queue an action into a specific slot. If that slot does not exist
      // (perhaps because it completed before this call) it will be created.
      Result     QueueActionNext(SlotHandle  atSlot, IActionRunner* action, u8 numRetries = 0);
      Result     QueueActionAtEnd(SlotHandle atSlot, IActionRunner* action, u8 numRetries = 0);
      
      // Start doing the given action *now* -- cancels any currently-running action.
      Result     QueueActionNow(SlotHandle atSlot, IActionRunner* action, u8 numRetries = 0);
      
      bool       IsEmpty() const;
      
      size_t     GetQueueLength(SlotHandle atSlot);

      // Blindly clears out the contents of the action list
      void       Clear();

      // Only cancels actions from the specified slot with the specified type, and
      // does any cleanup specified by the action's Cancel/Cleanup methods.
      // Returns true if any actions were cancelled.
      bool       Cancel(SlotHandle fromSlot = -1, // -1 == "all slots"
                        RobotActionType withType = RobotActionType::UNKNOWN);
      
      // Find and cancel the action with the specified ID Tag. The slot to search in
      // can optionally be specified, but otherwise, all slots are searched.
      // Returns true if the action was found and cancelled.
      bool       Cancel(u32 idTag, SlotHandle fromSlot = -1);
      
      void       Print() const;
      
      // Returns true if actionName is the name of one of the actions that are currently
      // being executed.
      bool       IsCurrAction(const std::string& actionName);

      
    protected:
      
      std::map<SlotHandle, ActionQueue> _queues;
      
    }; // class ActionList
    
    
    inline Result ActionList::QueueActionNext(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      return _queues[atSlot].QueueNext(action, numRetries);
    }
    
    inline Result ActionList::QueueActionAtEnd(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      return _queues[atSlot].QueueAtEnd(action, numRetries);
    }
    
    inline Result ActionList::QueueActionNow(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      return _queues[atSlot].QueueNow(action, numRetries);
    }
    
    inline size_t ActionList::GetQueueLength(SlotHandle atSlot)
    {
      return _queues[atSlot].Length();
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTION_CONTAINERS_H
