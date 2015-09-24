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
#include "anki/cozmo/basestation/emotionManager.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"

#include "anki/common/basestation/math/point_impl.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include <opencv2/highgui/highgui.hpp>

namespace Anki {
namespace Cozmo {
  
  using namespace ExternalInterface;

  BehaviorInteractWithFaces::BehaviorInteractWithFaces(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    _name = "InteractWithFaces";

    // TODO: Init timeouts, etc, from Json config
    
    // Set up signal handlers for while we are running
    if (_robot.HasExternalInterface())
    {
      IExternalInterface* interface = _robot.GetExternalInterface();

      _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotObservedFace,
                                                  std::bind(&BehaviorInteractWithFaces::HandleRobotObservedFace,
                                                            this,
                                                            std::placeholders::_1)));
      
      _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotDeletedFace,
                                                   std::bind(&BehaviorInteractWithFaces::HandleRobotDeletedFace,
                                                             this,
                                                             std::placeholders::_1)));
      
      _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotCompletedAction,
                                                   std::bind(&BehaviorInteractWithFaces::HandleRobotCompletedAction,
                                                             this,
                                                             std::placeholders::_1)));
    }

  }
  
  Result BehaviorInteractWithFaces::Init(double currentTime_sec)
  {

    
    // Make sure the robot's idle animation is set to use Live, since we are
    // going to stream live face mimicking
    _robot.SetIdleAnimation(AnimationStreamer::LiveAnimation);
    _currentState = State::Inactive;
    
    return RESULT_OK;
  }
  
  bool BehaviorInteractWithFaces::IsRunnable(double currentTime_sec) const
  {
    return !_interestingFacesOrder.empty();
  }
  
  IBehavior::Status BehaviorInteractWithFaces::Update(double currentTime_sec)
  {
    switch(_currentState)
    {
      case State::Inactive:
      {
        // If we're still finishing an action, just wait
        if(_isActing)
        {
          break;
        }
        
        // If enough time has passed since we looked down toward the ground, do that now
        if (currentTime_sec - _lastGlanceTime >= kGlanceDownInterval_sec)
        {
          float headAngle = _robot.GetHeadAngle();
          
          // Move head down to check for a block
          MoveHeadToAngleAction* moveHeadAction = new MoveHeadToAngleAction(0);
          _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveHeadAction);
          
          // Now move the head back up to the angle it was previously at
          moveHeadAction = new MoveHeadToAngleAction(headAngle);
          
          _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveHeadAction);
          _lastActionTag = moveHeadAction->GetTag();
          _isActing = true;
          _lastGlanceTime = currentTime_sec;
          break;
        }
        
        // If we don't have any faces to care about, we're done here
        auto iterFirst = _interestingFacesOrder.begin();
        if (_interestingFacesOrder.end() == iterFirst)
        {
          _currentState = State::Interrupted;
          break;
        }
        
        auto faceID = *iterFirst;
        const Face* face = _robot.GetFaceWorld().GetFace(faceID);
        if(face == nullptr)
        {
          PRINT_NAMED_ERROR("BehaviorInteractWithFaces.Update.InvalidFaceID",
                            "Got event that face ID %lld was observed, but it wasn't found.",
                            faceID);
          break;
        }
        
        auto dataIter = _interestingFacesData.find(faceID);
        if (_interestingFacesData.end() == dataIter)
        {
          PRINT_NAMED_ERROR("BehaviorInteractWithFaces.Update.MissingInteractionData",
                            "Failed to find interaction data associated with faceID %llu", faceID);
          break;
        }
        dataIter->second._trackingStart_sec = currentTime_sec;
        
        // Start tracking face
        UpdateBaselineFace(face);
        
        PRINT_NAMED_INFO("BehaviorInteractWithFaces.Update.SwitchToTracking",
                         "Observed face %llu while looking around, switching to tracking.", faceID);
        _currentState = State::TrackingFace;
        break;
      }
        
      case State::TrackingFace:
      {
        auto faceID = _robot.GetTrackToFace();
        // If we aren't tracking the first faceID in the list, something's wrong
        if (_interestingFacesOrder.empty() || _interestingFacesOrder.front() != faceID)
        {
          // The face we're tracking doesn't match the first one in our list, so reset our state to select the right one
          _robot.DisableTrackToFace();
          _currentState = State::Inactive;
          break;
        }
        
        // If too much time has passed since we last saw this face, remove it go back to inactive state and find a new face
        auto lastSeen = _interestingFacesData[faceID]._lastSeen_sec;
        if(currentTime_sec - lastSeen > _trackingTimeout_sec)
        {
          _robot.DisableTrackToFace();
          _interestingFacesOrder.erase(_interestingFacesOrder.begin());
          _interestingFacesData.erase(faceID);
          
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.Update.DisablingTracking",
                           "Current t=%.2f - lastSeen time=%.2f > timeout=%.2f. "
                           "Switching back to looking around.",
                           currentTime_sec, lastSeen, _trackingTimeout_sec);
          _currentState = State::Inactive;
          break;
        }
        
        // If we've watched this face longer than it's considered interesting, put it on cooldown and go to inactive
        auto watchingFaceDuration = currentTime_sec - _interestingFacesData[faceID]._trackingStart_sec;
        if (watchingFaceDuration >= kFaceInterestingDuration_sec)
        {
          _robot.DisableTrackToFace();
          _interestingFacesOrder.erase(_interestingFacesOrder.begin());
          _interestingFacesData.erase(faceID);
          _cooldownFaces[faceID] = currentTime_sec + kFaceCooldownDuration_sec;
          
          PRINT_NAMED_INFO("BehaviorInteractWithFaces.Update.FaceOnCooldown",
                           "WatchingFaceDuration %.2f >= InterestingDuration %.2f.",
                           watchingFaceDuration, kFaceInterestingDuration_sec);
          _currentState = State::Inactive;
        }
        
        // We need a face to work with
        const Face* face = _robot.GetFaceWorld().GetFace(faceID);
        if(face == nullptr)
        {
          PRINT_NAMED_ERROR("BehaviorInteractWithFaces.Update.InvalidFaceID",
                            "Updating with face ID %lld, but it wasn't found.",
                            faceID);
          _robot.DisableTrackToFace();
          _currentState = State::Inactive;
          break;
        }
        
        // Update cozmo's face based on our currently focused face
        UpdateProceduralFace(_crntProceduralFace, *face);
        
        if(!_isActing)
        {
          Pose3d headWrtRobot;
          bool headPoseRetrieveSuccess = face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), headWrtRobot);
          if(!headPoseRetrieveSuccess)
          {
            PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.PoseWrtFail","");
            break;
          }
          
          Vec3f headTranslate = headWrtRobot.GetTranslation();
          headTranslate.z() = 0.0f; // We only want to work with XY plane distance
          auto distSqr = headTranslate.LengthSq();
          if(distSqr < (kTooCloseDistance_mm * kTooCloseDistance_mm))
          {
            // The head is very close (scary!). Move backward along the line from the
            // robot to the head.
            PRINT_NAMED_INFO("BehaviorInteractWithFaces.HandleRobotObservedFace.Shocked",
                             "Head is %.1fmm away: playing shocked anim.",
                             headWrtRobot.GetTranslation().Length());
            PlayAnimationAction* animAction = new PlayAnimationAction("Demo_Face_Interaction_ShockedScared_A");
            _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, animAction);
            _robot.GetEmotionManager().HandleEmotionalMoment(EmotionManager::EmotionEvent::CloseFace);
            MoveToSafeDistanceFromPoint(headTranslate);
          }
          else if (distSqr > (kTooFarDistance_mm * kTooFarDistance_mm))
          {
            MoveToSafeDistanceFromPoint(headTranslate);
          }
        }
        
        break;
      }
      case State::Interrupted:
      {
        // Since we're ending being in InteractWithFaces, set our procedural face to be neutral
        ProceduralFace resetFace;
        auto oldTimeStamp = _robot.GetProceduralFace().GetTimeStamp();
        oldTimeStamp += IKeyFrame::SAMPLE_LENGTH_MS;
        resetFace.SetTimeStamp(oldTimeStamp);
        _robot.SetProceduralFace(resetFace);
        return Status::Complete;
      }
      default:
        
        return Status::Failure;
    } // switch(_currentState)
    
    return Status::Running;
  } // Update()
  
  void BehaviorInteractWithFaces::MoveToSafeDistanceFromPoint(const Vec3f& robotRelativePoint)
  {
    Pose3d goalPose = _robot.GetPose();
    auto pointDistance = robotRelativePoint.Length();
    // Calculate the ratio of the relative point minus the safe distance to the overall current distance
    auto distanceRatio = (pointDistance - (kTooFarDistance_mm + kTooCloseDistance_mm) / 2.0f) / pointDistance;
    
    // Use the distance ratio to create a goal pose where the translation is the safe distance away from the relative point
    goalPose.SetTranslation(goalPose.GetTranslation() + goalPose.GetRotation() * (robotRelativePoint * distanceRatio));
    
    DriveToPoseAction* driveAction = new DriveToPoseAction(goalPose, false, false);
    _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, driveAction);
    _lastActionTag = driveAction->GetTag();
    _isActing = true;
    
    // Disable our face tracking to allow the drive to pose action to function
    _robot.DisableTrackToFace();
    
    // Set our state back to inactive so we can correctly refocus on a face when we're done moving
    _currentState = State::Inactive;
  }
  
  Result BehaviorInteractWithFaces::Interrupt(double currentTime_sec)
  {
    _robot.DisableTrackToFace();
    _currentState = State::Interrupted;
    
    return RESULT_OK;
  }
  
  bool BehaviorInteractWithFaces::GetRewardBid(Reward& reward)
  {
    // TODO: Fill reward  in...
    return true;
  }
  
