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
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/components/lightsComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/vision/basestation/trackedFace.h"

#include "util/console/consoleInterface.h"

#include "clad/types/enrolledFaceStorage.h"

#include <cmath>

namespace Anki {
namespace Cozmo {

  namespace {
    // Use backpack lights to help debug when enrollment is occurring or not.
    CONSOLE_VAR(bool,              kUseBackpackLights,                   "Actions.EnrollNamedFace", false);
    CONSOLE_VAR(TimeStamp_t,       kTimeoutForReLookForFace_ms,          "Actions.EnrollNamedFace", 1500);
    
    // Thresholds for when to update face ID based on pose
    CONSOLE_VAR(f32,               kUpdateFacePositionThreshold_mm,      "Actions.EnrollNamedFace", 100.f);
    CONSOLE_VAR(f32,               kUpdateFaceAngleThreshold_deg,        "Actions.EnrollNamedFace", 45.f);
    
    // Default timeout for this action (vs. the one inherited from IAction)
    CONSOLE_VAR(f32,               kFaceEnrollmentTimeout_sec,           "Actions.EnrollNamedFace", 15.f);
    CONSOLE_VAR(f32,               kFaceEnrollmentTimeoutMax_sec,        "Actions.EnrollNamedFace", 35.f);
    
    // Amount of "extra" time to add each time we re-start actually enrolling, in case we lose the face
    // mid way or take a while to initially find the face, up to the max timeout
    CONSOLE_VAR(f32,               kFaceEnrollmentTimeoutExtraTime_sec,  "Actions.EnrollNamedFace", 8.f);
    
    // Amount to drive forward once face is found to signify intent
    CONSOLE_VAR(f32,               kDriveForwardIntentDist_mm,           "Actions.EnrollNamedFace", 14.f);
    CONSOLE_VAR(f32,               kDriveForwardIntentSpeed_mmps,        "Actions.EnrollNamedFace", 75.f);
    
    // Minimum angles to turn during tracking to keep the robot moving and looking alive
    CONSOLE_VAR(f32,               kMinTrackingPanAngle_deg,             "Actions.EnrollNamedFace", 5.0f);
    CONSOLE_VAR(f32,               kMinTrackingTiltAngle_deg,            "Actions.EnrollNamedFace", 4.0f);
    
    const char* kLogChannelName = "FaceRecognizer";
  }
  
  static void SetBackpackLightsHelper(Robot& robot, const ColorRGBA& color)
  {
    if(kUseBackpackLights)
    {
      const BackpackLights lights = {
        .onColors               = {{color,color,color,color,color}},
        .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
        .onPeriod_ms            = {{500,500,500,500,500}},
        .offPeriod_ms           = {{100,100,100,100,100}},
        .transitionOnPeriod_ms  = {{250,250,250,250,250}},
        .transitionOffPeriod_ms = {{250,250,250,250,250}},
        .offset                 = {{0,0,0,0,0}}
      };
      robot.GetLightsComponent().SetBackpackLights(lights);
    }
  }
  
