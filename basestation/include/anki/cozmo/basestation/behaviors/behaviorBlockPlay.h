/**
 * File: behaviorBlockPlay.h
 *
 * Author: Kevin Yoon
 * Date:   11/24/15
 *
 * Description: Implements a block play behavior
 *
 *              Init conditions:
 *                - Cozmo sees a block that's above ground level and has very recently seen a face.
 *
 *              Behavior:
 *                - player presents block and cozmo tracks it
 *                - player places block on ground sideways
 *                - cozmo rolls it upright and it lights up
 *                - cozmo celebrates and picks up block
 *                - cozmo looks to player and begs for another block
 *                - player places block on ground upright
 *                - cozmo stacks carried block on top of it and checks out stack
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBlockPlay_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBlockPlay_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/trackedFace.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/pathMotionProfile.h"


namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class IDriveToInteractWithObject;
  
  class BehaviorBlockPlay : public IBehavior
  {
  protected:
    
    // Enforce creation through BehaviorFactory
    friend class BehaviorFactory;
    BehaviorBlockPlay(Robot& robot, const Json::Value& config);
    
  public:

    virtual ~BehaviorBlockPlay() { }
    
    virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
    
  private:
    using Face = Vision::TrackedFace;
    
    enum class BlockLightState {
      None,
      Visible,
      Upright,
      Complete
    };
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    enum class State {
      TrackingFace,
      TrackingBlock,
      InspectingBlock,
      RollingBlock,
      PickingUpBlock,
      PlacingBlock,
      WaitingForBlock,
      SearchingForMissingBlock, // this is only entered if a block "disappears" on us
    };
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
   
    // Finish placing current object if there is one, otherwise good to go
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
    
    virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
    
    // If the robot is not executing any action for this amount of time,
    // something went wrong so start it up again.
    constexpr static f32 kInactionFailsafeTimeoutSec = 1;
    
    // Height to set lift to once it has picked up the first block
    constexpr static f32 kLowCarryHeightMM = 45;
    
    // Robot raises lift if object that it's tracking falls below this angle and it's
    // also holding a block
    const f32 kTrackedObjectViewAngleThreshForRaisingCarriedBlock = 0.6; //atan2f(90, 200);
    
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
    virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;

    virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) override;
    
    // Handlers for signals coming from the engine
    Result HandleObservedObjectWhileRunning(Robot& robot,
                                            const ExternalInterface::RobotObservedObject& msg,
                                            double currentTime_sec);
    Result HandleObservedObjectWhileNotRunning(const Robot& robot,
                                               const ExternalInterface::RobotObservedObject& msg,
                                               double currentTime_sec);

    // returns true if we care about this object
    bool HandleObservedObjectHelper(const Robot& robot,
                                    const ExternalInterface::RobotObservedObject& msg,
                                    double currentTime_sec);
    
    Result HandleDeletedObject(const ExternalInterface::RobotDeletedObject& msg, double currentTime_sec);
    Result HandleObservedFace(const Robot& robot,
                              const ExternalInterface::RobotObservedFace& msg,
                              double currentTime_sec);
    Result HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg);
    Result HandleActionCompleted(Robot& robot,
                                 const ExternalInterface::RobotCompletedAction& msg,
                                 double currentTime_sec);
    Result HandleObjectMoved(const Robot& robot, const ObjectMoved &msg);

    void TrackBlockWithLift(Robot& robot, const Pose3d& objectPose);

    // tries to move the robot towards a face, properly handling things like if the robot is holding a cube,
    // if it has seen a face, etc
    void TurnTowardsAFace(Robot& robot);
    
    void InitState(const Robot& robot);
    void SetCurrState(State s);
    void UpdateStateName();
    void PlayAnimation(Robot& robot, const std::string& animName, bool sequential = true);
    void SetBlockLightState(Robot& robot, const ObjectID& objID, BlockLightState state);

    // returns true if the callback handled the action, false if we should continue to handle it in HandleActionCompleted
    using ActionResultCallback = std::function<bool(ActionResult result)>;
    
    void StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback = {});
    void SetDriveToObjectSounds(IDriveToInteractWithObject* action);
    
    State _currentState;
    bool  _interrupted;
    bool _isActing = false;

    f32 _holdUntilTime = -1.0f;

    // if we "lose" the block, we can use State::SearchingForMissingBlock. If we see it again, go back to this state
    State _missingBlockFoundState = State::TrackingFace;

    // if we are "searching" for a missing block, consider it found if it is seen after this time
    f32 _searchStartTime = 0.0f;

    // if we are expecting a block, this is the time we started waiting to see it (not counting time to move
    // the head / lift out of the way)
    f32 _waitForBlockStartTime = 0.0f;

    const f32 _maxTimeToWaitForBlock = 3.5f;
    
    Result _lastHandlerResult;

    PathMotionProfile _motionProfile;

    // If it fails to pickup or place the same object a certain number of times in a row
    // then delete the object. Assuming that the failures are due to not being able to see
    // an object where it used to be.
    ObjectID _lastObjectFailedToPickOrPlace;
    Pose3d _lastPoseWhereFailedToPickOrPlace;
 
    // ID tag of last queued action
    u32 _lastActionTag;
    std::map<u32, std::string> _animActionTags;
    ActionResultCallback _actionResultCallback;

    //f32 _inactionStartTime;
    
    // ID of player face that Cozmo will interact with for this behavior
    Face::ID_t _faceID;
    
    double _noFacesStartTime = -1.0;
    
    float _oldHeadAngle_rads = 0.0f;
    
    // The block that currently has Cozmo's attention
    ObjectID _trackedObject;

    // blocks that should be ignored (which happens at the end of the demo)
    std::set<ObjectID> _objectsToIgnore;

    std::set<ObjectID> _objectsToTurnOffLights;

    // used for moving the lift to track (grab at) the cube
    const f32 _maxObjectDistToMoveLift = 145.0f;
    const f32 _minObjectDistanceToMove = 180.0f;
    const f32 _minTrackingAngleToMove_rads = DEG_TO_RAD(10.0f);
    const f32 _distToDriveForwardWhileTracking = 40.0f;
    const f32 _speedToDriveForwardWhileTracking = 90.0f;
    const f32 _highLiftHeight = 70.0f;
    const f32 _minHeadAngleforLiftUp_rads = DEG_TO_RAD(20.0f);
    const f32 _lostBlockTimeToLookDown = 1.2f;
    const f32 _waitForBlockHeadAngle_rads = DEG_TO_RAD(-10.0f);
    const f32 _timetoInspectBlock = 0.3f;
    const f32 _robotObjectOrientationDiffThreshForDirectPickup = DEG_TO_RAD(3);
    const f32 _angleToObjectThreshForDirectPickup = DEG_TO_RAD(3);
    u32 _driveForwardActionTag = 0;
    bool _isDrivingForward = false;

    bool _lockedLift = false;
    void LiftShouldBeLocked(Robot& robot);
    void LiftShouldBeUnlocked(Robot& robot);

    bool _lockedHead = false;
    void HeadShouldBeLocked(Robot& robot);
    void HeadShouldBeUnlocked(Robot& robot);

    bool _lockedBody = false;
    void BodyShouldBeLocked(Robot& robot);
    void BodyShouldBeUnlocked(Robot& robot);

    void IgnoreObject(Robot& robot, ObjectID objectID);


    // The last time we saw _trackedObject
    f32 _lastObjectObservedTime = 0.0f;

    f32 _trackedObjectStoppedMovingTime = -1.0f;

    // how long ago we should have seen the object to attempt interaction
    const f32 _objectObservedTimeThreshold = 0.75f;

    // This ID is set when the first block is on the ground.
    // This is the block that gets picked up and placed upon the second block.
    ObjectID _objectToPickUp;
    
    // This ID is set when the second block is on the ground.
    // This is the block upon which the first block is placed.
    ObjectID _objectToPlaceOn;

    // The higher this gets, the more frustrated
    // TODO: Base on moodmanager instead?
    s32 _attemptCounter = 0;
    
  }; // class BehaviorBlockPlay

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorBlockPlay_H__
