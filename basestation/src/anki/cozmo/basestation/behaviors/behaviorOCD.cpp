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

#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
  IBehavior::Status OCD_Behavior::Init(Robot& robot)
  {
    
    // Register to listen for "block" observed signals/messages instead of
    // polling the robot on every call to GetReward()?
    
    return Status::INITIALIZED;
  }
  
  IBehavior::Status OCD_Behavior::Update(Robot& robot)
  {
    switch(_currentState)
    {
        
        // TODO: fill in state machine!!
        
      default:
        PRINT_NAMED_ERROR("OCD_Behavior.Update.UnknownState",
                          "Reached unknown state %d.\n", _currentState);
        return Status::FAILURE;
    }
    
    return Status::RUNNING;
  }
  
  void OCD_Behavior::HandleObservedObject(RobotObservedObject &msg)
  {
    // if the object is a BLOCK or ACTIVE_BLOCK, add its ID to the list we care about
    _objectsOfInterest.insert(msg.objectID);
  }
  
  void OCD_Behavior::HandleDeletedObject(RobotDeletedObject &msg)
  {
    // remove the object if we knew about it
    _objectsOfInterest.erase(msg.objectID);
  }
  
  bool OCD_Behavior::GetRewardBid(Robot& robot, Reward& reward)
  {
    const BlockWorld& blockWorld = robot.GetBlockWorld();
    
    //const EmotionMgr emo = robot.GetEmotionMgr();
    
    const BlockWorld::ObjectsMapByType_t& blocks = blockWorld.GetExistingObjectsByFamily(BlockWorld::ObjectFamily::BLOCKS);
    
    const BlockWorld::ObjectsMapByType_t& lightCubes = blockWorld.GetExistingObjectsByFamily(BlockWorld::ObjectFamily::ACTIVE_BLOCKS);
    
    if(blocks.empty() && lightCubes.empty()) {
      // If there are no blocks to get OCD about, this isn't a viable behavior
      return false;
    }
    
    // TODO: Populate a reward object based on how many poorly-arranged blocks there are
    
    return true;
  } // GetReward()

} // namespace Cozmo
} // namespace Anki