#pragma mark -
#pragma mark Signal Handlers
  
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
      f32 maxY = std::numeric_limits<f32>::min();
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
  
  void BehaviorInteractWithFaces::UpdateBaselineFace(const Vision::TrackedFace* face)
  {
    _robot.EnableTrackToFace(face->GetID(), false);
    
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
  }
  
  void BehaviorInteractWithFaces::HandleRobotObservedFace(const AnkiEvent<MessageEngineToGame>& event)
  {
    const RobotObservedFace& msg = event.GetData().Get_RobotObservedFace();
    
    Face::ID_t faceID = static_cast<Face::ID_t>(msg.faceID);
    
    auto iter = _cooldownFaces.find(faceID);
    // If we have a cooldown entry for this face, check if the cooldown time has passed
    if (_cooldownFaces.end() != iter && iter->second < event.GetCurrentTime())
    {
      _cooldownFaces.erase(iter);
      iter = _cooldownFaces.end();
    }
    
    // This face is still on cooldown, so ignore its observation
    if (_cooldownFaces.end() != iter)
    {
      return;
    }
    
    // If we don't already have data on this faceID, we need to add it to our order list
    auto dataIter = _interestingFacesData.find(faceID);
    if (_interestingFacesData.end() == dataIter)
    {
      _interestingFacesOrder.push_back(faceID);
    }
    _interestingFacesData[faceID]._lastSeen_sec = event.GetCurrentTime();
  }
  
  void BehaviorInteractWithFaces::HandleRobotDeletedFace(const AnkiEvent<MessageEngineToGame>& event)
  {
    const RobotDeletedFace& msg = event.GetData().Get_RobotDeletedFace();
    
    Face::ID_t faceID = static_cast<Face::ID_t>(msg.faceID);
    
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
        orderIter++;
      }
    }
  }
  
  void BehaviorInteractWithFaces::UpdateProceduralFace(ProceduralFace& proceduralFace, const Face& face) const
  {
    ProceduralFace prevProcFace(proceduralFace);
    
    const Radians& faceAngle = face.GetHeadRoll();
    
    // If eyebrows have raised/lowered (based on distance from eyes), mimic their position:
    const Face::Feature& leftEyeBrow  = face.GetFeature(Face::FeatureName::LeftEyebrow);
    const Face::Feature& rightEyeBrow = face.GetFeature(Face::FeatureName::RightEyebrow);
    
    const f32 leftEyebrowHeight  = GetAverageHeight(leftEyeBrow, face.GetLeftEyeCenter(), faceAngle);
    const f32 rightEyebrowHeight = GetAverageHeight(rightEyeBrow, face.GetRightEyeCenter(), faceAngle);
    
    const f32 distanceNorm =  face.GetIntraEyeDistance() / _baselineIntraEyeDistance;
    
    // Get expected height based on intra-eye distance
    const f32 expectedLeftEyebrowHeight = distanceNorm * _baselineLeftEyebrowHeight;
    const f32 expectedRightEyebrowHeight = distanceNorm * _baselineRightEyebrowHeight;
    
    // Compare measured distance to expected
    const f32 leftEyebrowHeightScale = (leftEyebrowHeight - expectedLeftEyebrowHeight)/expectedLeftEyebrowHeight;
    const f32 rightEyebrowHeightScale = (rightEyebrowHeight - expectedRightEyebrowHeight)/expectedRightEyebrowHeight;
    
    // Map current eyebrow heights onto Cozmo's face, based on measured baseline values
    proceduralFace.SetParameter(ProceduralFace::WhichEye::Left,
                                     ProceduralFace::Parameter::BrowCenY,
                                     leftEyebrowHeightScale);
    
    proceduralFace.SetParameter(ProceduralFace::WhichEye::Right,
                                     ProceduralFace::Parameter::BrowCenY,
                                     rightEyebrowHeightScale);
    
    const f32 expectedEyeHeight = distanceNorm * _baselineEyeHeight;
    const f32 eyeHeightFraction = (GetEyeHeight(&face) - expectedEyeHeight)/expectedEyeHeight + .1f; // bias a little larger
    
    for(auto whichEye : {ProceduralFace::WhichEye::Left, ProceduralFace::WhichEye::Right}) {
      proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::EyeHeight,
                                       eyeHeightFraction);
      proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilHeight,
                                       std::max(.25f, std::min(.75f,eyeHeightFraction)));
    }
    // Adjust pupil positions depending on where face is in the image
    Point2f newPupilPos(face.GetLeftEyeCenter());
    newPupilPos += face.GetRightEyeCenter();
    newPupilPos *= 0.5f;
    
    Point2f imageHalfSize(_robot.GetCamera().GetCalibration().GetNcols()/2,
                          _robot.GetCamera().GetCalibration().GetNrows()/2);
    newPupilPos -= imageHalfSize; // make relative to image center
    newPupilPos /= imageHalfSize; // scale to be between -1 and 1
    newPupilPos *= .8f; // magic value to make it more realistic
    
    for(auto whichEye : {ProceduralFace::WhichEye::Left, ProceduralFace::WhichEye::Right}) {
      proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::EyeHeight,
                                       eyeHeightFraction);
      proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilHeight,
                                       eyeHeightFraction);
      
      // To get saccade-like movement, only update the pupils if they new position is
      // different enough
      Point2f pupilChange(newPupilPos);
      pupilChange.x() -= proceduralFace.GetParameter(whichEye, ProceduralFace::Parameter::PupilCenX);
      pupilChange.y() -= proceduralFace.GetParameter(whichEye, ProceduralFace::Parameter::PupilCenY);
      if(pupilChange.Length() > .15f) { // TODO: Tune this parameter to get better-looking saccades
        proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilCenX,
                                         -newPupilPos.x());
        proceduralFace.SetParameter(whichEye, ProceduralFace::Parameter::PupilCenY,
                                         newPupilPos.y());
      }
    }
    
    // If face angle is rotated, mirror the rotation (with a deadzone)
    if(std::abs(faceAngle.getDegrees()) > 5) {
      proceduralFace.SetFaceAngle(faceAngle.getDegrees()/static_cast<f32>(ProceduralFace::MaxFaceAngle));
    } else {
      proceduralFace.SetFaceAngle(0);
    }
    
    // Smoothing
    proceduralFace.Interpolate(prevProcFace, proceduralFace, 0.9f);
    
    proceduralFace.SetTimeStamp(face.GetTimeStamp());
    proceduralFace.MarkAsSentToRobot(false);
    _robot.SetProceduralFace(proceduralFace);
  }
  
  void BehaviorInteractWithFaces::HandleRobotCompletedAction(const AnkiEvent<MessageEngineToGame>& event)
  {
    const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
    
    if(msg.idTag == _lastActionTag)
    {
      _robot.SetProceduralFace(_crntProceduralFace);
      _isActing = false;
    }
  }
} // namespace Cozmo
} // namespace Anki