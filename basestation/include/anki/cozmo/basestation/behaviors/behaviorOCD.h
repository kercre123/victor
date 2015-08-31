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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorOCD_H__
#define __Cozmo_Basestation_Behaviors_BehaviorOCD_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "util/signals/simpleSignal_fwd.h"

#include "clad/externalInterface/messageEngineToGame.h"

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
    
    virtual bool IsRunnable(float currentTime_sec) const override;

    virtual Result Init() override;
    
    virtual Status Update(float currentTime_sec) override;
    
    // Finish placing current object if there is one, otherwise good to go
    virtual Result Interrupt(float currentTime_sec) override;
    
    virtual bool GetRewardBid(Reward& reward) override;
    
  private:
    
    typedef enum {
      DONT_CARE_IF_OBJECT_ON_TOP,
      OBJECT_ON_TOP,
      NO_OBJECT_ON_TOP
    } ObjectOnTopStatus;
    
    typedef enum {
      MESSY,
      NEAT,
      COMPLETE
    } BlockLightState;
    
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
    void ComputeAlignedPoses(const Pose3d& basePose, f32 distance, std::vector<Pose3d> &alignedPoses);
    
    // Gets the object within objectSet that is closest to the robot and
    // 1) at the specified heighLevel (0 == equal same as robot, 1 == high dock height) and
    // 2) honors the ObjectOnTopStatus
    Result GetClosestObjectInSet(const std::set<ObjectID>& objectSet, u8 heightLevel, ObjectOnTopStatus ootStatus, ObjectID &objID);
    
    void SetBlockLightState(const std::set<ObjectID>& objectSet,  BlockLightState state);
    void SetBlockLightState(const ObjectID& objID, BlockLightState state);
    
    // Updates all block lights according to messy/neat state
    void UpdateBlockLights();
    
    // Set/Unset the location of the last pick or place failure
    void SetLastPickOrPlaceFailure(const ObjectID& objectID, const Pose3d& pose);
    void UnsetLastPickOrPlaceFailure();
    
    // Deletes the object if it was also the previous object that the robot
    // had failed to pick or place from the same robot pose.
    void DeleteObjectIfFailedToPickOrPlaceAgain(const ObjectID& objectID);
    
    // Checks if blocks are "aligned".
    // A messy block that is aligned with a neat block should be considered neat
    bool AreAligned(const ObjectID& o1, const ObjectID& o2);
    
    // Checks the neatness of any blocks that it has observed within the last second
    void VerifyNeatness();
    
    // Returns neatness score of object
    f32 GetNeatnessScore(const ObjectID& whichObject);
    
    // Goal is to move objects from messy to neat
    std::set<ObjectID> _messyObjects;
    std::set<ObjectID> _neatObjects;
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    enum class State {
      PickingUpBlock,
      PlacingBlock
    };
    
    State _currentState;
    bool  _interrupted;
    
    Result _lastHandlerResult;
    std::vector<::Signal::SmartHandle> _eventHandles;
    
    // Enumerate possible arrangements Cozmo "likes".
    enum class Arrangement {
      StacksOfTwo = 0,
      Line,
      NumArrangements
    };
    
    Arrangement _currentArrangement;
    
    ObjectID _objectToPickUp;
    ObjectID _objectToPlaceOn;
    ObjectID _lastObjectPlacedOnGround;
    ObjectID _anchorObject; // the object the arrangement is anchored to
    
    // If it fails to pickup or place the same object a certain number of times in a row
    // then delete the object. Assuming that the failures are due to not being able to see
    // an object where it used to be.
    ObjectID _lastObjectFailedToPickOrPlace;
    Pose3d _lastPoseWhereFailedToPickOrPlace;
 
    // ID tag of last queued action
    u32 _lastActionTag;
    
    void UpdateName();
    
  }; // class BehaviorOCD

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorOCD_H__