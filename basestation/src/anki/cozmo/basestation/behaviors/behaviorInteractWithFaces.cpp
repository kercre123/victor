/**
 * File: behaviorInteractWithFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "InteractWithFaces" behavior, which tracks/interacts with faces if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/trackingActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters_impl.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include <opencv2/highgui/highgui.hpp>

#include "clad/externalInterface/messageEngineToGame.h"

#define DO_FACE_MIMICKING 0
#define DO_TOO_CLOSE_SCARED 1

namespace Anki {
namespace Cozmo {
  
  using namespace ExternalInterface;

  static const char * const kStrongFriendlyReactAnimName = "ID_react2face_friendly_01";
  static const char * const kMinorFriendlyReactAnimName = "ID_react2face_2nd";
  static const char * const kStrongScaredReactAnimName = "ID_react2face_disgust";
  
  BehaviorInteractWithFaces::BehaviorInteractWithFaces(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    SetDefaultName("Faces");

    // TODO: Init timeouts, etc, from Json config

    SubscribeToTags({{
      EngineToGameTag::RobotObservedFace,
      EngineToGameTag::RobotDeletedFace,
      EngineToGameTag::RobotCompletedAction
    }});
    
    if (GetEmotionScorerCount() == 0)
    {
      // Primarily loneliness and then boredom -> InteractWithFaces
      AddEmotionScorer(EmotionScorer(EmotionType::Social,  Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, { 0.0f, 1.0f}, {0.2f, 0.5f}, {1.0f, 0.1f}}), false));
      AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, { 0.0f, 1.0f}, {0.5f, 0.6f}, {1.0f, 0.5f}}), false));
    }
  }
  
  Result BehaviorInteractWithFaces::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
  {
    if (isResuming && (_resumeState != State::Interrupted))
    {
      if (currentTime_sec > _timeWhenInterrupted)
      {
        const double timeWaitingToResume = currentTime_sec - _timeWhenInterrupted;
        if (_newFaceAnimCooldownTime > 0.0)
        {
          _newFaceAnimCooldownTime += timeWaitingToResume;
        }
      }
      _currentState = _resumeState;
      _resumeState = State::Interrupted;
      
      // [MarkW:TODO] Might want to cache and resume anything that was stopped/cancelled in StopInternal()
    }
    else
    {
      _currentState = State::Inactive;
    }
    
    _timeWhenInterrupted = 0.0;
    
    // Make sure we've done this at least once in case StopTracking gets called somehow
    // before StartTracking (which is where we normally store off the original params).
    _originalLiveIdleParams = robot.GetAnimationStreamer().GetAllParams();
    
    return RESULT_OK;
  }
  
  BehaviorInteractWithFaces::~BehaviorInteractWithFaces()
  {

  }
  
  void BehaviorInteractWithFaces::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotObservedFace:
        HandleRobotObservedFace(robot, event);
        break;
        
      case EngineToGameTag::RobotDeletedFace:
        HandleRobotDeletedFace(event);
        break;
        
      case EngineToGameTag::RobotCompletedAction:
        // handled by WhileRunning handler
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorInteractWithFaces.AlwaysHandle.InvalidTag",
                          "Received event with unhandled tag %hhu.",
                          event.GetData().GetTag());
        break;
    }
  }
  
  void BehaviorInteractWithFaces::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotObservedFace:
      case EngineToGameTag::RobotDeletedFace:
        // Handled by AlwaysHandle
        break;
        
      case EngineToGameTag::RobotCompletedAction:
        HandleRobotCompletedAction(robot, event);
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorInteractWithFaces.AlwaysHandle.InvalidTag",
                          "Received event with unhandled tag %hhu.",
                          event.GetData().GetTag());
        break;
    }
  }
  
  void ResetFaceToNeutral(Robot& robot)
  {
#   if DO_FACE_MIMICKING
    //robot.GetMoveComponent().DisableTrackToFace();
    StopTracking(robot);
    ProceduralFace resetFace;
    auto oldTimeStamp = robot.GetProceduralFace().GetTimeStamp();
    oldTimeStamp += IKeyFrame::SAMPLE_LENGTH_MS;
    resetFace.SetTimeStamp(oldTimeStamp);
    robot.SetProceduralFace(resetFace);
#   endif
  }
  
  bool BehaviorInteractWithFaces::IsRunnable(const Robot& robot, double currentTime_sec) const
  {
    bool isRunnable = false;
    for(auto & faceData : _interestingFacesData) {
      if(currentTime_sec > faceData.second._coolDownUntil_sec) {
        isRunnable = true;
        break;
      }
    }

    return isRunnable;
  }
  
  void BehaviorInteractWithFaces::StartTracking(Robot& robot, const Face::ID_t faceID, double currentTime_sec)
  {
    PRINT_NAMED_INFO("BehaviorInteractWithFaces.StartTracking", "FaceID = %llu", faceID);
    
    const Face* face = robot.GetFaceWorld().GetFace(faceID);
    if(nullptr == face) {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.StartTracking.NullFace",
                        "FaceWorld returned null face for ID %llu", faceID);
      RemoveFaceID(faceID);
      return;
    }
    
    auto dataIter = _interestingFacesData.find(faceID);
    if (_interestingFacesData.end() == dataIter)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.StartTracking.MissingInteractionData",
                        "Failed to find interaction data associated with faceID %llu", faceID);
      return;
    }
    
    if (_newFaceAnimCooldownTime == 0.0)
    {
      _newFaceAnimCooldownTime = currentTime_sec;
    }
    
    // If we haven't played our init anim yet for this face and it's been awhile
    // since we did so, do so now and start tracking afterward
    QueueActionPosition queueTrackingPosition = QueueActionPosition::NOW;
    if (!dataIter->second._playedInitAnim && currentTime_sec >= _newFaceAnimCooldownTime)
    {
      robot.GetActionList().Cancel();
      robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new FacePoseAction(face->GetHeadPose(), DEG_TO_RAD(179)));
      
      
      auto friendlyAnimName = kMinorFriendlyReactAnimName;
      if (0 == kCurrentFriendlyAnimCount)
      {
        friendlyAnimName = kStrongFriendlyReactAnimName;
      }
      ++kCurrentFriendlyAnimCount;
      kCurrentFriendlyAnimCount = kCurrentFriendlyAnimCount % kStrongFriendlyAnimRatio;
      
      PlayAnimation(robot, friendlyAnimName, QueueActionPosition::AT_END);
      
      robot.GetMoodManager().AddToEmotions(EmotionType::Happy,  kEmotionChangeMedium,
                                           EmotionType::Social, kEmotionChangeMedium,
                                           EmotionType::Excited,    kEmotionChangeSmall,  "SeeSomethingNew", currentTime_sec);
      dataIter->second._playedInitAnim = true;
      _newFaceAnimCooldownTime = currentTime_sec + kSeeNewFaceAnimationCooldown_sec;
      queueTrackingPosition = QueueActionPosition::AT_END;
    }
    
    dataIter->second._trackingStart_sec = currentTime_sec;
    
    PRINT_NAMED_INFO("BehaviorInteractWithFaces.StartTracking",
                     "Will start tracking face %llu", faceID);
    _trackedFaceID = faceID;
    TrackFaceAction* trackAction = new TrackFaceAction(_trackedFaceID);
    trackAction->SetMoveEyes(true);
    _trackActionTag = trackAction->GetTag();
    trackAction->SetUpdateTimeout(kTrackingTimeout_sec);
    robot.GetActionList().QueueAction(Robot::DriveAndManipulateSlot, queueTrackingPosition, trackAction);
    
    UpdateBaselineFace(robot, face);
    
    {
      // Store LiveIdle params so we can restore after tracking
      _originalLiveIdleParams = robot.GetAnimationStreamer().GetAllParams();
     
      using Param = LiveIdleAnimationParameter;
      //      robot.GetAnimationStreamer().SetParam(Param::EyeDartSpacingMinTime_ms, 0.f);
      //      robot.GetAnimationStreamer().SetParam(Param::EyeDartSpacingMaxTime_ms, 0.25f);
      //robot.GetAnimationStreamer().SetParam(Param::EyeDartMinScale, 1.f);
      //robot.GetAnimationStreamer().SetParam(Param::EyeDartMaxScale, 1.f);
      robot.GetAnimationStreamer().SetParam(Param::EyeDartMaxDistance_pix, 1.f); // reduce dart distance
    }
    
    _currentState = State::TrackingFace;
    
  } // StartTracking()
  
  void BehaviorInteractWithFaces::StopTracking(Robot& robot)
  {
    PRINT_NAMED_INFO("BehaviorInteractWithFaces.StopTracking", "");
    _trackedFaceID = Face::UnknownFace;
    robot.GetActionList().Cancel(_trackActionTag);
    _trackActionTag = (u32)ActionConstants::INVALID_TAG;
    _currentState = State::Inactive;
    robot.GetAnimationStreamer().RemovePersistentFaceLayer(_tiltLayerTag);
    robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeDartLayerTag);
    
    // Put the live idle params back like they were
    robot.GetAnimationStreamer().SetAllParams(_originalLiveIdleParams);
  }
  
  BehaviorInteractWithFaces::Face::ID_t BehaviorInteractWithFaces::GetRandIdHelper()
  {
    // Add all faces other than the one we are currently tracking and ones that
    // are on cooldown to the candidate list to choose next face from.
    std::vector<Face::ID_t> candidateList;
    for(auto & candidate : _interestingFacesData)
    {
      if(candidate.first != _trackedFaceID && candidate.second._coolDownUntil_sec == 0) {
        candidateList.push_back(candidate.first);
      }
    }
    
    if(candidateList.empty()) {
      PRINT_NAMED_INFO("BehaviorInteractWithFaces.GetRandIdHelper.NoAvailableIDs",
                       "No faces available that are not on cooldown");
      return Face::UnknownFace;
    }
    
    const s32 index = GetRNG().RandIntInRange(0, static_cast<s32>(candidateList.size()-1));
    return candidateList[index];
  }
  
  bool BehaviorInteractWithFaces::TrackNextFace(Robot& robot, Face::ID_t currentFace, double currentTime_sec)
  {
    // We are switching away from tracking this face entirely, stop accumulating
    // total tracking time
    auto dataIter = _interestingFacesData.find(currentFace);
    if(dataIter != _interestingFacesData.end()) {
      dataIter->second._cumulativeTrackingTime_sec = 0.;
    }
    
    if(_interestingFacesOrder.empty()) {
      PRINT_NAMED_INFO("BehaviorInteractWithFaces.TrackNextFace.NoMoreFaces",
                       "No more faces to track, switching to Inactive.");
      StopTracking(robot);
      return false;
    } else {
      // Pick next face from those available to look at
      Face::ID_t nextFace = GetRandIdHelper();
      if(Face::UnknownFace == nextFace) {
        PRINT_NAMED_INFO("BehaviorInteractWithFaces.TrackNextFace.NoValidFaces",
                         "All remaining faces are on cooldown");
        return false;
      }
      
      PRINT_NAMED_INFO("BehaviorInteractWithFaces.TrackNextFace",
                       "CurrentFace = %llu, NextFace = %llu", currentFace, nextFace);
      StartTracking(robot, nextFace, currentTime_sec);
      return true;
    }
  } // TrackNextFace()
  
  
  void BehaviorInteractWithFaces::SwitchToDifferentFace(Robot& robot, Face::ID_t currentFace,
                                                        double currentTime_sec)
  {
    if(_interestingFacesOrder.size() > 1)
    {
      // Update cumulative tracking time for the current face before we switch
      // away from it
      auto dataIter = _interestingFacesData.find(currentFace);
      if(dataIter != _interestingFacesData.end()) {
        dataIter->second._cumulativeTrackingTime_sec += currentTime_sec - dataIter->second._trackingStart_sec;
        
        // Cumulative tracking time for the current face has gotten too large:
        // put the face on cool down and remove it from current tracking list
        if(dataIter->second._cumulativeTrackingTime_sec > kFaceInterestingDuration_sec)
        {
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.SwitchToDifferentFace.CoolDown",
                           "Cumulative tracking time for face %llu = %.1fsec > higher than %.1fsec. "
                           "Cooling down and moving to next face.", currentFace,
                           dataIter->second._cumulativeTrackingTime_sec, kFaceInterestingDuration_sec);
          
          dataIter->second._coolDownUntil_sec = currentTime_sec + kFaceCooldownDuration_sec;
          TrackNextFace(robot, currentFace, currentTime_sec);
          return;
        }
      }
      
      Face::ID_t nextFace = GetRandIdHelper();
      PRINT_NAMED_INFO("BehaviorInteractWithFaces.SwitchToDifferentFace",
                       "CurrentFace = %llu, NextFace = %llu", currentFace, nextFace);
      StartTracking(robot, nextFace, currentTime_sec);
    } else {
      PRINT_NAMED_WARNING("BehaviorInteractWithFaces.SwitchToDifferentFace.NoMoreFaces",
                          "Face %llu is the only one in the tracking list", currentFace);
    }
  } // SwitchToDifferentFace()
  
  
  IBehavior::Status BehaviorInteractWithFaces::UpdateInternal(Robot& robot, double currentTime_sec)
  {
    Status status = Status::Running;
    
    // If we're still finishing an action, just wait
    if(_isActing) {
      return status;
    }
    
    /*
    { // Verbose debugging
      std::string trackingFacesStr(" "), interestingFacesStr(" ");
      for(auto trackingFace : _trackingFaces) {
        trackingFacesStr += std::to_string(trackingFace) + " ";
      }
      for(auto interestingFace : _interestingFacesOrder) {
        interestingFacesStr += std::to_string(interestingFace) + " ";
      }
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.UpdateInternal.State",
                        "%s: CurrentFace=%llu, TrackingFaces=[%s\b], InterestingFaces=[%s\b]",
                        GetStateName().c_str(), _trackedFaceID,
                        trackingFacesStr.c_str(), interestingFacesStr.c_str());
    }
    */
    
    // Update cooldown times:
    for(auto & faceData : _interestingFacesData) {
      if(currentTime_sec > faceData.second._coolDownUntil_sec) {
        faceData.second._coolDownUntil_sec = 0;
      }
    }
    
    switch(_currentState)
    {
      case State::Inactive:
      {
        SetStateName("Inactive");
        
        // If enough time has passed since we looked down toward the ground, do that now
        if (currentTime_sec - _lastGlanceTime >= kGlanceDownInterval_sec)
        {
          _lastGlanceTime = currentTime_sec;
          float headAngle = robot.GetHeadAngle();
          if(headAngle > 0.f) // don't bother if we're already at or below zero degrees
          {
            // Move head down to check for a block, then look back up.
            CompoundActionSequential* moveHeadAction = new CompoundActionSequential({
              new MoveHeadToAngleAction(0.),
              new MoveHeadToAngleAction(headAngle),
            });
            _lastActionTag = moveHeadAction->GetTag();
            robot.GetActionList().QueueActionNow(Robot::DriveAndManipulateSlot, moveHeadAction);
            
            _isActing = true;
            break;
          }
        } // if(time to glance)
        
        // Try to start tracking next face in the list. This will put us in TrackinFace
        // state if it succeeds
        if(false == TrackNextFace(robot, _trackedFaceID, currentTime_sec)) {
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.UpdateInternal.NoMoreFaces",
                           "Ran out of interesting faces. Stopping.");
          status = IBehavior::Status::Complete;
        }
        
        break;
      } // case State::Inactive
        
      case State::TrackingFace:
      {
        SetStateName("TrackingFace");
        
        auto faceID = _trackedFaceID;
        
        // Check how long we've been watching this face
        auto watchingFaceDuration = currentTime_sec - _interestingFacesData[faceID]._trackingStart_sec;
        
        if(_interestingFacesOrder.size() > 1)
        {
          // We're tracking multiple faces. See if it's time to switch focus to
          // a different face.
          if(watchingFaceDuration >= _currentMultiFaceInterestingDuration_sec)
          {
            SwitchToDifferentFace(robot, faceID, currentTime_sec);
            
            PRINT_NAMED_INFO("BehaviorInteractWithFaces.Update.SwitchFaces",
                             "WatchingFaceDuration %.2f >= InterestingDuration %.2f.",
                             watchingFaceDuration, _currentMultiFaceInterestingDuration_sec);
            
            _currentMultiFaceInterestingDuration_sec = GetRNG().RandDblInRange(kMultiFaceInterestingDuration_sec-kMultiFaceInterestingVariation_sec, kMultiFaceInterestingDuration_sec+kMultiFaceInterestingVariation_sec);
            break;
          }
        }
        
        // We're just watching one face, see it's time for cooldown
        else if(watchingFaceDuration >= kFaceInterestingDuration_sec)
        {
          robot.GetMoodManager().AddToEmotions(EmotionType::Happy,   kEmotionChangeSmall,
                                               EmotionType::Excited, kEmotionChangeSmall,
                                               EmotionType::Social,  kEmotionChangeLarge,  "LotsOfFace", currentTime_sec);
          
          _interestingFacesData[faceID]._coolDownUntil_sec = currentTime_sec + kFaceCooldownDuration_sec;
          StopTracking(robot);
          
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.Update.FaceOnCooldown",
                           "WatchingFaceDuration %.2f >= InterestingDuration %.2f.",
                           watchingFaceDuration, kFaceInterestingDuration_sec);
          break;
        }
        
        // If we get this far, we're still apparently tracking the same face
        
        // Update cozmo's face based on our currently tracked face
        //UpdateRobotFace(robot);
        
