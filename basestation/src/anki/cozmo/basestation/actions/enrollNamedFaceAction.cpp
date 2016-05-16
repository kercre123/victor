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
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/vision/basestation/trackedFace.h"

#define DEBUG_ENROLL_NAMED_FACE_ACTION 1

#define USE_BACKPACK_LIGHTS 1

#if DEBUG_ENROLL_NAMED_FACE_ACTION
#  define PRINT_ENROLL_DEBUG(...) PRINT_NAMED_DEBUG(__VA_ARGS__)
#else
#  define PRINT_ENROLL_DEBUG(...)
#endif

namespace Anki {
namespace Cozmo {

  static void SetBackpackLightsHelper(Robot& robot, const ColorRGBA& color)
  {
    if(USE_BACKPACK_LIGHTS)
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
      
      // Make sure we find out if the face ID changes while we're running
      auto idChangeLambda = [this](const AnkiEvent<MessageEngineToGame>& event)
      {
        const auto & update = event.GetData().Get_RobotChangedObservedFaceID();
        if(update.oldID == _faceID) {
          PRINT_NAMED_INFO("EnrollNamedFaceAction.SignalHandler.UpdatingFaceID",
                           "Was enrolling ID=%d, changing to ID=%d",
                           _faceID, update.newID);
          _faceID = update.newID;
        }
      };
      
      _idChangeSignalHandle = _robot.GetExternalInterface()->Subscribe(MessageEngineToGameTag::RobotChangedObservedFaceID, idChangeLambda);
    }
    
  } // EnrollNamedFaceAction()
  
