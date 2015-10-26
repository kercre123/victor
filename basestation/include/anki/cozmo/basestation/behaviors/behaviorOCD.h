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

#include "clad/externalInterface/messageEngineToGame.h"

#include <set>
#include <queue>

namespace Anki {
namespace Cozmo {
  
  // A behavior that tries to neaten up blocks present in the world
  class BehaviorOCD : public IBehavior
  {
  public:
    
    BehaviorOCD(Robot& robot, const Json::Value& config);
    virtual ~BehaviorOCD() { }
    
    virtual bool IsRunnable(double currentTime_sec) const override;
    
  private:
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
   
    // Finish placing current object if there is one, otherwise good to go
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
    
    // Offset at which low block should be placed wrt
    // another low block. (Twice block width)
    constexpr static f32 kLowPlacementOffsetMM = 88.f;
    
    // If neat block is disturbed within this amount of time of
    // another neat block being disturbed then escalate to major irritation animation.
    constexpr static f32 kMajorIrritationTimeIntervalSec = 5.f;
    
    // If a new block was observed within this amount of time before
    // Init() is called then do the 'oooo' animation.
    constexpr static f32 kExcitedAboutNewBlockTimeIntervalSec = 2.f;
    
    // Number of blocks that need to be neatly stacked in order to warrant
    // a celebratory dance.
    constexpr static u32 kNumBlocksForCelebration = 4;
    
    // If the robot is not executing any action for this amount of time,
    // something went wrong so start it up again.
    constexpr static f32 kInactionFailsafeTimeoutSec = 1;
    
    enum class ObjectOnTopStatus {
      DontCareIfObjectOnTop,
      ObjectOnTop,
      NoObjectOnTop
    };
    
    enum class BlockLightState {
      Messy,
      Neat,
      Complete
    };
    
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
    virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;
    
    // Handlers for signals coming from the engine
    // TODO: These need to be some kind of _internal_ signal or event
    bool HandleObjectMovedHelper(const Robot& robot, const ObjectID& objectID, double currentTime_sec);
    Result HandleObjectMovedWhileRunning(Robot& robot, const ObjectMoved &msg, double currentTime_sec);
    Result HandleObjectMovedWhileNotRunning(const Robot& robot, const ObjectMoved &msg, double currentTime_sec);
    
    bool HandleObservedObjectHelper(const Robot& robot, const ObjectID& objectID, double currentTime_sec);
    Result HandleObservedObjectWhileRunning(Robot& robot, const ExternalInterface::RobotObservedObject& msg, double currentTime_sec);
    Result HandleObservedObjectWhileNotRunning(const Robot& robot, const ExternalInterface::RobotObservedObject& msg, double currentTime_sec);
    
    Result HandleDeletedObject(const ExternalInterface::RobotDeletedObject& msg, double currentTime_sec);
    
    Result HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction& msg, double currentTime_sec);
    Result HandleBlockPlaced(const ExternalInterface::BlockPlaced& msg, double currentTime_sec);
    
    Result SelectArrangement();
    Result SelectNextObjectToPickUp(Robot& robot);
    Result SelectNextPlacement(Robot& robot);
    Result FindEmptyPlacementPose(const Robot& robot, const ObjectID& nearObjectID, const f32 offset_mm, Pose3d& pose);
    void ComputeAlignedPoses(const Pose3d& basePose, f32 distance, std::vector<Pose3d> &alignedPoses);
    
    // Gets the object within objectSet that is closest to the robot and
    // 1) at the specified heighLevel (0 == equal same as robot, 1 == high dock height) and
    // 2) honors the ObjectOnTopStatus
    Result GetClosestObjectInSet(const Robot& robot, const std::set<ObjectID>& objectSet, u8 heightLevel,
                                 ObjectOnTopStatus ootStatus, ObjectID &objID);
    
    void SetBlockLightState(Robot& robot, const std::set<ObjectID>& objectSet,  BlockLightState state);
    void SetBlockLightState(Robot& robot, const ObjectID& objID, BlockLightState state);
    
    // Updates all block lights according to messy/neat state
    void UpdateBlockLights(Robot& robot);
    
    // Set/Unset the location of the last pick or place failure
    void SetLastPickOrPlaceFailure(const ObjectID& objectID, const Pose3d& pose);
    void UnsetLastPickOrPlaceFailure();
    
    // Deletes the object if it was also the previous object that the robot
    // had failed to pick or place from the same robot pose.
    // Returns true if object deleted.
    bool DeleteObjectIfFailedToPickOrPlaceAgain(Robot& robot, const ObjectID& objectID);
    
    // Checks if blocks are "aligned".
    // A messy block that is aligned with a neat block should be considered neat
    bool AreAligned(const Robot& robot, const ObjectID& o1, const ObjectID& o2);
    
    // Checks the neatness of any blocks that it has observed within the last second
    void VerifyNeatness(const Robot& robot, bool& foundMessyToNeat,
                        bool& foundNeatToMessy, bool& needToCancelLastActionTag);
    
    // Returns neatness score of object
    f32 GetNeatnessScore(const Robot& robot, const ObjectID& whichObject);
    
    // Goal is to move objects from messy to neat
    std::set<ObjectID> _messyObjects;
    std::set<ObjectID> _neatObjects;
    
    // A map of the number of times an object was observed to be
    // in a neatness state other than what it currently is stored as
    // so that VerifyNeatness can convert it only when it has accumulated
    // a requisite number of observations in the alternate state.
    std::map<ObjectID, s8> _conversionEvidenceCount;
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    enum class State {
      PickingUpBlock,
      PlacingBlock,
      Animating,
      FaceDisturbedBlock
    };
    
    State _currentState;
    bool  _interrupted;
    
    Result _lastHandlerResult;
    
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
    ObjectID _blockToFace;  // the disturbed block that robot should look to before acting irritated
    
    // If it fails to pickup or place the same object a certain number of times in a row
    // then delete the object. Assuming that the failures are due to not being able to see
    // an object where it used to be.
    ObjectID _lastObjectFailedToPickOrPlace;
    Pose3d _lastPoseWhereFailedToPickOrPlace;
 
    // ID tag of last queued action
    u32 _lastActionTag;
    std::map<u32, std::string> _animActionTags;
    
    f32 _lastNeatBlockDisturbedTime;
    f32 _lastNewBlockObservedTime;
    f32 _inactionStartTime;
    
    void UpdateName();
    
    void PlayAnimation(Robot& robot, const std::string& animName);
    void FaceDisturbedBlock(Robot& robot, const ObjectID& objID);
    
    std::queue<std::pair<ObjectID, BlockLightState> > _blocksToSetLightState;
    
    void MakeNeat(const ObjectID& objID);
    void MakeMessy(const ObjectID& objID);
    
  }; // class BehaviorOCD

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorOCD_H__