#       if DO_TOO_CLOSE_SCARED
        if(!_isActing &&
           (currentTime_sec - _lastTooCloseScaredTime) > kTooCloseScaredInterval_sec)
        {
          auto face = robot.GetFaceWorld().GetFace(faceID);
          ASSERT_NAMED(nullptr != face, "Face should not be NULL!");
          Pose3d headWrtRobot;
          bool headPoseRetrieveSuccess = face->GetHeadPose().GetWithRespectTo(robot.GetPose(), headWrtRobot);
          if(!headPoseRetrieveSuccess)
          {
            PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.PoseWrtFail","");
            break;
          }
          
          Vec3f headTranslate = headWrtRobot.GetTranslation();
          headTranslate.z() = 0.0f; // We only want to work with XY plane distance
          auto distSqr = headTranslate.LengthSq();
          
          // Keep track of how long a face has been really close, continuously
          static double continuousCloseStartTime_sec = std::numeric_limits<float>::max();
          
          // If a face isn't too close, reset the continuous close face timer
          if(distSqr >= (kTooCloseDistance_mm * kTooCloseDistance_mm))
          {
            continuousCloseStartTime_sec = std::numeric_limits<float>::max();
          }
          else
          {
            // If the timer hasn't been set yet and a face is too close, set the timer
            if (continuousCloseStartTime_sec == std::numeric_limits<float>::max())
            {
              continuousCloseStartTime_sec = currentTime_sec;
            }
            
            if ((currentTime_sec - continuousCloseStartTime_sec) >= kContinuousCloseScareTime_sec)
            {
              // The head is very close (scary!). Move backward along the line from the
              // robot to the head.
              PRINT_NAMED_INFO("BehaviorInteractWithFaces.HandleRobotObservedFace.Shocked",
                               "Head is %.1fmm away: playing shocked anim.",
                               headWrtRobot.GetTranslation().Length());
              
              // Queue the animation to happen now, which will interrupt tracking, but
              // re-enable it immediately after the animation finishes
              PlayAnimation(robot, kStrongScaredReactAnimName, QueueActionPosition::NOW_AND_RESUME);
              
              robot.GetMoodManager().AddToEmotion(EmotionType::Brave, -kEmotionChangeMedium, "CloseFace", currentTime_sec);
              _lastTooCloseScaredTime = currentTime_sec;
              _isActing = true;
              continuousCloseStartTime_sec = std::numeric_limits<float>::max();
            }
          }
        }
