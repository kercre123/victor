/**
 * File: behaviorDockingTestSimple.h
 *
 * Author: Al Chaussee
 * Date:   05/09/2016
 *
 * Description: Simple docking test behavior
 *              Saves attempt information to log webotsCtrlGameEngine/temp
 *              Saves images with timestamps in webotsCtrlViz/images
 *
 *              Init conditions:
 *                - Cozmo is about 154mm away from center of a cube facing it
 *
 *              Behavior:
 *                - Pick up cube
 *                - Place cube
 *                - Drive back to start pose with random offsets
 *                - Repeat until consecutively fails kMaxConsecFails times or does kMaxNumAttempts attempts
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDockingTestSimple_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDockingTestSimple_H__

#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_hash.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/vision/engine/visionMarker.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#include <fstream>

namespace Anki {
namespace Vector {
  
namespace ExternalInterface {
  struct RobotObservedObject;
  struct RobotStopped;
  struct RobotCompletedAction;
  struct RobotToEngine;
}

class BehaviorDockingTestSimple : public ICozmoBehavior
{
protected:
  friend class BehaviorFactory;
  BehaviorDockingTestSimple(const Json::Value& config);

public:      
  virtual ~BehaviorDockingTestSimple() { }

  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}


  void InitBehavior() override;

private:
  // returns true if the callback handled the action, false if we should continue to handle it in HandleActionCompleted
  using ActionResultCallback = std::function<bool(const ActionResult& result, const ActionCompletedUnion& completionInfo)>;

  enum class State {
    Init,
    Inactive,
    Roll,
    PickupLow,
    PlaceLow,
    Reset,
    ManualReset
  };

  struct InstanceConfig {
    InstanceConfig();
    std::string imageFolder;  // image folder name
    Signal::SmartHandle signalHandle;
    std::unique_ptr<Util::RollingFileLogger> logger;
  };

  struct DynamicVariables {
    DynamicVariables();
    State currentState;
    std::map<u32, ActionResultCallback> actionCallbackMap;

    // ID of block to pickup
    ObjectID       blockObjectIDPickup;
    Vision::Marker initialVisionMarker;
    Vision::Marker markerBeingSeen;
    
    // Where the cube should be put down
    Pose3d cubePlacementPose;
    f32    initialPreActionPoseAngle_rad;
    bool   reset;             // test needs to be reset
    bool   yellForHelp;       // whether or not we should yell for help
    bool   yellForCompletion; // whether or not we should yell the test completed
    bool   endingFromFailedAction;
    int    numSawObject;

    // Stats for test/attempts
    int  numFails;
    int  numDockingRetries;
    int  numAttempts;
    int  numExtraAttemptsDueToFailure;
    int  numConsecFails;
    bool didHM;
    bool failedCurrentAttempt;

    RobotTimeStamp_t attemptStartTime;
    Pose3d      initialRobotPose;
    Pose3d      initialCubePose;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  virtual void OnBehaviorActivated() override;

  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;

  // Handlers for signals coming from the engine/robot
  void HandleObservedObject(Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  void HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg);
  void HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction& msg);
  void HandleDockingStatus(const AnkiEvent<RobotInterface::RobotToEngine>& message);

  void DelegateIfInControl(Robot& robot, IActionRunner* action, ActionResultCallback callback = {});

  bool IsControlDelegated() const {return !_dVars.actionCallbackMap.empty(); }

  // Records this attempt and sets state to reset
  void EndAttempt(Robot& robot, ActionResult result, std::string name, bool endingFromFailedAction = false);

  // Write out information relating to this attempt
  void RecordAttempt(ActionResult result, std::string name);

  void PrintStats();

  void SetCurrState(State s);
  void SetCurrStateAndFlashLights(State s, Robot& robot);
  const char* StateToString(const State m);
  void UpdateStateName();

  void Write(const std::string& s);
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorDockingTest_H__
