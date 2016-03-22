/**
 * File: behaviorFactoryTest.h
 *
 * Author: Kevin Yoon
 * Date:   03/18/2016
 *
 * Description: Functional test fixture behavior
 *
 *              Init conditions:
 *                - Cozmo is on starting charger of the test fixture
 *
 *              Behavior:
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/trackedFace.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/pathMotionProfile.h"


namespace Anki {
namespace Cozmo {
  
  class BehaviorFactoryTest : public IBehavior
  {
  protected:
    
    // Enforce creation through BehaviorFactory
    friend class BehaviorFactory;
    BehaviorFactoryTest(Robot& robot, const Json::Value& config);
    
  public:

    virtual ~BehaviorFactoryTest() { }
    
    virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
    
  private:
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    enum class State {
      StartOnCharger,           // Verify that robot is on charger and IMU is not drifting
      DriveToSlot,              // Drive forward until cliff event. Set robot's pose.

      // Check head and lift motors?
      
      GotoCalibrationPose,      // Drive to pose from which calibration images are to be taken
      TakeCalibrationImages,    // Take and save images of calibration patterns
      ComputeCameraCalibration, // Compute calibration and check that values are reasonable
      GotoPickupPose,
      
      PickingUpBlock,
      PlacingBlock,
    };
    
    enum class FactoryTestErrorCode {
      UNKNOWN = 0,
      
      SUCCESS,
      
      CHARGER_UNDETECTED,
      STILL_ON_CHARGER,
      CLIFF_UNDETECTED,
      NOT_IN_CALIBRATION_POSE,
      CALIBRATION_VALUES_OOR,
      NOT_IN_PRE_PICKUP_POSE,
      CUBE_NOT_FOUND,
      CUBE_NOT_WHERE_EXPECTED,
      PICKUP_FAILED,
      UNEXPECTED_OBSERVED_OBJECT,
    };
    
    
    // Test constants
//    constexpr static kIMUDriftTimeout_ms = 2000
    const Pose3d _cliffDetectPose;
    const Pose3d _camCalibPose;
    std::vector<std::pair<f32,f32> > _camCalibPanAndTiltAngles;

    const Pose3d _prePickupPose;
    const Pose3d _expectedLightCubePose;
    
    static constexpr f32 _kRobotPoseSamenessDistThresh_mm = 10;
    static constexpr f32 _kRbotPoseSamenessAngleThresh_rad = DEG_TO_RAD(5);

    
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
    void EndTest(FactoryTestErrorCode errCode);
   
    // Finish placing current object if there is one, otherwise good to go
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
    
    virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
    
    // If the robot is not executing any action for this amount of time,
    // something went wrong so start it up again.
    constexpr static f32 kInactionFailsafeTimeoutSec = 1;
    
    // Height to set lift to once it has picked up the first block
    constexpr static f32 kLowCarryHeightMM = 45;
    
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

    
    // Handlers for signals coming from the engine
    Result HandleObservedObject(Robot& robot,
                                const ExternalInterface::RobotObservedObject& msg,
                                double currentTime_sec);
    
    Result HandleDeletedObject(const ExternalInterface::RobotDeletedObject& msg, double currentTime_sec);

    Result HandleActionCompleted(Robot& robot,
                                 const ExternalInterface::RobotCompletedAction& msg,
                                 double currentTime_sec);
    Result HandleObjectMoved(const Robot& robot, const ObjectMoved &msg);

    
    void InitState(const Robot& robot);
    void SetCurrState(State s);
    void UpdateStateName();
    

    // returns true if the callback handled the action, false if we should continue to handle it in HandleActionCompleted
    using ActionResultCallback = std::function<bool(ActionResult result)>;
    
    void StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback = {});

    
    State _currentState;
    bool  _interrupted;
    bool _isActing = false;
    
    f32 _holdUntilTime = -1.0f;
    
    Result _lastHandlerResult;

    PathMotionProfile _motionProfile;
 
    // ID tag of last queued action
    u32 _lastActionTag;
    std::map<u32, std::string> _animActionTags;
    ActionResultCallback _actionResultCallback;

    // ID of block to pickup
    ObjectID _blockObjectID;
    ObjectID _cliffObjectID;
    ObjectID _chargerObjectID;
    
    s32 _attemptCounter = 0;
    
    u32 _camCalibPoseIndex = 0;
    
    FactoryTestErrorCode _testResult;
    
  }; // class BehaviorFactoryTest

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__