#       else
        // avoid pesky unused variable error
        (void)_lastTooCloseScaredTime;
#       endif // DO_TOO_CLOSE_SCARED
        
        break;
      } // case State::TrackingFace
        
      case State::Interrupted:
      {
        SetStateName("Interrupted");
        StopTracking(robot);
        status = Status::Complete;
        break;
      }
        
      default:
        status = Status::Failure;
        
    } // switch(_currentState)
    
    if(Status::Running != status || _currentState == State::Inactive) {
      ResetFaceToNeutral(robot);
    }
    
    return status;
  } // Update()
  
  void BehaviorInteractWithFaces::PlayAnimation(Robot& robot, const std::string& animName,
                                                QueueActionPosition position)
  {
    PlayAnimationAction* animAction = new PlayAnimationAction(animName);
    robot.GetActionList().QueueAction(IBehavior::sActionSlot, position, animAction);
    _lastActionTag = animAction->GetTag();
    _isActing = true;
  }
  
  Result BehaviorInteractWithFaces::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
  {
    _resumeState = isShortInterrupt ? _currentState : State::Interrupted;
    _timeWhenInterrupted = currentTime_sec;
    _currentState = State::Interrupted;
    
    return RESULT_OK;
  }
  
  void BehaviorInteractWithFaces::StopInternal(Robot& robot, double currentTime_sec)
  {
    //robot.GetMoveComponent().DisableTrackToFace();
    StopTracking(robot);
  }
  