  EnrollNamedFaceAction::~EnrollNamedFaceAction()
  {
    Util::SafeDelete(_action);
    
    // Leave enrollment enabled
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
    
    // Turn backpack off:
    SetBackpackLightsHelper(_robot, NamedColors::BLACK);
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
          .startFcn = [this](const Vision::TrackedFace* face) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.SimpleStepOneStart", "");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            _action = new CompoundActionSequential(_robot, {
              new TurnTowardsPoseAction(_robot, face->GetHeadPose(), DEG_TO_RAD(45)),
              // TODO: Play start animation?
            });
            return RESULT_OK;
          },
          .stopFcn = [this](const Vision::TrackedFace*) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.SimpleStepOneStop", "");
            SetBackpackLightsHelper(_robot, NamedColors::BLACK);
            // TODO: _action = PlayAnimationGroupAction();
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
        
        // TODO: Add completion animations ("dings") to the sequence
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this](const Vision::TrackedFace*) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepOneFunction", "Red");
            SetBackpackLightsHelper(_robot, NamedColors::RED);
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kFwdDist_mm, kSpeed_mmps](const Vision::TrackedFace* face) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepThreeStart", "Blue");
            SetBackpackLightsHelper(_robot, NamedColors::BLUE);
            _action = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kFwdDist_mm, kSpeed_mmps),
              new TurnTowardsPoseAction(_robot, face->GetHeadPose(), DEG_TO_RAD(45)),
            });
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kBwdDist_mm, kSpeed_mmps](const Vision::TrackedFace* face) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepTwoFunction", "Green");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            _action = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kBwdDist_mm, kSpeed_mmps),
              new TurnTowardsPoseAction(_robot, face->GetHeadPose(), DEG_TO_RAD(45)),
            });
            return RESULT_OK;
          },
          .stopFcn = [this](const Vision::TrackedFace* face) {
            PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.StepFourEnd", "Black");
            SetBackpackLightsHelper(_robot, NamedColors::BLACK);
            return RESULT_OK;
          },
        });

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
    
    const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
    if(nullptr == face) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.InitCurrentStep.NullFace",
                          "Face %d does not exist in face world", _faceID);
      return RESULT_FAIL;
    }
    
    // Run the first step's start function if there is one
    if(nullptr != _seqIter->startFcn)
    {
      Result result = _seqIter->startFcn(face);
      if(RESULT_OK != result) {
        PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.StartFcnFailed", "");
        return result;
      }
    }
    
    // Disable enrollments, in case we just started an action
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::Disabled);
    ASSERT_NAMED_EVENT(_seqIter->numEnrollments > 0, "EnrollNamedFaceAction.InitCurrentStep.BadNumEnrollments",
                       "Expecting numEnrollments > 0, not %d", _seqIter->numEnrollments);
    _numEnrollmentsRequired = _seqIter->numEnrollments;
    _enrollmentCountReached = false;

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
      // Make sure enrollment is enabled if we got asked to enroll a negative (tracking) ID.
      // Hope we start enrollment and this gets updated to a positive ID via the
      // updatedID handler set up in the constructor.
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.InvalidID",
                          "Can only enroll positive IDs, not unknown/tracker ID %d. Enabling enrollment and trying again...",
                          _faceID);
      _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
      return ActionResult::RUNNING;
      
      // TODO: Do we want to just go back to failing in this case?
      //      return ActionResult::FAILURE_ABORT;
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
    
    auto reachedCountCallback = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
    {
      auto & countReached = event.GetData().Get_RobotReachedEnrollmentCount();
      if(countReached.faceID == _faceID) {
        if(countReached.count != _numEnrollmentsRequired) {
          PRINT_NAMED_WARNING("EnrollNamedFaceAction.CountReachedCallback.WrongNumCounts",
                              "Expecting completion at %d counts, not %d",
                              _numEnrollmentsRequired, countReached.count);
        }
        _enrollmentCountReached = true;
      }
    };
    
    _enrollmentCountSignalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotReachedEnrollmentCount, reachedCountCallback);
    
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
        
        _robot.GetVisionComponent().SetFaceEnrollmentMode(_seqIter->pose, _faceID, _numEnrollmentsRequired);
        _state = State::Enrolling;
        break;
      }
        
      case State::Enrolling:
      {
        // Just waiting for enrollments to come in...
        if(_enrollmentCountReached) {
          if(nullptr != _seqIter->stopFcn) {
            const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
            if(nullptr == face) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.NullFace",
                                  "Face %d does not exist in face world", _faceID);
              return ActionResult::FAILURE_ABORT;
            }
            Result result = _seqIter->stopFcn(face);
            if(RESULT_OK != result) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.StopFcnFailed", "");
              return ActionResult::FAILURE_ABORT;
            }
          }
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
        if(currentTime_ms - _lastModeChangeTime_ms > kMinTimePerEnrollMode_ms)
        {
          _lastModeChangeTime_ms = currentTime_ms;
          
          // Move to next step in the sequence
          ++_seqIter;
          
          Result stepInitResult = InitCurrentStep();
          if(RESULT_OK != stepInitResult) {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.StepInitFailed", "");
            return ActionResult::FAILURE_ABORT;
          }
        }
        break;
      }
    } // switch(_state)
    
    if(_seqIter == _enrollSequence.end()) {
      // Completed all steps!
      PRINT_ENROLL_DEBUG("EnrollNamedFaceAction.CheckIfDone.CompletedAllSteps", "");
      
      _robot.GetVisionComponent().AssignNameToFace(_faceID, _faceName);
      
      // Get the robot ready to be able to say the name the first time (really necessary?)
      _robot.GetTextToSpeechComponent().CreateSpeech(_faceName, SayTextStyle::Name_FirstIntroduction);
      
      // Save the new album to the robot.
      if(_saveToRobot) {
        _robot.GetVisionComponent().SaveFaceAlbumToRobot();
      }
      
      return ActionResult::SUCCESS;
    }

    return ActionResult::RUNNING;
    
  } // CheckIfDone()
  
  
  void EnrollNamedFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
  {
    FaceEnrollmentCompleted info;
    info.faceID = _faceID;
    info.name   = _faceName;
    completionUnion.Set_faceEnrollmentCompleted(std::move( info ));
  }
  
} // namespace Cozmo
} // namespace Anki
