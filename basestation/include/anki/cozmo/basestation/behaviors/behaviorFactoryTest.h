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
 *                - Drive straight off charger until cliff detected
 *                - Backup to pose for camera calibration
 *                - Turn to take pictures of all calibration targets and calibrate
 *                - Go to pickup block
 *                - Place block back down
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/pathMotionProfile.h"
#include "clad/types/factoryTestTypes.h"


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
    
    virtual bool IsRunnable(const Robot& robot) const override;
    
  private:
    
    // Test constants
    const Pose3d _cliffDetectPose;
    const Pose3d _camCalibPose;
    const Pose3d _prePickupPose;
    const Pose3d _expectedLightCubePose;
          Pose3d _actualLightCubePose;
    const Pose3d _expectedChargerPose;
    std::vector<std::pair<f32,f32> > _camCalibPanAndTiltAngles;
    
    static constexpr f32 _kRobotPoseSamenessDistThresh_mm = 10;
    static constexpr f32 _kRbotPoseSamenessAngleThresh_rad = DEG_TO_RAD(5);
    static constexpr u32 _kNumPickupRetries = 1;
    static constexpr f32 _kIMUDriftDetectPeriod_sec = 2.f;
    static constexpr f32 _kIMUDriftAngleThreshDeg = 0.2f;
    static constexpr u32 _kMinNumberOfCalibrationImagesRequired = 5;

    // If no change in behavior state for this long then trigger failure
    static constexpr f32 _kWatchdogTimeout = 20;    
    
    // Compute rotation ambiguities.
    // As long as the cube is upright, it's fine.
    static const std::vector<RotationMatrix3d> _kBlockRotationAmbiguities;
    
    virtual Result InitInternal(Robot& robot) override;
    virtual Status UpdateInternal(Robot& robot) override;
    void EndTest(Robot& robot, FactoryTestResultCode resCode);
    void PrintAndLightResult(Robot& robot, FactoryTestResultCode res);
   
    virtual void   StopInternal(Robot& robot) override;
    
    virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

    
    // Handlers for signals coming from the engine
    Result HandleObservedObject(Robot& robot, const ExternalInterface::RobotObservedObject& msg);
    Result HandleDeletedObject(const ExternalInterface::RobotDeletedObject& msg);
    Result HandleObjectMoved(const Robot& robot, const ObjectMoved &msg);
    Result HandleCameraCalibration(Robot& robot, const CameraCalibration &msg);
    Result HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg);
    Result HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction& msg);
    
    void SetCurrState(FactoryTestState s);
    void UpdateStateName();
    

    // returns true if the callback handled the action, false if we should continue to handle it in HandleActionCompleted
    using ActionResultCallback = std::function<bool(const ActionResult& result, const ActionCompletedUnion& completionInfo)>;
    
    void StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback = {});

    std::map<u32, ActionResultCallback> _actionCallbackMap;
    bool IsActing() const {return !_actionCallbackMap.empty(); }
    
    FactoryTestState  _currentState;
    f32               _holdUntilTime = -1.0f;
    Radians           _startingRobotOrientation;
    Result            _lastHandlerResult;
    PathMotionProfile _motionProfile;
 
    // Map of action tags that have been commanded to callback functions
    std::map<u32, std::string> _animActionTags;
    ActionResultCallback       _actionResultCallback;

    // ID of block to pickup
    ObjectID _blockObjectID;
    ObjectID _cliffObjectID;
    ObjectID _chargerObjectID;
    
    
    u32      _camCalibPoseIndex = 0;
    f32      _watchdogTriggerTime = -1.0;
    
    s32 _attemptCounter = 0;
    bool _calibrationReceived = false;
    FactoryTestResultCode _testResult;
    
  }; // class BehaviorFactoryTest

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__