#pragma mark -
#pragma mark Signal Handlers

# if DO_FACE_MIMICKING
  inline static f32 GetAverageHeight(const Vision::TrackedFace::Feature& feature,
                                     const Point2f relativeTo, const Radians& faceAngle_rad)
  {
    f32 height = 0.f;
    for(auto point : feature) {
      point -= relativeTo;
      height += -point.x()*std::sin(-faceAngle_rad.ToFloat()) + -point.y()*std::cos(-faceAngle_rad.ToFloat());
    }
    height /= static_cast<f32>(feature.size());
    return height;
  }
  
  inline static f32 GetEyeHeight(const Vision::TrackedFace* face)
  {
    RotationMatrix2d R(-face->GetHeadRoll());
    
    f32 avgEyeHeight = 0.f;
    
    for(auto iFeature : {Vision::TrackedFace::FeatureName::LeftEye, Vision::TrackedFace::FeatureName::RightEye})
    {
      f32 maxY = std::numeric_limits<f32>::lowest();
      f32 minY = std::numeric_limits<f32>::max();
      
      for(auto point : face->GetFeature(iFeature)) {
        point = R*point;
        if(point.y() < minY) {
          minY = point.y();
        }
        if(point.y() > maxY) {
          maxY = point.y();
        }
      }
      
      avgEyeHeight += maxY - minY;
    }
    
    avgEyeHeight *= 0.5f;
    return avgEyeHeight;
  }