  EnrollNamedFaceAction::EnrollNamedFaceAction(Robot& robot, Vision::FaceID_t faceID, const std::string& name,
                                               Vision::FaceID_t saveID)
  : IAction(robot,
            "EnrollNamedFace",
            RobotActionType::ENROLL_NAMED_FACE,
            (u8)AnimTrackFlag::NO_TRACKS)
  , _faceID(faceID)
  , _saveID(saveID)
  , _faceName(name)
  , _timeout_sec(kFaceEnrollmentTimeout_sec)
  , _enrollingSpecificID(_faceID != Vision::UnknownFaceID)
  {
    PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.Constructor", "Original ID=%d with name='%s'",
                  _faceID, Util::HidePersonallyIdentifiableInfo(_faceName.c_str()));
    
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
      helper.SubscribeEngineToGame<MessageEngineToGameTag::UnexpectedMovement>();
    }
    
  } // EnrollNamedFaceAction()
  
  EnrollNamedFaceAction::~EnrollNamedFaceAction()
  {
    SetAction( nullptr ) ;
    
    // Leave enrollment enabled
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
    
    // Turn backpack off:
    SetBackpackLightsHelper(_robot, NamedColors::BLACK);
    
    // NOTE: we are potentially leaving the scanning idle in place here, for Unity to pop
    //  This grossness should go away when we make this action into a behavior (COZMO-4138)
    //    // Go back to previous idle if we haven't already
    //    if(!_idlePopped) {
    //      _robot.GetAnimationStreamer().PopIdleAnimation();
    //    }
  
    _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, true);
    _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, true);
  }
  
  static TrackFaceAction* CreateTrackAction(Robot& robot, Vision::FaceID_t faceID)
  {
    TrackFaceAction* trackAction = new TrackFaceAction(robot, faceID);
    
    // Disable tracking sounds because we have a face enrollment idle animation
    // which has special sound
    trackAction->SetSound(AnimationTrigger::Count);
    
    // Add constant small movement
    trackAction->SetTiltTolerance(DEG_TO_RAD(kMinTrackingTiltAngle_deg));
    trackAction->SetPanTolerance(DEG_TO_RAD(kMinTrackingPanAngle_deg));
    trackAction->SetClampSmallAnglesToTolerances(true);
    
    return trackAction;
  }
  
  static IActionRunner* CreateTurnTowardsFaceAction(Robot& robot, Vision::FaceID_t faceID, Vision::FaceID_t saveID)
  {
    IActionRunner* action = nullptr;
    
    if(faceID != Vision::UnknownFaceID)
    {
      // Try to look at the specified face
      const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(faceID);
      if(nullptr != face) {
        PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.CreateTurnTowardsFaceAction.TurningTowardsFaceID",
                      "Turning towards faceID=%d (saveID=%d)",
                      faceID, saveID);
        
        action = new TurnTowardsFaceAction(robot, faceID, DEG_TO_RAD(45.f));
      } else {

      }
    }
    else
    {
      // We weren't given a specific FaceID, so grab the last observed *unnamed*
      // face ID from FaceWorld and look at it (if there is one). If saveID was
      // specified, check first to see it's present in FaceWorld and turn towards
      // it (since that's who we are re-enrolling).
      const Vision::TrackedFace* faceToTurnTowards = nullptr;
      if(saveID != Vision::UnknownFaceID )
      {
        faceToTurnTowards = robot.GetFaceWorld().GetFace(saveID);
      }
      
      if(faceToTurnTowards == nullptr)
      {
        auto allFaceIDs = robot.GetFaceWorld().GetKnownFaceIDs();
        for(auto & ID : allFaceIDs)
        {
          const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(ID);
          const bool isFirstOrMoreRecent = (faceToTurnTowards == nullptr || face->GetTimeStamp() > faceToTurnTowards->GetTimeStamp());
          if(!face->HasName() && isFirstOrMoreRecent)
          {
            faceToTurnTowards = face;
          }
        }
      }
      
      if(nullptr != faceToTurnTowards)
      {
        // Couldn't find face in face world, try turning towards last face pose
        PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.CreateTurnTowardsFaceAction.FoundFace",
                      "Turning towards faceID=%d last seen at t=%d (saveID=%d)",
                      faceToTurnTowards->GetID(), faceToTurnTowards->GetTimeStamp(), saveID);
        
        action = new TurnTowardsFaceAction(robot, faceToTurnTowards->GetID(), DEG_TO_RAD(90.f));
      }
    }
    
    if(action == nullptr)
    {
      // Couldn't find face in face world, try turning towards last face pose
      PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.CreateTurnTowardsFaceAction.NullFace",
                    "No face found to turn towards. FaceID=%d. SaveID=%d. Turning towards last face pose.",
                    faceID, saveID);
      
      // No face found to look towards: fallback on looking at last face pose
      action = new TurnTowardsLastFacePoseAction(robot, DEG_TO_RAD(45.f));
    }
    
    CompoundActionParallel* liftAndTurnTowardsAction = new CompoundActionParallel(robot, {
      new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK),
      action
    });
    
    return liftAndTurnTowardsAction;
  }
  
  IActionRunner* EnrollNamedFaceAction::CreateLookAroundAction()
  {
    // If we haven't seen the face since this behavior was created,
    // try looking up further: it's more likely a face is further up and
    // we're looking too low. Add a little movement so he doesn't look dead.
    // NOTE: we will just keep doing this until timeout if we never see the face!
    const Radians absHeadAngle = GetRNG().RandDblInRange(MAX_HEAD_ANGLE - DEG_TO_RAD(10), MAX_HEAD_ANGLE);
    
    // Rotate in the opposite direction enough to undo the last rotation plus a little more
    const double newAngle = std::copysign(GetRNG().RandDblInRange(0, DEG_TO_RAD(10)),
                                          -_lastRelBodyAngle.ToDouble());
    const Radians relBodyAngle = newAngle -_lastRelBodyAngle;
    _lastRelBodyAngle = newAngle;
    
    CompoundActionSequential* lookAroundAction = new CompoundActionSequential(_robot, {
      new PanAndTiltAction(_robot, relBodyAngle, absHeadAngle, false, true),
    });
    
    // Also back up a little if we haven't gone too far back already
    if(_totalBackup_mm <= _kMaxTotalBackup_mm) {
      const f32 backupSpeed_mmps = 100.f;
      const f32 backupDist_mm = GetRNG().RandDblInRange(_kMinBackup_mm, _kMaxBackup_mm);
      _totalBackup_mm += backupDist_mm;
      const bool shouldPlayAnimation = false; // don't want head to move down!
      DriveStraightAction* backUpAction = new DriveStraightAction(_robot, -backupDist_mm, backupSpeed_mmps, shouldPlayAnimation);
      lookAroundAction->AddAction(backUpAction);
    }
    
    lookAroundAction->AddAction(new WaitForImagesAction(_robot, _kNumImagesToWait, VisionMode::DetectingFaces));
    
    return lookAroundAction;
  }
  
  void EnrollNamedFaceAction::SetAction(IActionRunner* action)
  {
    if(_action != nullptr) {
      _action->PrepForCompletion();
      Util::SafeDelete(_action);
    }
    _action = action;
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
          .numEnrollments = (s32)Vision::FaceRecognitionConstants::MaxNumEnrollDataPerAlbumEntry,
          .startFcn = [this]() {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.SimpleStepOneStart", "");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            SetAction(new CompoundActionSequential(_robot, {
              new TurnTowardsFaceAction(_robot, _faceID, M_PI, false),
              new CompoundActionParallel(_robot, {
                new TriggerAnimationAction(_robot, AnimationTrigger::MeetCozmoLookFaceGetIn),
                new DriveStraightAction(_robot, kDriveForwardIntentDist_mm, kDriveForwardIntentSpeed_mmps, false)
              })
            }));
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            SetAction( CreateTrackAction(_robot, _faceID) );
            if(!_idlePopped) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.SimpleStepOneDuring.IdleNotPopped", "");
            } else {
              _robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::MeetCozmoScanningIdle);
            }
            _idlePopped = false;
            return RESULT_OK;
          },
          .stopFcn = [this]() {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.SimpleStepOneStop", "");
            SetBackpackLightsHelper(_robot, NamedColors::BLACK);
            // if(!_idlePopped)
            // {
            //  _robot.GetAnimationStreamer().PopIdleAnimation();
            //  _idlePopped = true;
            // }
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
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.StepOneFunction", "Red");
            SetBackpackLightsHelper(_robot, NamedColors::RED);
            SetAction( CreateTurnTowardsFaceAction(_robot, _faceID, _saveID) );
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kFwdDist_mm, kSpeed_mmps]() {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.StepThreeStart", "Blue");
            SetBackpackLightsHelper(_robot, NamedColors::BLUE);
            CompoundActionSequential* compoundAction = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kFwdDist_mm, kSpeed_mmps),
              new TurnTowardsFaceAction(_robot, _faceID, DEG_TO_RAD(45)),
            });
            
            SetAction( compoundAction );
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            SetAction( CreateTrackAction(_robot, _faceID) );
            return RESULT_OK;
          },
        });
        
        _enrollSequence.emplace_back(EnrollStep{
          .pose = Vision::FaceEnrollmentPose::LookingStraight,
          .numEnrollments = 2,
          .startFcn = [this, kBwdDist_mm, kSpeed_mmps]() {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.StepTwoFunction", "Green");
            SetBackpackLightsHelper(_robot, NamedColors::GREEN);
            CompoundActionSequential* compoundAction = new CompoundActionSequential(_robot, {
              new DriveStraightAction(_robot, kBwdDist_mm, kSpeed_mmps),
              new TurnTowardsFaceAction(_robot, _faceID, DEG_TO_RAD(45)),
            });
            
            SetAction( compoundAction );
            
            return RESULT_OK;
          },
          .duringFcn = [this]() {
            SetAction( CreateTrackAction(_robot, _faceID) );
            return RESULT_OK;
          },
          .stopFcn = [this]() {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.StepFourEnd", "Black");
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
            SetAction( CreateTurnTowardsFaceAction(_robot, _faceID, _saveID) );
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
    
    PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.InitCurrentStep",
                   "FaceID=%d, EnrollCount=%d",
                   _faceID, _numEnrollmentsRequired);
    
    return RESULT_OK;
  } // InitCurrentStep()
  
  
  ActionResult EnrollNamedFaceAction::Init()
  {
    PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.Init",
                  "Initialize with ID=%d and name '%s', to be saved to ID=%d",
                  _faceID, Util::HidePersonallyIdentifiableInfo(_faceName.c_str()), _saveID);


    // TOOD: If saveID is specified, make sure that it is the ID of a _named_ face
   
    // AcknowledgeFace, if enabled, can and generally will interupt this action as soon
    // as enrollment finishes, which we don't want. So disable it here.
    // We re-enable in the destructor.
    _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::AcknowledgeFace, false);
    
    // Same goes for ReactToUnexpectedMovement: we want people to be able to manually re-orient
    // Cozmo to face them without him registering unexpected movement and cancelling
    // this Enroll action
    _robot.GetBehaviorManager().RequestEnableReactionaryBehavior(GetName(), BehaviorType::ReactToUnexpectedMovement, false);
    
    Result initSeqResult = InitSequence();
    if(RESULT_OK != initSeqResult) {
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.Init.InitSequenceFail",
                          "Requested sequence: %s", EnumToString(_whichSeq));
      return ActionResult::FAILURE_ABORT;
    }
    
    ASSERT_NAMED(!_enrollSequence.empty(), "EnrollNamedFaceAction.Init.EmptyEnrollSequence");
    
    // First thing we want to do is turn towards the face and make sure we see it
    TransitionToLookingForFace();
    
    // Start with this timeout (do this here in addition to using the constructor's
    // initializaion list in case there's a Reset)
    _timeout_sec = kFaceEnrollmentTimeout_sec;
    
    return ActionResult::SUCCESS;
  } // Init()
  
  void EnrollNamedFaceAction::TransitionToLookingForFace()
  {
    // In case we were enrolling and lost face, cancel tracking action
    SetAction(nullptr);
    
    _state = State::LookForFace;
    _action = new CompoundActionSequential(_robot, {
      CreateTurnTowardsFaceAction(_robot, _faceID, _saveID),
      new WaitForImagesAction(_robot, _kNumImagesToWait, VisionMode::DetectingFaces),
    });
    
    // If we are going _back_ to looking for a face after having been enrolling
    // (and lost the face), we were in scanning idle and need to pop it here
    if(!_idlePopped)
    {
      _robot.GetAnimationStreamer().PopIdleAnimation();
      _idlePopped = true;
    }
  }
  
  ActionResult EnrollNamedFaceAction::CheckIfDone()
  {
    if(_needToAbort)
    {
      // A handler told us to abort
      return ActionResult::FAILURE_ABORT;
    }
    
    switch(_state)
    {
      case State::LookForFace:
      {
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::RUNNING != subResult)
          {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.TurnTowardsFaceCompleted", "");
            
            if(_lastFaceSeen_ms == 0) {
              SetAction(CreateLookAroundAction());
              return ActionResult::RUNNING;
            }
            
            // Otherwise, we've seen the face so continue with starting enrollment sequence
            SetAction(nullptr);
            _seqIter = _enrollSequence.begin();
            _lastModeChangeTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
            
            // Give ourselves a little more time to finish now that we've seen a face, but
            // don't go over the max timeout
            _timeout_sec = std::min(kFaceEnrollmentTimeoutMax_sec, _timeout_sec + kFaceEnrollmentTimeoutExtraTime_sec);
            
            PRINT_NAMED_INFO("EnrollNamedFaceAction.CheckIfDone.FaceSeen",
                             "Found face %d to enroll. Timeout set to %.1fsec",
                             _faceID, _timeout_sec);
            
            Result stepInitResult = InitCurrentStep(); // Transitions us to PreActing
            if(RESULT_OK != stepInitResult) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.StepInitFailed", "");
              return ActionResult::FAILURE_ABORT;
            }
            
          }
          else if(ActionResult::RUNNING != subResult)
          {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.LookAtFaceStateFailure", "");
            return subResult;
          }
          break;
        }
        
        // There's no action for some reason, just switch states and fall through immediately
        _state = State::PreActing;
      }
        
      // NOTE that we expect a valid (positive) face ID to be set if we enter any of the
      // remaining states
        
      case State::PreActing:
      {
        if(_faceID <= Vision::UnknownFaceID) {
          PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.InvalidFaceID_PreActing", "");
          return ActionResult::FAILURE_ABORT;
        }
        
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::SUCCESS == subResult) {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.PreActionCompleted", "");
            SetAction( nullptr );
          } else {
            return subResult; // NOTE: if action failed, it will still get deleted in destructor
          }
        }
        
        PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.SetEnrollmentMode",
                       "pose:%s ID:%d enrollCount:%d",
                       EnumToString(_seqIter->pose), _faceID, _numEnrollmentsRequired);
        
        _robot.GetVisionComponent().SetFaceEnrollmentMode(_seqIter->pose, _faceID, _numEnrollmentsRequired);
        if(_seqIter->duringFcn != nullptr)
        {
          PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.StartingDuringAction", "");
          Result duringResult = _seqIter->duringFcn();
          if(RESULT_OK != duringResult) {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.DuringFcnFailed", "");
            return ActionResult::FAILURE_ABORT;
          }
        }
        
        PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.TransitionToEnrolling", "");
        _state = State::Enrolling;
        break;
      }
        
      case State::Enrolling:
      {
        if(_faceID <= Vision::UnknownFaceID) {
          PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.InvalidFaceID_Enrolling", "");
          return ActionResult::FAILURE_ABORT;
        }
        
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::RUNNING != subResult) {
            if(ActionResult::SUCCESS != subResult) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.DuringActionFailed", "");
            }
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.DuringCompleted", "");
            SetAction( nullptr );
          } else {
            //PRINT_NAMED_DEBUG("EnrollNamedFaceAction.CheckIfDone.DuringActionRunning", "");
          }
        }
        
        // If we haven't seen the person in too long, go back to looking for them
        auto lastImageTime = _robot.GetLastImageTimeStamp();
        if(lastImageTime - _lastFaceSeen_ms > kTimeoutForReLookForFace_ms)
        {
          _lastFaceSeen_ms = 0;
          SetBackpackLightsHelper(_robot, NamedColors::BLACK);
          
          // Go back to looking for face
          PRINT_NAMED_INFO("EnrollNamedFaceAction.CheckIfDone.NoLongerSeeingFace",
                           "Have not seen face %d in %dms, going back to LookForFace state for %s",
                           _faceID, kTimeoutForReLookForFace_ms,
                           _enrollingSpecificID ? ("face " + std::to_string(_faceID)).c_str() : "any face");
          
          // If we are not enrolling a specific face ID, we are allowed to try again
          // with a new face, so don't hang waiting to see the one we previously picked
          if(!_enrollingSpecificID)
          {
            _faceID = Vision::UnknownFaceID;
          }
          
          TransitionToLookingForFace();
          break;
        }
        
        // Just waiting for enrollments to come in...
        if(_enrollmentCountReached)
        {
          PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.ReachedEnrollmentCount",
                         "Count=%d", _numEnrollmentsRequired);
          
          // Cancel any "during" action we were doing
          SetAction( nullptr );
          
          if(nullptr != _seqIter->stopFcn)
          {
            Result result = _seqIter->stopFcn();
            if(RESULT_OK != result) {
              PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.StopFcnFailed", "");
              return ActionResult::FAILURE_ABORT;
            }
          }
          
          PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.TransitionToPostActing", "");
          _state = State::PostActing;
        }
        
        break;
      }
        
      case State::PostActing:
      {
        if(_faceID <= Vision::UnknownFaceID) {
          PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.InvalidFaceID_PostActing", "");
          return ActionResult::FAILURE_ABORT;
        }
        
        if(_action != nullptr)
        {
          ActionResult subResult = _action->Update();
          if(ActionResult::SUCCESS == subResult) {
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.PostActionCompleted", "");
            SetAction( nullptr );
          } else {
            return subResult;
          }
        }
        
        const TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        if(currentTime_ms - _lastModeChangeTime_ms > _minTimePerEnrollStep_ms)
        {
          PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.DoneWithCurrentStep", "");
          _lastModeChangeTime_ms = currentTime_ms;
          
          // Move to next step in the sequence
          ++_seqIter;
          
          if(_seqIter == _enrollSequence.end())
          {
            // Completed all steps!
            PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.CompletedAllSteps", "");
            
            _robot.GetVisionComponent().AssignNameToFace(_faceID, _faceName, _saveID);
            
            if(_sayNameWhenDone)
            {
              // Play success animation which says player's name
              // TODO: this should probably all be handled by Unity on successful completion of this action
              
              if(!_idlePopped)
              {
                _robot.GetAnimationStreamer().PopIdleAnimation();
                _idlePopped = true;
              }
              
              // Get out of the scanning face
              TriggerAnimationAction* getOutAction = new TriggerAnimationAction(_robot, AnimationTrigger::MeetCozmoLookFaceGetOut);
              
              IActionRunner* finalAnimation = nullptr;
              if(_saveID == Vision::UnknownFaceID)
              {
                // If we're not being told which ID to save to, then assume this is a
                // first-time enrollment and play the bigger sequence of animations,
                // along with special music state
                // TODO: PostMusicState should take in a GameState::Music, making the cast unnecessary...
                _robot.GetRobotAudioClient()->PostMusicState((Audio::GameState::GenericState)Audio::GameState::Music::Minigame__Meet_Cozmo_Say_Name, false, 0);
                
                // 1. Say name once
                SayTextAction* sayNameAction1 = new SayTextAction(_robot, _faceName, SayTextIntent::Name_FirstIntroduction_1);
                sayNameAction1->SetAnimationTrigger(AnimationTrigger::MeetCozmoFirstEnrollmentSayName);
                
                // 2. Repeat name
                SayTextAction* sayNameAction2 = new SayTextAction(_robot, _faceName, SayTextIntent::Name_FirstIntroduction_2);
                sayNameAction2->SetAnimationTrigger(AnimationTrigger::MeetCozmoFirstEnrollmentRepeatName);
                
                // 3. Big celebrate (no name being said)
                TriggerAnimationAction* celebrateAction = new TriggerAnimationAction(_robot, AnimationTrigger::MeetCozmoFirstEnrollmentCelebration);
                
                finalAnimation = new CompoundActionSequential(_robot, {getOutAction, sayNameAction1, sayNameAction2, celebrateAction});
              }
              else
              {
                // This is a re-enrollment, so do the more subdued animation
                SayTextAction* sayNameAction = new SayTextAction(_robot, _faceName, SayTextIntent::Name_Normal);
                sayNameAction->SetAnimationTrigger(AnimationTrigger::MeetCozmoReEnrollmentSayName);
                
                finalAnimation = new CompoundActionSequential(_robot, {getOutAction, sayNameAction});
              }
              
              SetAction( finalAnimation );
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
          PRINT_CH_DEBUG(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.WaitingForMinTimerPerStep",
                         "CurrentTime=%d LastChange=%d MinTime=%d",
                         currentTime_ms, _lastModeChangeTime_ms, _minTimePerEnrollStep_ms);
        }
        
        break;
      }
    
      case State::Finishing:
      {
        ActionResult result = ActionResult::SUCCESS;
        
        // Finish the final success SayTextAction, if it exists
        if(nullptr != _action)
        {
          result = _action->Update();
        }

        // Save the new album to the robot here, only once we are about to return SUCCESS,
        // to ensure that the face gets saved if and only if the action reports success.
        if(ActionResult::SUCCESS == result && _saveToRobot)
        {
          std::function<void(NVStorage::NVResult)> saveEnrollCallback = [this](NVStorage::NVResult result)
          {
            if(result == NVStorage::NVResult::NV_OKAY) {
              _saveEnrollResult = ActionResult::SUCCESS;
            } else {
              _saveEnrollResult = ActionResult::FAILURE_ABORT;
            }
          };
          
          std::function<void(NVStorage::NVResult)> saveAlbumCallback = [this](NVStorage::NVResult result)
          {
            if(result == NVStorage::NVResult::NV_OKAY) {
              _saveAlbumResult = ActionResult::SUCCESS;
            } else {
              _saveAlbumResult = ActionResult::FAILURE_ABORT;
            }
          };
          
          _robot.GetVisionComponent().SaveFaceAlbumToRobot(saveAlbumCallback, saveEnrollCallback);
          _state = State::SavingToRobot;
          result = ActionResult::RUNNING;
        }
        
        return result;
      }
        
      case State::SavingToRobot:
      {
        // Wait for Save to complete (success or failure)
        if(ActionResult::FAILURE_NOT_STARTED != _saveEnrollResult &&
           ActionResult::FAILURE_NOT_STARTED != _saveAlbumResult)
        {
          if(ActionResult::SUCCESS == _saveEnrollResult &&
             ActionResult::SUCCESS == _saveAlbumResult)
          {
            PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.CheckIfDone.NVStorageSaveSucceeded", "");
            return ActionResult::SUCCESS;
          }
          else
          {
            PRINT_NAMED_WARNING("EnrollNamedFaceAction.CheckIfDone.NVStorageSaveFailed",
                                "SaveEnroll:%s SaveAlbum:%s. Erasing face.",
                                EnumToString(_saveEnrollResult),
                                EnumToString(_saveAlbumResult));
            
            // if this was a new enrollment, then this person is not going to be
            // known when we reconnect to the robot, so erase them from memory and
            // report failure. If this was a re-enrollment, then we'll just silently
            // fail to remember this particular enrollment data, since that won't
            // have a big effect on the user.
            if(Vision::UnknownFaceID == _saveID)
            {
              _robot.GetVisionComponent().EraseFace(_faceID);
              return ActionResult::FAILURE_ABORT;
            }
            else
            {
              // Failure to save doesn't really matter if this was a re-enrollment. The
              // only downside is that this most recent enrollment won't get reloaded
              // from the robot next time we restart, but that's not a huge loss and
              // it will still get used for the remainder of this session.
              return ActionResult::SUCCESS;
            }
          }
        }
        
        return ActionResult::RUNNING;
      }
        
        
    } // switch(_state)

    return ActionResult::RUNNING;
    
  } // CheckIfDone()
  
  
  void EnrollNamedFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
  {
    FaceEnrollmentCompleted info;
    
    // If observed ID/face are set, then it means we never found a valid, unnamed face to use
    // for enrollment, so return those in the completion union
    if(_observedID != Vision::UnknownFaceID && !_observedName.empty())
    {
      info.faceID = _observedID;
      info.name   = _observedName;
      info.neverSawValidFace = true;
    }
    else
    {
      if(_saveID != Vision::UnknownFaceID)
      {
        // We just merged the enrolled ID (faceID) into saveID, so report saveID as
        // "who" was enrolled
        info.faceID = _saveID;
      }
      else
      {
        info.faceID = _faceID;
      }
      info.name   = _faceName;
      info.neverSawValidFace = false;
    }
    
    static_assert(State::SavingToRobot > State::Finishing, "Bad state ordering for next check");
    info.saidName = (_sayNameWhenDone && _state >= State::Finishing && _action==nullptr);
    
    // Let the caller know if we left the face "scanning" or not.
    // TODO: This should not be necessary once this is a behavior (COZMO-4138)
    info.isFaceScanning = !_idlePopped;
    
    completionUnion.Set_faceEnrollmentCompleted(std::move( info ));
  }
  
  f32 EnrollNamedFaceAction::GetTimeoutInSeconds() const
  {
    if(_state == State::Finishing && nullptr != _action)
    {
      // This is kinda hacky, but we could have used up a lot of our timeout time
      // during enrollment and don't want to cutoff the final action (which could be
      // pretty long if it's a first time enrollment), so increase our timeout
      // at this point.
      return 2.f*IAction::GetTimeoutInSeconds();
    }
  
    return _timeout_sec;
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
    // Listen for changed ID messages in case the FaceRecognizer changes the ID we
    // were enrolling
    if(msg.oldID == _faceID)
    {
      const Vision::TrackedFace* newFace = _robot.GetFaceWorld().GetFace(msg.newID);
      if(msg.newID != _saveID &&
         newFace != nullptr &&
         newFace->HasName())
      {
        // If we just realized the faceID we were enrolling is someone else and that
        // person is already enrolled with a name, we should abort (unless the newID
        // mathces the person we were re-enrolling).
        PRINT_CH_INFO(kLogChannelName,
                      "EnrollNamedFaceAction.HandleRobotChangedObservedFaceID.CannotUpdateToNamedFace",
                       "OldID:%d. NewID:%d is named '%s' and != SaveID:%d, so cannot be used",
                      msg.oldID, msg.newID,
                      Util::HidePersonallyIdentifiableInfo(newFace->GetName().c_str()), _saveID);

        _needToAbort = true;
        
        _observedID = msg.newID;
        _observedName = newFace->GetName();
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.HandleRobotChangedObservedFaceID.UpdatingFaceID",
                      "Was enrolling ID=%d, changing to ID=%d",
                      _faceID, msg.newID);
        _faceID = msg.newID;
      }
    }
    
    if(msg.oldID == _saveID)
    {
      // I don't think this should happen: we should never update a saveID because it
      // should be named, meaning we should never merge into it
      PRINT_NAMED_WARNING("EnrollNamedFaceAction.HandleRobotChangedObservedFaceID.SaveIDChanged",
                          "Was saving to ID=%d, which apparently changed to %d. Should not happen. Will abort.",
                          _saveID, msg.newID);
      _needToAbort = true;
    }
  }
  
  template<>
  void EnrollNamedFaceAction::HandleMessage(const ExternalInterface::RobotObservedFace& msg)
  {
    // We only care about this observed face (a) if it is a different one than we
    // were already using for enrollment, and (b) if it is not for a "tracked" face (one
    // with negative ID, which we never want to try to enroll)
    if(msg.faceID != _faceID && msg.faceID > 0)
    {
      // Record the last person we saw, in case we fail and need to message
      // that the reason why was that we we were seeing this named face.
      // These get unset if we end up using the face in this message.
      _observedID   = msg.faceID;
      _observedName = msg.name;
      
      // We can only switch to this observed faceID if it is unnamed, _unless_
      // it matches the saveID.
      // - for new enrollments we can't enroll an already-named face (that's a re-enrollment, by definition)
      // - for re-enrollment, a face with a name must be the one we are re-enrolling
      // - if the name matches the face ID, then the faceID matches too and we wouldn't even
      //   be considering this observation because there's no ID change
      const bool canUseObservedFace = msg.name.empty() || (msg.faceID == _saveID);
      
      if(canUseObservedFace)
      {
        if(_faceID != Vision::UnknownFaceID)
        {
          // Face ID is already set but we're seeing a face with a different ID.
          // See if it matches the pose of the one we were already enrolling.
          
          auto myFace = _robot.GetFaceWorld().GetFace(_faceID);
          auto newFace = _robot.GetFaceWorld().GetFace(msg.faceID);
          
          if(nullptr != myFace && nullptr != newFace &&
             newFace->GetHeadPose().IsSameAs(myFace->GetHeadPose(),
                                             kUpdateFacePositionThreshold_mm,
                                             DEG_TO_RAD(kUpdateFaceAngleThreshold_deg)))
          {
            PRINT_CH_INFO(kLogChannelName, "EnrollNamedFaceAction.HandleRobotObservedFace.UpdatingFaceIDbyPose",
                          "Was enrolling ID=%d, changing to unnamed ID=%d based on pose (saveID=%d)",
                          _faceID, msg.faceID, _saveID);
            
            // NOTE: Making these equal triggers the next "if" on purpose
            _faceID = msg.faceID;
          }
          
        }
        else
        {
          // We don't have a face ID set yet. Use this one, since it passed all the earlier checks
          PRINT_CH_INFO(kLogChannelName,
                        "EnrollNamedFaceAction.HandleRobotObservedFace.SettingInitialFaceID",
                        "Set face ID to unnamed face %d (saveID=%d)", msg.faceID, _saveID);
          
          _faceID = msg.faceID;
        }
      }
      else
      {
        PRINT_CH_INFO(kLogChannelName,
                      "EnrollNamedFaceAction.HandleRobotObservedFace.IgnoringObservedFace",
                      "Refusing to enroll %s face %d, with current faceID=%d and saveID=%d",
                      msg.name.empty() ? "unnamed" : Util::HidePersonallyIdentifiableInfo(msg.name.c_str()),
                      msg.faceID, _faceID, _saveID);
      }
      
      // Should never be left with a negative faceID to be enrolling
      ASSERT_NAMED(_faceID >= 0, "EnrollNamedFaceAction.HandleRobotObservedFace.SetNegativeFaceID");
      
    } //if(msg.faceID != _faceID && msg.faceID > 0)
    
    if(msg.faceID == _faceID)
    {
      if(_lastFaceSeen_ms == 0) {
        SetBackpackLightsHelper(_robot, NamedColors::GREEN);
      }
      
      _lastFaceSeen_ms = msg.timestamp;
      
      _observedName.clear();
      _observedID = Vision::UnknownFaceID;
    }
    
  }
  
  
  template<>
  void EnrollNamedFaceAction::HandleMessage(const ExternalInterface::UnexpectedMovement& msg)
  {
    if(State::LookForFace == _state && _action != nullptr)
    {
      // Cancel and replace the existing look-around action so we don't fight a
      // person trying to re-orient Cozmo towards themself
      SetAction( CreateLookAroundAction() );
    }
  }
  
} // namespace Cozmo
} // namespace Anki
