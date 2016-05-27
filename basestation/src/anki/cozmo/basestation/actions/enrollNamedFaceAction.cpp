/**
 * File: enrollNamedFaceAction.cpp
 *
 * Author: Andrew Stein
 * Date:   4/24/2016
 *
 * Description: Implements an action for going through the steps to enroll a new, named face.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "enrollNamedFaceAction.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/vision/basestation/trackedFace.h"
#include "util/console/consoleInterface.h"

#define DEBUG_ENROLL_NAMED_FACE_ACTION 0

#if DEBUG_ENROLL_NAMED_FACE_ACTION
#  define PRINT_ENROLL_DEBUG(...) PRINT_NAMED_DEBUG(__VA_ARGS__)
#else
#  define PRINT_ENROLL_DEBUG(...)
#endif

namespace Anki {
namespace Cozmo {

  static std::string kIdleAnimName = "interactWithFaces_squint"; //"ag_keepAlive_eyes_01";
  
  static void SetIdleAnimName(ConsoleFunctionContextRef context) {
    kIdleAnimName = ConsoleArg_Get_String(context, "name");
  }
  
  CONSOLE_FUNC(SetIdleAnimName,             "Actions.EnrollNamedFace", const char* name);
  CONSOLE_VAR(f32, kENF_MinSoundSpace_s,    "Actions.EnrollNamedFace", 0.25f);
  CONSOLE_VAR(f32, kENF_MaxSoundSpace_s,    "Actions.EnrollNamedFace", 0.75f);
  CONSOLE_VAR(bool, kENF_UseBackpackLights, "Actions.EnrollNamedFace", true);
  CONSOLE_VAR(TimeStamp_t, kENF_TimeoutForBackpackLights_ms, "Actions.EnrollNamedFace", 250);
  
  static void SetBackpackLightsHelper(Robot& robot, const ColorRGBA& color)
  {
    if(kENF_UseBackpackLights)
    {
      std::array<u32, (size_t)LEDId::NUM_BACKPACK_LEDS> onColor, offColor, onPeriod, offPeriod, transitionOnPeriod, transitionOffPeriod;
      onColor.fill(color);
      offColor.fill(NamedColors::BLACK);
      onPeriod.fill(500);
      offPeriod.fill(100);
      transitionOnPeriod.fill(250);
      transitionOffPeriod.fill(250);
      robot.SetBackpackLights(onColor, offColor, onPeriod, offPeriod, transitionOnPeriod, transitionOffPeriod);
    }
  }
  
  EnrollNamedFaceAction::EnrollNamedFaceAction(Robot& robot, Vision::FaceID_t faceID, const std::string& name)
  : IAction(robot)
  , _faceID(faceID)
  , _faceName(name)
  { 

    if(_robot.GetExternalInterface() == nullptr) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Constructor.NullExternalInterface",
                          "Will not handle FaceID changes");
    } else {
      using namespace ExternalInterface;
      
      // Event subscription:
      // One could argue we should do this in Init(), but I want to make sure
      // we at least subscribe to the ChangedObservedFaceID messages ASAP, just in case
      // the ID we were constructed with changes before Init() is called.
      auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotReachedEnrollmentCount>();
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotChangedObservedFaceID>();
      helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedFace>();
    }
    
  } // EnrollNamedFaceAction()
  
  EnrollNamedFaceAction::~EnrollNamedFaceAction()
  {
    if(_action != nullptr)
    {
      _action->PrepForCompletion();
    }
    Util::SafeDelete(_action);
    
    // Leave enrollment enabled
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
    
    // Turn backpack off:
    SetBackpackLightsHelper(_robot, NamedColors::BLACK);
    
    // Go back to previous idle
    if(_idlePushed) {
      _robot.GetAnimationStreamer().PopIdleAnimation();
    }
  
  }
  
  static TrackFaceAction* CreateTrackAction(Robot& robot, Vision::FaceID_t faceID)
  {
    TrackFaceAction* trackAction = new TrackFaceAction(robot, faceID);
    //trackAction->SetSound("");
    // Make him make more sound while enrolling:
    trackAction->SetSoundSpacing(kENF_MinSoundSpace_s, kENF_MaxSoundSpace_s);
    trackAction->SetMinPanAngleForSound(DEG_TO_RAD(5));
    trackAction->SetMinTiltAngleForSound(DEG_TO_RAD(5));
    return trackAction;
  }
  
  static IActionRunner* CreateTurnTowardsFaceAction(Robot& robot, Vision::FaceID_t faceID)
  {
    IActionRunner* action = nullptr;
    
    const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(faceID);
    if(nullptr != face) {
      action = new TurnTowardsPoseAction(robot, face->GetHeadPose(), DEG_TO_RAD_F32(45.f));
    }
    else {
      // Couldn't find face in face world, try turning towards last face pose
      PRINT_NAMED_INFO("EnrollNamedFaceAction.CreateTurnTowardsFaceAction.NullFace",
                       "No face with ID=%d in FaceWorld. Using TurnTowardsLastFacePose",
                       faceID);
      action = new TurnTowardsLastFacePoseAction(robot, DEG_TO_RAD_F32(45.f));
    }
    
    return action;
  }
  
  Result EnrollNamedFaceAction::InitSequence()
  {
    switch(_whichSeq)
    {
      case FaceEnrollmentSequence::Simple:
      {
        // Look at face, sit still and collect N enrollments
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 6,
          .startFcn = [this]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.SimpleStepOneStart", "");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            _action = CreateTurnTowardsFaceAction(_robot, _faceID);
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            _action = CreateTrackAction(_robot, _faceID);
            return RESULT_OK;
          },
          .stopFcn = [this]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.SimpleStepOneStop", "");
            SetBackpackLightsHelper(_robot, NamedColors::BLACK);
            return RESULT_OK;
          },
        });
        
        break;
      }
        
      case FaceEnrollmentSequence::BackAndForth:
      {
        // TODO: Expose:
        const f32 kFwdDist_mm    = 100.f;
        const f32 kSpeed_mmps    = 75.f;
        const f32 kBwdDist_mm    = -50.f;
        
        _enrollSequence.reserve(3);
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepOneFunction", "Red");
            SetBackpackLightsHelper(_robot, NamedColors::RED);
            _action = CreateTurnTowardsFaceAction(_robot, _faceID);
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kFwdDist_mm, kSpeed_mmps]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepThreeStart", "Blue");
            SetBackpackLightsHelper(_robot, NamedColors::BLUE);
            CompoundActionSequential* compoundAction = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kFwdDist_mm, kSpeed_mmps)
            });
            
            const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
            if(nullptr != face) {
              compoundAction->AddAction(new TurnTowardsPoseAction(_robot, face->GetHeadPose(), DEG_TO_RAD(45)));
            }
            
            _action = compoundAction;
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            _action = CreateTrackAction(_robot, _faceID);
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kBwdDist_mm, kSpeed_mmps]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepTwoFunction", "Green");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            CompoundActionSequential* compoundAction = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kBwdDist_mm, kSpeed_mmps)
            });
            
            const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
            if(nullptr != face) {
              compoundAction->AddAction(new TurnTowardsPoseAction(_robot, face->GetHeadPose(), DEG_TO_RAD(45)));
            }
            
            _action = compoundAction;
            
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            _action = CreateTrackAction(_robot, _faceID);
            return RESULT_OK;
          },
          .stopFcn = [this]() {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepFourEnd", "Black");
            SetBackpackLightsHelper(_robot, NamedColors::BLACK);
            return RESULT_OK;
          },
        });

        break;
      }
        
      case FaceEnrollmentSequence::Immediate:
      {
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = -1, // Don't count enrollments: use whatever we already have
          .startFcn = [this]() {
            _action = CreateTurnTowardsFaceAction(_robot, _faceID);
            return RESULT_OK;
          },
          .duringFcn = {},
          .stopFcn = {},
        });
        _enrollmentCountReached = true;
        _minTimePerEnrollStep_ms = 0;
        break;
      }
        
      default:
        PRINT_NAMED_WARNING("EnrollNamedFaceAction.SetSequence.UnknownSequence", "");
        return RESULT_FAIL;
    } // switch(seq)
    
    return RESULT_OK;
  } // InitSequence()
  
  
  Result EnrollNamedFaceAction::InitCurrentStep()
  {
    if(_seqIter == _enrollSequence.end()) {
      // At end, nothing to init
      return RESULT_OK;
    }
    
    // Run the first step's start function if there is one
    if(nullptr != _seqIter->startFcn)
    {
      Result result = _seqIter->startFcn();
      if(RESULT_OK != result) {
        PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.StartFcnFailed", "");
        return result;
      }
    }
    
    // Disable enrollments, in case we just started an action
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::Disabled);
    ASSERT_NAMED_EVENT(_seqIter->numEnrollments != 0, "EnrollNamedFaceAction.InitCurrentStep.BadNumEnrollments",
                       "Expecting numEnrollments != 0, not %d", _seqIter->numEnrollments);
    _numEnrollmentsRequired = _seqIter->numEnrollments;
    if(_numEnrollmentsRequired > 0) {
      _enrollmentCountReached = false;
    }

    _state = State::PreActing;
    
    PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.InitCurrentStep",
                       "FaceID=%d, EnrollCount=%d",
                       _faceID, _numEnrollmentsRequired);
    
    return RESULT_OK;
  } // InitCurrentStep()
  
  
  ActionResult EnrollNamedFaceAction::Init()
  {
    PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.Init", "");
    
    if(_faceID <= 0) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.InvalidID",
                          "Can only enroll positive IDs, not unknown/tracker ID %d",
                          _faceID);
      return ActionResult::FAILURE_ABORT;
    }

    Result initSeqResult = InitSequence();
    if(RESULT_OK != initSeqResult) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.InitSequenceFail",
                          "Requested sequence: %s", EnumToString(_whichSeq));
      return ActionResult::FAILURE_ABORT;
    }
    
    ASSERT_NAMED(!_enrollSequence.empty(), "EnrollNamedFaceAction.Init.EmptyEnrollSequence");
    
    _seqIter = _enrollSequence.begin();
    _lastModeChangeTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
    Result stepInitResult = InitCurrentStep();
    if(RESULT_OK != stepInitResult) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.StepInitFailed", "");
      return ActionResult::FAILURE_ABORT;
    }
    
    return ActionResult::SUCCESS;
  } // Init()
  
  
  ActionResult EnrollNamedFaceAction::CheckIfDone()
  {
    switch(_state)
    {
      case State::PreActing:
      {
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::SUCCESS == subResult) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.PreActionCompleted", "");
            Util::SafeDelete(_action); // delete and set to null to signify we're done with this action
          } else {
            return subResult; // NOTE: if action failed, it will still get deleted in destructor
          }
        }
        
        PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.SetEnrollmentMode",
                           "pose:%s ID:%d enrollCount:%d",
                           EnumToString(_seqIter->pose), _faceID, _numEnrollmentsRequired);
        
        _robot.GetVisionComponent().SetFaceEnrollmentMode(_seqIter->pose, _faceID, _numEnrollmentsRequired);
        if(_seqIter->duringFcn != nullptr)
        {
          PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.StartingDuringAction", "");
          Result duringResult = _seqIter->duringFcn();
          if(RESULT_OK != duringResult) {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.DuringFcnFailed", "");
            return ActionResult::FAILURE_ABORT;
          }
        }
        
        PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.TransitionToEnrolling", "");
        _state = State::Enrolling;
        break;
      }
        
      case State::Enrolling:
      {
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::RUNNING != subResult) {
            if(ActionResult::SUCCESS != subResult) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.DuringActionFailed", "");
            }
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.PreActionCompleted", "");
            Util::SafeDelete(_action); // delete and set to null to signify we're done with this action
          } else {
            //PRINT_NAMED_DEBUG("EnrollNamedFaceAction.CheckIfDone.DuringActionRunning", "");
          }
        }
        
        if(!_idlePushed) {
          _robot.GetAnimationStreamer().PushIdleAnimation(kIdleAnimName);
          _idlePushed = true;
        }
        
        // Make the backpack lights turn off if we can't see the person
        auto lastImageTime = _robot.GetLastImageTimeStamp();
        if(lastImageTime - _lastFaceSeen_ms > kENF_TimeoutForBackpackLights_ms)
        {
          _lastFaceSeen_ms = 0;
          SetBackpackLightsHelper(_robot, NamedColors::BLACK);
        }
        
        // Just waiting for enrollments to come in...
        if(_enrollmentCountReached)
        {
          PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.ReachedEnrollmentCount",
                             "Count=%d", _numEnrollmentsRequired);
          
          // Cancel any "during" action we were doing
          Util::SafeDelete(_action);
          
          if(nullptr != _seqIter->stopFcn)
          {
            Result result = _seqIter->stopFcn();
            if(RESULT_OK != result) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.StopFcnFailed", "");
              return ActionResult::FAILURE_ABORT;
            }
          }
          
          PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.TransitionToPostActing", "");
          _state = State::PostActing;
        }
        break;
      }
        
      case State::PostActing:
      {
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::SUCCESS == subResult) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.PostActionCompleted", "");
            Util::SafeDelete(_action);
          } else {
            return subResult;
          }
        }
        
        const TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        if(currentTime_ms - _lastModeChangeTime_ms > _minTimePerEnrollStep_ms)
        {
          PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.DoneWithCurrentStep", "");
          _lastModeChangeTime_ms = currentTime_ms;
          
          // Move to next step in the sequence
          ++_seqIter;
          
          if(_seqIter == _enrollSequence.end())
          {
            // Completed all steps!
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.CompletedAllSteps", "");
            
            _robot.GetVisionComponent().AssignNameToFace(_faceID, _faceName);
            
            // Play success animation which says player's name
            SayTextAction* sayTextAction = new SayTextAction(_robot, _faceName, SayTextStyle::Name_Normal, false);
            sayTextAction->SetGameEvent(GameEvent::OnLearnedPlayerName);
            _action = sayTextAction;
            
            //  // Test: trying saying name twice
            //  SayTextAction* sayTextAction2 = new SayTextAction(_robot, _faceName, SayTextStyle::Name_Normal, false);
            //  sayTextAction2->SetGameEvent(GameEvent::OnLearnedPlayerName);
            //  _action = new CompoundActionSequential(_robot, {sayTextAction, sayTextAction2});
            
            //  // Test: Play happy animation after say text
            //  _action = new CompoundActionSequential(_robot, {sayTextAction,
            //    new PlayAnimationAction(_robot, "anim_reactToBlock_success_01_45"),
            //    new MoveHeadToAngleAction(_robot, DEG_TO_RAD(40))
            //  });
            
            // Save the new album to the robot.
            if(_saveToRobot) {
              _robot.GetVisionComponent().SaveFaceAlbumToRobot();
            }
            _state = State::Finishing;
            
            break;
          }
          
          Result stepInitResult = InitCurrentStep();
          if(RESULT_OK != stepInitResult) {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.StepInitFailed", "");
            return ActionResult::FAILURE_ABORT;
          }
        } else {
          PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.WaitingForMinTimerPerStep",
                             "CurrentTime=%d LastChange=%d MinTime=%d",
                             currentTime_ms, _lastModeChangeTime_ms, _minTimePerEnrollStep_ms);
        }
        
        break;
      }
    
      case State::Finishing:
      {
        if(nullptr != _action)
        {
          // Finish the final success SayTextAction. Once it completes, this action
          // will return SUCCESS.
          return _action->Update();
        } else {
          return ActionResult::SUCCESS;
        }
      }
        
    } // switch(_state)

    return ActionResult::RUNNING;
    
  } // CheckIfDone()
  
  
  void EnrollNamedFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
  {
    FaceEnrollmentCompleted info;
    info.faceID = _faceID;
    info.name   = _faceName;
    completionUnion.Set_faceEnrollmentCompleted(std::move( info ));
  }
  
  
#pragma mark -
#pragma mark Event Handlers
  
  template<>
  void EnrollNamedFaceAction::HandleMessage(const ExternalInterface::RobotReachedEnrollmentCount& msg)
  {
    if(msg.faceID == _faceID) {
      if(msg.count != _numEnrollmentsRequired) {
        PRINT_NAMED_WARNING("EnrollNamedFaceAction.HandleRobotReachedEnrollmentCount.WrongNumCounts",
                            "Expecting completion at %d counts, not %d",
                            _numEnrollmentsRequired, msg.count);
      }
      _enrollmentCountReached = true;
    }
  }
  
  template<>
  void EnrollNamedFaceAction::HandleMessage(const ExternalInterface::RobotChangedObservedFaceID& msg)
  {
    if(msg.oldID == _faceID) {
      PRINT_NAMED_INFO("EnrollNamedFaceAction.HandleRobotChangedObservedFaceID.UpdatingFaceID",
                       "Was enrolling ID=%d, changing to ID=%d",
                       _faceID, msg.newID);
      _faceID = msg.newID;
    }
  }
  
  template<>
  void EnrollNamedFaceAction::HandleMessage(const ExternalInterface::RobotObservedFace& msg)
  {
    if(msg.faceID == _faceID)
    {
      if(_lastFaceSeen_ms == 0) {
        SetBackpackLightsHelper(_robot, NamedColors::GREEN);
      }
      
      _lastFaceSeen_ms = msg.timestamp;
    }
  }
  
  
  
} // namespace Cozmo
} // namespace Anki