# endif // DO_FACE_MIMICKING
  

  void BehaviorInteractWithFaces::UpdateBaselineFace(Robot& robot, const Vision::TrackedFace* face)
  {
    //robot.GetMoveComponent().EnableTrackToFace(face->GetID(), false);
   
#   if DO_FACE_MIMICKING
    
    const Radians& faceAngle = face->GetHeadRoll();
    
    // Record baseline eyebrow heights to compare to for checking if they've
    // raised/lowered in the future
    const Face::Feature& leftEyeBrow  = face->GetFeature(Face::FeatureName::LeftEyebrow);
    const Face::Feature& rightEyeBrow = face->GetFeature(Face::FeatureName::RightEyebrow);
    
    // TODO: Roll correction (normalize roll before checking height?)
    _baselineLeftEyebrowHeight = GetAverageHeight(leftEyeBrow, face->GetLeftEyeCenter(), faceAngle);
    _baselineRightEyebrowHeight = GetAverageHeight(rightEyeBrow, face->GetRightEyeCenter(), faceAngle);
    
    _baselineEyeHeight = GetEyeHeight(face);
    
    _baselineIntraEyeDistance = face->GetIntraEyeDistance();
#   else
    // hack to avoid unused warning
    (void)_baselineEyeHeight;
    (void)_baselineIntraEyeDistance;
    (void)_baselineLeftEyebrowHeight;
    (void)_baselineRightEyebrowHeight;
#   endif // DO_FACE_MIMICKING
  }
  
  void BehaviorInteractWithFaces::HandleRobotObservedFace(const Robot& robot, const EngineToGameEvent& event)
  {
    assert(event.GetData().GetTag() == EngineToGameTag::RobotObservedFace);
    
    const RobotObservedFace& msg = event.GetData().Get_RobotObservedFace();
    
    Face::ID_t faceID = static_cast<Face::ID_t>(msg.faceID);
    
    // We need a valid face to work with
    const Face* face = robot.GetFaceWorld().GetFace(faceID);
    if(face == nullptr)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.InvalidFaceID",
                        "Got event that face ID %lld was observed, but it wasn't found.", faceID);
      return;
    }
    
    auto dataIter = _interestingFacesData.find(faceID);
    
    // If we have an entry for this face, check if the cooldown time has passed
    if (dataIter != _interestingFacesData.end() && dataIter->second._coolDownUntil_sec > 0)
    {
      // There is a cooldown time for this face...
      if(event.GetCurrentTime() > dataIter->second._coolDownUntil_sec) {
        // ...and it has passed, so reset the cooldown on this face.
        dataIter->second._coolDownUntil_sec = 0;
      } else {
        // ...and it has not passed, so ignore this observation
        return;
      }
    }
    
    Pose3d headPose;
    bool gotPose = face->GetHeadPose().GetWithRespectTo(robot.GetPose(), headPose);
    if (!gotPose)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.InvalidFacePose",
                        "Could not get head pose of face ID %lld w.r.t. robot.", faceID);

      return;
    }

    // Determine if head is close enough to bother with
    Vec3f distVec = headPose.GetTranslation();
    distVec.z() = 0;
    const bool closeEnough = distVec.LengthSq() < (kCloseEnoughDistance_mm * kCloseEnoughDistance_mm);
    
    if (_interestingFacesData.end() != dataIter)
    {
      // This is a face we already knew about. If it's close enough, update the
      // last seen time. If not, remove it.
      if(closeEnough) {
        dataIter->second._lastSeen_sec = event.GetCurrentTime();
      } else {
        PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.RemoveFace",
                          "face %lld is too far (%f > %f), removing",
                          faceID,
                          distVec.Length(),
                          kTooFarDistance_mm);
        RemoveFaceID(faceID);
        return;
      }
    } else if(closeEnough) {
      // This is not a face we already knew about, but it's close enough. Add it as one
      // we could choose to track
      _interestingFacesOrder.push_back(faceID);
      auto insertRet = _interestingFacesData.insert( { faceID, FaceData() } );
      if (insertRet.second)
      {
        dataIter = insertRet.first;
        dataIter->second._lastSeen_sec = event.GetCurrentTime();
      }
    }
    
  } // HandleRobotObservedFace()
  
  
  void BehaviorInteractWithFaces::HandleRobotDeletedFace(const EngineToGameEvent& event)
  {
    const RobotDeletedFace& msg = event.GetData().Get_RobotDeletedFace();
    
    RemoveFaceID(static_cast<Face::ID_t>(msg.faceID));
  }
  
  
  void BehaviorInteractWithFaces::RemoveFaceID(Face::ID_t faceID)
  {
    auto dataIter = _interestingFacesData.find(faceID);
    if (_interestingFacesData.end() != dataIter)
    {
      _interestingFacesData.erase(dataIter);
    }
    
    auto orderIter = _interestingFacesOrder.begin();
    while (_interestingFacesOrder.end() != orderIter)
    {
      if ((*orderIter) == faceID)
      {
        orderIter = _interestingFacesOrder.erase(orderIter);
      }
      else
      {
        ++orderIter;
      }
    }
    
  } // RemoveFaceID()
  
  
  void BehaviorInteractWithFaces::UpdateRobotFace(Robot& robot)
  {
    // Occasionally tilt face
    const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(false && currentTime - _lastFaceTiltTime > _faceTiltSpacing)
    {
      AnimationStreamer::FaceTrack tiltTrack;
      ProceduralFace face;

      if(_currentTilt == 0.f) {
        if(GetRNG().RandDbl() > 0.5) {
          _currentTilt = kTiltFaceAmount_deg;
        } else {
          _currentTilt = -kTiltFaceAmount_deg;
        }
      } else {
        _currentTilt = 0.f;
      }
      
      face.GetParams().SetFaceAngle(_currentTilt);

      tiltTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(face, 250));
      robot.GetAnimationStreamer().RemovePersistentFaceLayer(_tiltLayerTag);
      _tiltLayerTag = robot.GetAnimationStreamer().AddPersistentFaceLayer("InteractWithFacesTilt", std::move(tiltTrack));
      
      _lastFaceTiltTime = currentTime;
      _faceTiltSpacing = GetRNG().RandDblInRange(kTiltSpacingMin_sec, kTiltSpacingMax_sec);
      
    } // if(time to tilt face)
    
    auto face = robot.GetFaceWorld().GetFace(_trackedFaceID);
    ASSERT_NAMED(nullptr != face, "Face is null");
    
    Pose3d facePoseWrtCamera;
    if(false == face->GetHeadPose().GetWithRespectTo(robot.GetVisionComponent().GetCamera().GetPose(), facePoseWrtCamera)) {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.UpdateRobotFace.BadPose",
                        "Could not get pose of face %llu w.r.t. camera", _trackedFaceID);
      return;
    }

    f32 xPixShift = 0.f, yPixShift = 0.f;
    if(currentTime - _lastEyeDartTime > _eyeDartSpacing)
    {
      const Point2f& eyeCen = (_lookingAtLeftEye ? face->GetLeftEyeCenter() : face->GetRightEyeCenter());
      
      const f32 focalLength = robot.GetVisionComponent().GetCameraCalibration().GetFocalLength_x();
      
      xPixShift = HEAD_CAM_POSITION[0] * (eyeCen.x() - robot.GetVisionComponent().GetCameraCalibration().GetCenter_x()) / focalLength;
      yPixShift = HEAD_CAM_POSITION[0] * (eyeCen.y() - robot.GetVisionComponent().GetCameraCalibration().GetCenter_y()) / focalLength;
      
      _lookingAtLeftEye = !_lookingAtLeftEye;
      _lastEyeDartTime = currentTime;
      _eyeDartSpacing = GetRNG().RandDblInRange(kEyeDartSpacingMin_sec, kEyeDartSpacingMax_sec);
    } // if(time to dart eyes)

    // TODO: Try to get this to work well (scale eyes with distance)
