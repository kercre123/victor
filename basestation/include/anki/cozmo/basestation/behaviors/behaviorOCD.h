/**
 * File: behaviorOCD.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines Cozmo's "OCD" behavior, which likes to keep blocks neat n' tidy.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef COZMO_BEHAVIOR_OCD_H
#define COZMO_BEHAVIOR_OCD_H

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

// TODO: This needs to be switched to some kind of _internal_ signals definition
#include "messageEngineToGame.h"

#include <set>

namespace Anki {
namespace Cozmo {
  
  // A behavior that tries to neaten up blocks present in the world
  class BehaviorOCD : public IBehavior
  {
  public:
    
    BehaviorOCD(Robot& robot, const Json::Value& config);
    virtual ~BehaviorOCD() { }
    
    virtual bool IsRunnable() const override;

    virtual Result Init() override;
    
    virtual Status Update() override;
    
    virtual bool GetRewardBid(Reward& reward) override;
    
  private:
    
    // Handlers for signals coming from the engine
    // TODO: These need to be some kind of _internal_ signal or event
    void HandleObjectMoved(const ExternalInterface::ActiveObjectMoved &msg);
    void HandleObservedObject(const ExternalInterface::RobotObservedObject &msg);
    void HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg);
    
    void HandleActionCompleted(const ExternalInterface::RobotCompletedAction &msg);
    
    Result SelectArrangement();
    Result SelectNextObjectToPickUp();
    Result SelectNextPlacement();
    
    f32 GetNeatnessScore(ObjectID whichObject);
    
    // Goal is to move objects from messy to neat
    std::set<ObjectID> _messyObjects;
    std::set<ObjectID> _neatObjects;
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    enum class State {
      PICKING_UP_BLOCK,
      PLACING_BLOCK
    };
    
    State _currentState;
    
    // Enumerate possible arrangements Cozmo "likes".
    enum class Arrangement {
      STACKS = 0,
      LINE,
      NUM_ARRANGEMENTS
    };
    
    Arrangement _currentArrangement;
    
    ObjectID _objectToPickUp;
    
    // A placement can be on the ground or on another object:
    struct NextPlacement {
      NextPlacement() {}
      ~NextPlacement() {}
      bool placeOnObject;
      union {
        Pose3d   atPose;
        ObjectID whichObject;
      };
    };
    
    NextPlacement _nextPlacement;
    
  }; // class BehaviorOCD

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_OCD_H