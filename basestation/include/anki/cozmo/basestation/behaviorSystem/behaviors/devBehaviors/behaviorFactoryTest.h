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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "clad/types/pathMotionProfile.h"
#include "clad/types/factoryTestTypes.h"

namespace Anki {
namespace Cozmo {

  class Robot;
  
  class BehaviorFactoryTest : public IBehavior
  {
  protected:
    
    // Enforce creation through BehaviorFactory
    friend class BehaviorFactory;
    BehaviorFactoryTest(Robot& robot, const Json::Value& config);
    
  public:

    virtual ~BehaviorFactoryTest() { }
    
    virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
    virtual bool CarryingObjectHandledInternally() const override{ return true;}

  private:
    
    virtual Result InitInternal(Robot& robot) override;
    virtual Status UpdateInternal(Robot& robot) override;
    virtual void   StopInternal(Robot& robot) override;
    virtual void   HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
    
    void EndTest(Robot& robot, FactoryTestResultCode resCode);
    void PrintAndLightResult(Robot& robot, FactoryTestResultCode res);

    void QueueWriteToRobot(Robot& robot, NVStorage::NVEntryTag tag, const u8* data, size_t size);
    bool SendQueuedWrites(Robot& robot);
    
    struct WriteEntry {
      WriteEntry(NVStorage::NVEntryTag tag,
                 const u8* data, size_t size,
                 NVStorageComponent::NVStorageWriteEraseCallback callback)
      : _tag(tag)
      , _callback(callback) {
        _data.assign(data, data+size);
      }
      
      NVStorage::NVEntryTag _tag;
      std::vector<u8> _data;
      NVStorageComponent::NVStorageWriteEraseCallback _callback;
    };
    void PackWrites(const std::list<WriteEntry>& writes, std::vector<u8>& packedData);
    

    // Handlers for signals coming from the engine
    Result HandleObservedObject(Robot& robot, const ExternalInterface::RobotObservedObject& msg);
    Result HandleDeletedLocatedObject(const ExternalInterface::RobotDeletedLocatedObject& msg);
    Result HandleObjectMoved(const Robot& robot, const ObjectMoved &msg);
    Result HandleCameraCalibration(Robot& robot, const CameraCalibration &msg);
    Result HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg);
    Result HandleRobotOfftreadsStateChanged(Robot& robot, const ExternalInterface::RobotOffTreadsStateChanged &msg);
    Result HandleMotorCalibration(Robot& robot, const MotorCalibration &msg);
    
    // Handlers for signals coming from robot
    void HandleActiveObjectAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& msg);
    void HandlePickAndPlaceResult(const AnkiEvent<RobotInterface::RobotToEngine>& msg);
    void HandleFactoryTestParameter(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleMfgID(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleFactoryFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message);

    void SetCurrState(FactoryTestState s);
    void UpdateStateName();
    
    void SendTestResultToGame(Robot& robot, FactoryTestResultCode resCode);
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
    FactoryTestState  _currentState;
    f32               _holdUntilTime = -1.0f;
    Radians           _startingRobotOrientation;
    Result            _lastHandlerResult;
    PathMotionProfile _motionProfile;
    s32               _stationID;
    
    FactoryTestResultCode _prevResult;
    bool                  _prevResultReceived;    
    bool                  _hasBirthCertificate;
    
    bool                  _writeTestResult;
    bool                  _eraseBirthCertificate;
 
    // Map of action tags that have been commanded to callback functions
    std::map<u32, std::string> _animActionTags;
    ActionResultCallback       _actionResultCallback;

    // ID of block to pickup
    ObjectID _blockObjectID;
    ObjectID _cliffObjectID;
    
    u32      _camCalibPoseIndex = 0;
    f32      _watchdogTriggerTime = -1.0;
    
    bool     _toolCodeImagesStored;

    bool     _headCalibrated;
    bool     _liftCalibrated;
    
    bool     _blockPickedUpReceived;
    Radians  _robotAngleAtPickup;
    Radians  _robotAngleAfterBackup;
    
    s32 _attemptCounter = 0;
    bool _calibrationReceived = false;
    FactoryTestResultCode _testResult;
    FactoryTestResultEntry _testResultEntry;
    std::vector<u32> _stateTransitionTimestamps;
    
    FactoryTestLogger _factoryTestLogger;
    
    std::list<WriteEntry> _queuedWrites;
    FactoryTestResultCode _writeFailureCode;

    static PoseData ConvertToPoseData(const Pose3d& p);
    u8 _numPlacementAttempts;
    
    bool _activeObjectAvailable = false;
    
    bool _gotHWVersion = false;
    bool _hasWrongFirmware = false;
    std::string _fwBuildType = "";
    std::string _fwVersion = "";
    
    Pose3d _closestPredockPose;
    
    Robot& _robot;
    
  }; // class BehaviorFactoryTest

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFactoryTest_H__
