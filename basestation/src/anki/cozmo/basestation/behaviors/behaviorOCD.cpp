/**
 * File: behaviorOCD.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "OCD" behavior, which likes to keep blocks neat n' tidy.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"

namespace Anki {
namespace Cozmo {
  
  BehaviorOCD::BehaviorOCD(Robot& robot)
  : IBehavior(robot)
  , _currentArrangement(Arrangement::STACKS)
  {
    
  }
  
  
  bool BehaviorOCD::IsRunnable() const
  {
    // We can only run this behavior when there are "messy" blocks present
    return !_messyObjects.empty();
  }
  
  Result BehaviorOCD::Init()
  {
    Result lastResult = RESULT_OK;
    
    if(_robot.IsCarryingObject()) {
      // Make sure whatever the robot is carrying is some kind of block...
      // (note that we have an actively-updated list of blocks, so we can just
      //  check against that)
      ObjectID carriedObjectID = _robot.GetCarryingObject();
      
      const bool carriedObjectIsBlock = _messyObjects.count(carriedObjectID) > 0;
      
      if(carriedObjectIsBlock) {
        // ... if so, start in PLACING_BLOCK mode
        _currentState = State::PLACING_BLOCK;
        lastResult = SelectNextPlacement();
      } else {
        // ... if not, put this thing down and start in PICKING_UP_BLOCK mode
        _currentState = State::PICKING_UP_BLOCK;
        
        // TODO: Find a good place to put down this object
        // For now, just put it down right _here_
        lastResult = _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlaceObjectOnGroundAction());
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("BehaviorOCD.Init.PlacementFailed",
                            "Failed to queue PlaceObjectOnGroundAction.\n");
          return lastResult;
        }
        
        lastResult = SelectNextObjectToPickUp();
      }
    } else {
      lastResult = SelectNextObjectToPickUp();
    }
    
    lastResult = SelectArrangement();
    
    return lastResult;
  } // Init()

  Result BehaviorOCD::SelectArrangement()
  {
    // TODO: Make this based on something "smarter"
    // For now, just rotate between available arrangments
    s32 iArrangement = static_cast<s32>(_currentArrangement);
    ++iArrangement;
    if(iArrangement == static_cast<s32>(Arrangement::NUM_ARRANGEMENTS)) {
      iArrangement = 0;
    }
    _currentArrangement = static_cast<Arrangement>(iArrangement);
    
    return RESULT_OK;
  }

  IBehavior::Status BehaviorOCD::Update()
  {
    // Completion trigger is when all (?) blocks make it to his "neat" list
    if(_messyObjects.empty()) {
      return Status::COMPLETE;
    }
    
    switch(_currentState)
    {
      case State::PICKING_UP_BLOCK:
        
        break;
        
      case State::PLACING_BLOCK:
        
        break;
        
      default:
        PRINT_NAMED_ERROR("OCD_Behavior.Update.UnknownState",
                          "Reached unknown state %d.\n", _currentState);
        return Status::FAILURE;
    }
    
    return Status::RUNNING;
  }
  
  void BehaviorOCD::HandleActionCompleted(const ExternalInterface::RobotCompletedAction &msg)
  {
    switch(_currentState)
    {
      case State::PICKING_UP_BLOCK:
      {
        switch(msg.actionType) {
            
          case RobotActionType::PICKUP_OBJECT_HIGH:
          case RobotActionType::PICKUP_OBJECT_LOW:
            // We're done picking up the block, figure out where to put it
            SelectNextPlacement();
            _currentState = State::PLACING_BLOCK;
            break;
            
          case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
            // We failed to pick up or place the last block, try again?
            SelectNextObjectToPickUp();
            break;
            
          default:
            // Simply ignore other action completions?
            break;

        } // switch(actionType)
        break;
      } // case PICKING_UP_BLOCK
        
      case State::PLACING_BLOCK:
      {
        switch(msg.actionType) {
          case RobotActionType::PLACE_OBJECT_LOW:
          case RobotActionType::PLACE_OBJECT_HIGH:
            // We're done placing the block, mark it as neat and move to next one
            _messyObjects.erase(_objectToPickUp);
            _neatObjects.insert(_objectToPickUp);
            
            SelectNextObjectToPickUp();
            _currentState = State::PICKING_UP_BLOCK;
            break;
            
          case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
            // We failed to place the last block, try again?
            SelectNextPlacement();
            break;
            
          default:
            // Simply ignore other action completions?
            break;
        }
        break;
      } // case PLACING_BLOCK
    } // switch(_currentState)
    
  } // HandleActionCompleted()
  
  
  void BehaviorOCD::HandleObjectMoved(const ExternalInterface::ActiveObjectMoved &msg)
  {
    // If a previously-neat object is moved, move it to the messy list
    // and play some kind of irritated animation
    ObjectID objectID;
    objectID = msg.objectID;
    
    if(_neatObjects.count(objectID)>0)
    {
      _neatObjects.erase(objectID);
      _messyObjects.insert(objectID);
      
      // TODO: Should this be "now" or "next"?
      _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlayAnimationAction("Irritated"));
    }
  }
  
  
  void BehaviorOCD::HandleObservedObject(const ExternalInterface::RobotObservedObject &msg)
  {
    // if the object is a BLOCK or ACTIVE_BLOCK, add its ID to the list we care about
    // iff we haven't already seen and neatened it (if it's in our neat list,
    // we won't take it out unless we detect that it was moved)
    ObjectID objectID;
    objectID = msg.objectID;

    if(_neatObjects.count(objectID) == 0) {
      _messyObjects.insert(objectID);
    }
  }
  
  void BehaviorOCD::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;

    _messyObjects.erase(objectID);
    _neatObjects.erase(objectID);
  }
  
  bool BehaviorOCD::GetRewardBid(Reward& reward)
  {
    
    // TODO: Populate a reward object based on how many poorly-arranged blocks there are
    
    return true;
  } // GetReward()

} // namespace Cozmo
} // namespace Anki