//    // Scale both eyes with distance
//    const f32 CloseScale = 0.6f;
//    const f32 FarScale   = 1.3f;
//    f32 distScale = CLIP((facePoseWrtCamera.GetTranslation().Length()-kTooCloseDistance_mm)/(kTooFarDistance_mm-kTooCloseDistance_mm)*(FarScale-CloseScale) + CloseScale, CloseScale, FarScale);
    if(xPixShift != 0 || yPixShift != 0) { // TODO: remove
      robot.ShiftEyes(_eyeDartLayerTag, xPixShift, yPixShift, 100, "InteractWithFacesMimic");
    }

#   if DO_FACE_MIMICKING
    ProceduralFace prevProcFace(proceduralFace);
    
    const Radians& faceAngle = face.GetHeadRoll();
    const f32 distanceNorm =  face.GetIntraEyeDistance() / _baselineIntraEyeDistance;
    
    if(_baselineLeftEyebrowHeight != 0.f && _baselineRightEyebrowHeight != 0.f)
    {
      // If eyebrows have raised/lowered (based on distance from eyes), mimic their position:
      const Face::Feature& leftEyeBrow  = face.GetFeature(Face::FeatureName::LeftEyebrow);
      const Face::Feature& rightEyeBrow = face.GetFeature(Face::FeatureName::RightEyebrow);
      
      const f32 leftEyebrowHeight  = GetAverageHeight(leftEyeBrow, face.GetLeftEyeCenter(), faceAngle);
      const f32 rightEyebrowHeight = GetAverageHeight(rightEyeBrow, face.GetRightEyeCenter(), faceAngle);
      
      // Get expected height based on intra-eye distance
      const f32 expectedLeftEyebrowHeight = distanceNorm * _baselineLeftEyebrowHeight;
      const f32 expectedRightEyebrowHeight = distanceNorm * _baselineRightEyebrowHeight;
      
      // Compare measured distance to expected
      const f32 leftEyebrowHeightScale = (leftEyebrowHeight - expectedLeftEyebrowHeight)/expectedLeftEyebrowHeight;
      const f32 rightEyebrowHeightScale = (rightEyebrowHeight - expectedRightEyebrowHeight)/expectedRightEyebrowHeight;
      
      // Map current eyebrow heights onto Cozmo's face, based on measured baseline values
      proceduralFace.GetParams().SetParameter(ProceduralFace::WhichEye::Left,
                                              ProceduralFace::Parameter::UpperLidY,
                                              leftEyebrowHeightScale);
      
      proceduralFace.GetParams().SetParameter(ProceduralFace::WhichEye::Right,
                                              ProceduralFace::Parameter::UpperLidY,
                                              rightEyebrowHeightScale);
      
    }
    
    const f32 expectedEyeHeight = distanceNorm * _baselineEyeHeight;
    const f32 eyeHeightFraction = (GetEyeHeight(&face) - expectedEyeHeight)/expectedEyeHeight + .1f; // bias a little larger
    
    // Adjust pupil positions depending on where face is in the image
    Point2f newPupilPos(face.GetLeftEyeCenter());
    newPupilPos += face.GetRightEyeCenter();
    newPupilPos *= 0.5f;
    
    const Vision::CameraCalibration& camCalib = robot.GetVisionComponent().GetCameraCalibration();
    Point2f imageHalfSize(camCalib.GetNcols()/2, camCalib.GetNrows()/2);
    newPupilPos -= imageHalfSize; // make relative to image center
    newPupilPos /= imageHalfSize; // scale to be between -1 and 1

    // magic value to make pupil tracking feel more realistic
    // TODO: Actually intersect vector from robot head to tracked face with screen
    newPupilPos *= .75f;
    
    for(auto whichEye : {ProceduralFace::WhichEye::Left, ProceduralFace::WhichEye::Right}) {
      if(_baselineEyeHeight != 0.f) {
        proceduralFace.GetParams().SetParameter(whichEye, ProceduralFace::Parameter::EyeScaleX,
                                                std::max(-.8f, std::min(.8f, eyeHeightFraction)));
      }
    }
    
    // If face angle is rotated, mirror the rotation (with a deadzone)
    if(std::abs(faceAngle.getDegrees()) > 5) {
      proceduralFace.GetParams().SetFaceAngle(faceAngle.getDegrees());
    } else {
      proceduralFace.GetParams().SetFaceAngle(0);
    }
    
    // Smoothing
    proceduralFace.GetParams().Interpolate(prevProcFace.GetParams(), proceduralFace.GetParams(), 0.9f);
    
    proceduralFace.SetTimeStamp(face.GetTimeStamp());
    proceduralFace.MarkAsSentToRobot(false);
    robot.SetProceduralFace(proceduralFace);
