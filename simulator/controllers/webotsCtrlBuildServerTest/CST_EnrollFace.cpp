/**
 * File: CST_EnrollFace
 *
 * Author: Andrew Stein
 * Created: 12/05/2016
 *
 * Description: Tests enroll face behavior for successes and failures.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "simulator/game/cozmoSimTestController.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

// If enabled, don't do the first enrollment where we don't see a face and just
// wait for timeout. This flag is useful for local testing where you don't want
// to wait for this initial long state to complete
#define SKIP_INITIAL_ENROLL_TIMEOUT 0

// Audio causes problems (timeouts) on the build server, so turning this on is useful locally
// for hearing the animation audio (including saying names after enrollment).
// Otherwise, it is best to leave it off until audio throttling on the server is figered out.
#define ENABLE_AUDIO 0


namespace Anki {
namespace Cozmo {
  
namespace{
enum class TestState {
  Init,
  WaitForFacePoseChange,
  FaceReappear,
  WaitForLookDown,
  
  SimpleEnroll,                           // Should succeed
  ReEnroll_FaceDisappears,                // Should fail
  ReEnroll_FaceDisappearsAndReappears,    // Should succeed
  NewEnroll,                              // Should succeed
  ReEnroll_SeeKnownFace,                  // Should fail
  NewEnroll_NoFace,                       // Should fail
  ReEnroll_SeeMultipleFaces,              // Should fail
};
  
}

class CST_EnrollFace : public CozmoSimTestController
{
public:
  CST_EnrollFace();

private:
  s32 UpdateSimInternal() override;

  void StartEnrollment(Vision::FaceID_t saveID, const std::string& name, bool sayName = true);
  
  void WaitToSetNewFacePose(double waitTime_sec, webots::Node* node, const Pose3d& newPose, TestState newState);
  void LookDownAndTransition(TestState nextState, const std::function<void()>& fcn);
  
  virtual void HandleFaceEnrollmentCompleted(const ExternalInterface::FaceEnrollmentCompleted &msg) override;
  
  static constexpr f32 kFaceEnrollmentTimeout_sec = 30.f;
  
  TestState _testState = TestState::Init;
  
  Vision::FaceID_t _faceID1, _faceID2;
  
  const Pose3d _enrollmentPose;
  const Pose3d _poseOutsideFOV;
  const Pose3d _nextToPose;
  
  webots::Node* _face1 = nullptr;
  webots::Node* _face2 = nullptr;
  
  bool _faceEnrollmentCompleted;
  ExternalInterface::FaceEnrollmentCompleted _faceEnrollmentCompletedMsg;

  TestState _nextState;
  
  double _waitStartTime;
  
  struct {
    double           waitTime_sec;
    webots::Node*    face;
    Pose3d           newPose;
  } _facePoseChange;
  
  std::function<void()> _callback;
  
};
  
REGISTER_COZMO_SIM_TEST_CLASS(CST_EnrollFace);

CST_EnrollFace::CST_EnrollFace()
: _enrollmentPose(6.02139f, X_AXIS_3D(), {0.f, -425.f, 350.f})
, _poseOutsideFOV(0, Z_AXIS_3D(), {10000.f, 0.f, 0.f})
, _nextToPose(6.02139f, X_AXIS_3D(), {-207.f, -513.f, 350.f})
{

}

void CST_EnrollFace::StartEnrollment(Vision::FaceID_t saveID, const std::string& name, bool sayName)
{
  using namespace ExternalInterface;
  
  const bool saveToRobot = true;
  const bool useMusic    = false;
  
  SetFaceToEnroll setFaceToEnroll(name, Vision::UnknownFaceID, saveID, saveToRobot, sayName, useMusic);
  
  _faceEnrollmentCompleted = false;
  
  SendMessage(MessageGameToEngine(std::move(setFaceToEnroll)));
  SendMessage(MessageGameToEngine(ExecuteBehaviorByID(
                                    BehaviorTypesWrapper::BehaviorIDToString(BEHAVIOR_ID(MeetVictor)), -1)));
}
  
void CST_EnrollFace::WaitToSetNewFacePose(double waitTime_sec, webots::Node* face, const Pose3d& newPose, TestState newState)
{
  _facePoseChange.face         = face;
  _facePoseChange.newPose      = newPose;
  _facePoseChange.waitTime_sec = waitTime_sec;
  _nextState                   = newState;
  
  _waitStartTime = GetSupervisor()->getTime();
  
  SET_TEST_STATE(WaitForFacePoseChange);
}
  
void CST_EnrollFace::LookDownAndTransition(TestState nextState, const std::function<void()>& fcn)
{
  SendMoveHeadToAngle(MIN_HEAD_ANGLE, 75.f, 100.f);
  SET_TEST_STATE(WaitForLookDown);
  _callback = fcn;
  _waitStartTime = GetSupervisor()->getTime();
  _nextState = nextState;
}
  
s32 CST_EnrollFace::UpdateSimInternal()
{
  switch (_testState)
  {
    case TestState::Init:
    {      
      _faceID1 = Vision::UnknownFaceID;
      
      _face1 = GetNodeByDefName("Face_1");
      _face2 = GetNodeByDefName("Face_2");
      
      CST_ASSERT(nullptr != _face1, "CST_EnrollFace.Init.MissingFace1");
      CST_ASSERT(nullptr != _face2, "CST_EnrollFace.Init.MissingFace2");
      
      PRINT_NAMED_INFO("CST_EnrollFace.Init",
                       "Setting both faces out of FOV and starting enrollment");
      
      // Hide both faces so first enrollment times out
      SetNodePose(_face1, _poseOutsideFOV);
      SetNodePose(_face2, _poseOutsideFOV);
      
      {
        using namespace ExternalInterface;
        
        // Reduce some timeouts to keep test from going super long (since we _expect_
        // timeouts in several cases below)
        SendMessage(MessageGameToEngine(SetDebugConsoleVarMessage("EnrollFace_Timeout_sec", "8")));
        SendMessage(MessageGameToEngine(SetDebugConsoleVarMessage("EnrollFace_TimeoutMax_sec", "15")));
        SendMessage(MessageGameToEngine(SetDebugConsoleVarMessage("EnrollFace_TimeoutExtraTime_sec", "4")));
        
        if(!ENABLE_AUDIO)
        {
          // Disable audio completely because it runs more slowly on the server and causes timeouts
          SendMessage(MessageGameToEngine(SetRobotAudioOutputSource(RobotAudioOutputSourceCLAD::NoDevice)));
        }
      }
      
      StartEnrollment(Vision::UnknownFaceID, "Face1");
      SET_TEST_STATE(NewEnroll_NoFace);
      
      if(SKIP_INITIAL_ENROLL_TIMEOUT)
      {
        _faceEnrollmentCompleted = true;
        _faceEnrollmentCompletedMsg.result = FaceEnrollmentResult::TimedOut;
      }
      
      break;
    }
      
    case TestState::WaitForFacePoseChange:
    {
      // After a few seconds, move the face out of the FOV and wait for enrollment to fail
      if(CONDITION_WITH_TIMEOUT_ASSERT(HasXSecondsPassedYet(_facePoseChange.waitTime_sec), _waitStartTime,
                                       _facePoseChange.waitTime_sec + 1))
      {
        SetNodePose(_facePoseChange.face, _facePoseChange.newPose);
        _testState = _nextState;
      }
      break;
    }

    case TestState::NewEnroll_NoFace:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::TimedOut)
      {
        PRINT_NAMED_INFO("CST_EnrollFace.NewEnroll_NoFace.Complete",
                         "Result: %s", EnumToString(_faceEnrollmentCompletedMsg.result));
        
        LookDownAndTransition(TestState::SimpleEnroll,
                              [this]() {
                                PRINT_NAMED_INFO("CST_EnrollFace.NewEnroll_NoFace.LookDownComplete",
                                                 "Starting enrollment of face 1");
                                SetNodePose(_face1, _enrollmentPose);
                                StartEnrollment(Vision::UnknownFaceID, "Face");
                              });
      }
      break;
    }
      
    case TestState::SimpleEnroll:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::Success,
                                            _faceEnrollmentCompletedMsg.name == "Face")
      {
        _faceID1 = _faceEnrollmentCompletedMsg.faceID;
        
        const s32 kMoveTime_sec = 3;
        PRINT_NAMED_INFO("CST_EnrollFace.SimpleEnroll.RestartEnrollment",
                         "Starting to enroll face %d again, triggering face to move in %d seconds",
                         _faceID1, kMoveTime_sec);
        
        StartEnrollment(_faceID1, "Face1");
        
        WaitToSetNewFacePose(kMoveTime_sec, _face1, _poseOutsideFOV, TestState::ReEnroll_FaceDisappears);
      }
      break;
    }
      
    case TestState::ReEnroll_FaceDisappears:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::TimedOut)
      {
        PRINT_NAMED_INFO("CST_EnrollFace.ReEnroll_FaceDisappears.Complete",
                         "Result: %s", EnumToString(_faceEnrollmentCompletedMsg.result));
        
        PRINT_NAMED_INFO("CST_EnrollFace.ReEnroll_FaceDisappears.PuttingFaceBack", "");
        
        // Put the face back, start enrolling
        SetNodePose(_face1, _enrollmentPose);
        
        StartEnrollment(_faceID1, "Face1");
        
        // Let enrollment see the face for a few seconds, then disappear
        WaitToSetNewFacePose(3, _face1, _poseOutsideFOV, TestState::FaceReappear);
      }
      break;
    }
      
    case TestState::FaceReappear:
    {
      const s32 kMoveTime_sec = 3;
      PRINT_NAMED_INFO("CST_EnrollFace.FaceReappear", "Triggering face to reappear in %d seconds", kMoveTime_sec);
                       
      // Put the face back after a few seconds
      WaitToSetNewFacePose(kMoveTime_sec, _face1, _enrollmentPose, TestState::ReEnroll_FaceDisappearsAndReappears);
      break;
    }
  
    case TestState::ReEnroll_FaceDisappearsAndReappears:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::Success,
                                            _faceEnrollmentCompletedMsg.faceID == _faceID1,
                                            _faceEnrollmentCompletedMsg.name == "Face1")
      {
        PRINT_NAMED_INFO("CST_EnrollFace.ReEnroll_FaceDisappearsAndReappears.Complete",
                         "Result: %s", EnumToString(_faceEnrollmentCompletedMsg.result));
        
        LookDownAndTransition(TestState::NewEnroll,
                              [this]() {
                                PRINT_NAMED_INFO("CST_EnrollFace.ReEnroll_FaceDisappearsAndReappears.SwapFaces",
                                                 "Attempting to enroll face 2");
                                
                                // Move face 2 into view and move face 1 away
                                SetNodePose(_face1, _poseOutsideFOV);
                                SetNodePose(_face2, _enrollmentPose);
                                
                                // Now enroll face 2
                                const bool sayName = false;
                                StartEnrollment(Vision::UnknownFaceID, "Face2", sayName);
                              });
      }
      break;
    }
      
    case TestState::WaitForLookDown:
    {
      if(CONDITION_WITH_TIMEOUT_ASSERT(GetRobotHeadAngle_rad() < 0.f, _waitStartTime, 5))
      {
        if(nullptr != _callback)
        {
          _callback();
        }
        _testState = _nextState;
      }
      break;
    }

    case TestState::NewEnroll:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::Success,
                                            _faceEnrollmentCompletedMsg.name == "Face2",
                                            _faceEnrollmentCompletedMsg.faceID != _faceID1)
      {
        _faceID2 = _faceEnrollmentCompletedMsg.faceID;
        
        PRINT_NAMED_INFO("CST_Enroll.NewEnroll.Complete",
                         "Enrolled new face ID:%d Name:%s",
                         _faceID2, _faceEnrollmentCompletedMsg.name.c_str());
        
        LookDownAndTransition(TestState::ReEnroll_SeeKnownFace,
                              [this]() {
                                // Try to enroll face 1 again. Should fail b/c now we know face 2 and are still seeing it
                                StartEnrollment(_faceID1, "Face1");
                              });
      }
      break;
    }
      
    case TestState::ReEnroll_SeeKnownFace:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::SawWrongFace,
                                            _faceEnrollmentCompletedMsg.name == "Face2",
                                            _faceEnrollmentCompletedMsg.faceID == _faceID2)
      {
        PRINT_NAMED_INFO("CST_Enroll.ReEnroll_SeeKnownFace.Completed",
                         "Result:%s ID:%d Name:%s",
                         EnumToString(_faceEnrollmentCompletedMsg.result),
                         _faceEnrollmentCompletedMsg.faceID,
                         _faceEnrollmentCompletedMsg.name.c_str());
        
        LookDownAndTransition(TestState::ReEnroll_SeeMultipleFaces,
                              [this]() {
                                // Move face 1 back into FOV, next face 2
                                SetNodePose(_face1, _nextToPose);
                                
                                // Try to enroll face 2 again. Should fail b/c we're seeing multiple faces.
                                StartEnrollment(_faceID1, "Face1");
                              });


      }
      break;
    }
      
    case TestState::ReEnroll_SeeMultipleFaces:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(kFaceEnrollmentTimeout_sec,
                                            _faceEnrollmentCompleted,
                                            _faceEnrollmentCompletedMsg.result == FaceEnrollmentResult::SawMultipleFaces)
      {
        PRINT_NAMED_INFO("CST_Enroll.ReEnroll_SeeMultipleFaces.Completed", "Result:%s",
                         EnumToString(_faceEnrollmentCompletedMsg.result));
        
        CST_EXIT();
      }
      break;
    }
    
  } // switch(_testState)
  
  return RESULT_OK;
}
  
void CST_EnrollFace::HandleFaceEnrollmentCompleted(const ExternalInterface::FaceEnrollmentCompleted &msg)
{
  _faceEnrollmentCompleted = true;
  _faceEnrollmentCompletedMsg = msg;
}

}  // namespace Cozmo
}  // namespace Anki
