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

#include "util/signals/simpleSignal_fwd.h"

#include "messageEngineToGame.h"

#include <set>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  template<typename TYPE> class AnkiEvent;
  
  // A behavior that tries to neaten up blocks present in the world
  class BehaviorOCD : public IBehavior
  {
  public:
    
    BehaviorOCD(Robot& robot, const Json::Value& config);
    virtual ~BehaviorOCD() { }
    
    virtual bool IsRunnable() const override;

    virtual Result Init() override;
    
    virtual Status Update(float currentTime_sec) override;
    
    // Finish placing current object if there is one, otherwise good to go
    virtual Result Interrupt() override;
    
    virtual bool GetRewardBid(Reward& reward) override;
    
    virtual const std::string& GetName() const override {
      //static std::string name("OCD");
      return _name;
    }
    
  private:
    
    // Dispatch handlers based on tag of passed-in event
    void EventHandler(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    
    // Handlers for signals coming from the engine
    // TODO: These need to be some kind of _internal_ signal or event
    Result HandleObjectMoved(const ExternalInterface::ActiveObjectMoved &msg);
    Result HandleObservedObject(const ExternalInterface::RobotObservedObject &msg);
    Result HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg);
    
    Result HandleActionCompleted(const ExternalInterface::RobotCompletedAction &msg);
    
    Result SelectArrangement();
    Result SelectNextObjectToPickUp();
    Result SelectNextPlacement();
    Result FindEmptyPlacementPose(const ObjectID& nearObjectID, Pose3d& pose);
    
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
    bool  _interrupted;
    
    Result _lastHandlerResult;
    std::vector<::Signal::SmartHandle> _eventHandles;
    
    // Enumerate possible arrangements Cozmo "likes".
    enum class Arrangement {
      STACKS_OF_TWO = 0,
      LINE,
      NUM_ARRANGEMENTS
    };
    
    Arrangement _currentArrangement;
    
    ObjectID _objectToPickUp;
    ObjectID _objectToPlaceOn;
    ObjectID _lastObjectPlacedOnGround;
    ObjectID _anchorObject; // the object the arrangement is anchored to
    
    void UpdateName();
    std::string _name;
    
    /*
     Not sure this is needed anymore, now that we're using events and
     the current action should basically hold this state 
     
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
    */
  }; // class BehaviorOCD

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_OCD_H