#   endif // DO_FACE_MIMICKING
  } // UpdateProceduralFace()
  
  
  void BehaviorInteractWithFaces::HandleRobotCompletedAction(Robot& robot, const EngineToGameEvent& event)
  {
    const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
    
    if(msg.idTag == _lastActionTag)
    {
#     if DO_FACE_MIMICKING
      robot.SetProceduralFace(_crntProceduralFace);
#     endif
      _isActing = false;
    }
    
    if(RobotActionType::TRACK_FACE == msg.actionType) {
      if(_trackActionTag == msg.idTag) {
        const auto lastFaceID = msg.completionInfo.Get_trackFaceCompleted().faceID;
        
        if(ActionResult::FAILURE_TIMEOUT == msg.result) {
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.HandleRobotCompletedAction.TimedOut",
                           "Timed out tracking face %d, attempting to switch to next.",
                           lastFaceID);
          
          robot.GetMoodManager().AddToEmotions(EmotionType::Happy,  -kEmotionChangeVerySmall,
                                               EmotionType::Social, -kEmotionChangeVerySmall, "LostFace", MoodManager::GetCurrentTimeInSeconds());
        }
        
        TrackNextFace(robot, lastFaceID, event.GetCurrentTime());
      }
    }
  } // HandleRobotCompletedAction()
  



} // namespace Cozmo
} // namespace Anki
