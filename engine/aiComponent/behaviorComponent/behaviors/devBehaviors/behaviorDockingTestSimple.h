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
#include "coretech/vision/engine/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#include <fstream>

namespace Anki {
  namespace Cozmo {
    
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

      
        void InitBehavior() override;
      
      private:
      
        virtual void OnBehaviorActivated() override;
      
        virtual void BehaviorUpdate() override;

        virtual void OnBehaviorDeactivated() override;
        
        virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
      
        // Handlers for signals coming from the engine/robot
        void HandleObservedObject(Robot& robot, const ExternalInterface::RobotObservedObject& msg);
        void HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg);
        void HandleActionCompleted(Robot& robot, const ExternalInterface::RobotCompletedAction& msg);
        void HandleDockingStatus(const AnkiEvent<RobotInterface::RobotToEngine>& message);
      
        // returns true if the callback handled the action, false if we should continue to handle it in HandleActionCompleted
        using ActionResultCallback = std::function<bool(const ActionResult& result, const ActionCompletedUnion& completionInfo)>;
      
        void DelegateIfInControl(Robot& robot, IActionRunner* action, ActionResultCallback callback = {});
      
        bool IsControlDelegated() const {return !_actionCallbackMap.empty(); }
      
        // Records this attempt and sets state to reset
        void EndAttempt(Robot& robot, ActionResult result, std::string name, bool endingFromFailedAction = false);
      
        // Write out information relating to this attempt
        void RecordAttempt(ActionResult result, std::string name);
      
        void PrintStats();
      
        enum class State {
          Init,
          Inactive,
          Roll,
          PickupLow,
          PlaceLow,
          Reset,
          ManualReset
        };
      
        void SetCurrState(State s);
        void SetCurrStateAndFlashLights(State s, Robot& robot);
        const char* StateToString(const State m);
        void UpdateStateName();
      
        State _currentState = State::Init;
      
        std::map<u32, ActionResultCallback> _actionCallbackMap;
        Signal::SmartHandle _signalHandle;
      
        // ID of block to pickup
        ObjectID _blockObjectIDPickup;
      
        Vision::Marker _initialVisionMarker;
        Vision::Marker _markerBeingSeen;
      
        // Where the cube should be put down
        Pose3d _cubePlacementPose;
      
        const f32 kInvalidAngle = 1000;
        f32    _initialPreActionPoseAngle_rad = kInvalidAngle;
      
        void Write(const std::string& s);
      
        std::unique_ptr<Util::RollingFileLogger> _logger;
        std::string _imageFolder;  // image folder name
      
        bool _reset                = false; // test needs to be reset
        bool _yellForHelp          = false; // whether or not we should yell for help
        bool _yellForCompletion    = false; // whether or not we should yell the test completed
        bool _endingFromFailedAction = false;
        int _numSawObject           = 0;
      
        // Stats for test/attempts
        int _numFails              = 0;
        int _numDockingRetries     = 0;
        int _numAttempts           = 0;
        int _numExtraAttemptsDueToFailure = 0;
        int _numConsecFails        = 0;
        bool _didHM                = false;
        bool _failedCurrentAttempt = false;
        TimeStamp_t _attemptStartTime;
        Pose3d _initialRobotPose;
        Pose3d _initialCubePose;
    };
  }
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDockingTest_H__
