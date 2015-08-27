/**
 * File: behaviorLookForFaces.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "LookForFaces" behavior, which searches for people's
 *              faces and tracks/interacts with them if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include <opencv2/highgui/highgui.hpp>

namespace Anki {
namespace Cozmo {

  BehaviorLookForFaces::BehaviorLookForFaces(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::LOOKING_AROUND)
  , _animationStarted(false)
  {
    
    
  }
  
  Result BehaviorLookForFaces::Init()
  {
    _currentState = State::LOOKING_AROUND;
    
    // Set up signal handlers for while we are running
    
    auto cbObservedObject = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) {
      this->HandleRobotObservedFace(event.GetData().Get_RobotObservedFace());
    };
    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedFace, cbObservedObject));
    
    auto cbActionCompleted = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) {
      this->HandleRobotCompletedAction(event.GetData().Get_RobotCompletedAction());
    };
    _eventHandles.emplace_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotCompletedAction, cbActionCompleted));
    
    return RESULT_OK;
  }
  
  
  IBehavior::Status BehaviorLookForFaces::Update(float currentTime_sec)
  {
    _currentTime_sec = currentTime_sec;
    
    switch(_currentState)
    {
      case State::LOOKING_AROUND:
        
        if(currentTime_sec - _lastLookAround_sec > _nextMovementTime_sec)
        {
          /*
          // Time to move again: tilt head and move body by a random amount
          // TODO: Get the angle limits from config
          // TODO: re-enable random looking around (turned off for debugging streaming)
          const f32 headAngle_deg = 0; //_rng.RandIntInRange(0, MAX_HEAD_ANGLE);
          const f32 bodyAngle_deg = 0; //_rng.RandIntInRange(-90, 90);
           
          MoveHeadToAngleAction* moveHeadAction    = new MoveHeadToAngleAction(DEG_TO_RAD(headAngle_deg));
          TurnInPlaceAction*     turnInPlaceAction = new TurnInPlaceAction(DEG_TO_RAD(bodyAngle_deg), false);
          
          CompoundActionParallel* action = new CompoundActionParallel({moveHeadAction, turnInPlaceAction});
          _movementActionTag = action->GetTag();
          _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, action);
          */
          _currentState = State::MOVING;
        }

        break;
        
      case State::MOVING:
        // Nothing to do, just waiting for action to complete
        break;
        
      case State::TRACKING_FACE:
        if(_currentTime_sec - _lastSeen_sec > _trackingTimeout_sec) {
          _robot.DisableTrackToObject();
          _currentState = State::LOOKING_AROUND;
          SetNextMovementTime();
        }
        break;
        
      case State::INTERRUPTED:
        // This is the only way we stop this behavior: unsubscribe to all signals.
        _eventHandles.clear();
        return Status::Complete;
        
      default:
        
        return Status::Failure;
    } // switch(_currentState)
    
    return Status::Running;
  } // Update()
  
  Result BehaviorLookForFaces::Interrupt(float currentTime_sec)
  {
    _robot.DisableTrackToObject();
    _currentState = State::INTERRUPTED;
    
    return RESULT_OK;
  }
  
  bool BehaviorLookForFaces::GetRewardBid(Reward& reward)
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
    f32 maxY = std::numeric_limits<f32>::min();
    f32 minY = std::numeric_limits<f32>::max();
    for(auto iFeature : {Vision::TrackedFace::FeatureName::LeftEye, Vision::TrackedFace::FeatureName::RightEye})
    {
      for(auto point : face->GetFeature(iFeature)) {
        point = R*point;
        if(point.y() < minY) {
          minY = point.y();
        }
        if(point.y() > maxY) {
          maxY = point.y();
        }
      }
    }
    if(maxY < minY) {
      PRINT_NAMED_ERROR("GetEyeHeight.NegativeHeight", "");
      return 0.f;
    }
    return maxY - minY;
  }
  
  
  void BehaviorLookForFaces::HandleRobotObservedFace(const ExternalInterface::RobotObservedFace &msg)
  {
    using Face = Vision::TrackedFace;
    
    Face::ID_t faceID = static_cast<Face::ID_t>(msg.faceID);
    const Face* face = _robot.GetFaceWorld().GetFace(faceID);
    if(face == nullptr) {
      PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotObservedFace.InvalidFaceID",
                        "Got event that face ID %lld was observed, but it wasn't found.",
                        faceID);
      return;
    }
  
    const Radians& faceAngle = face->GetHeadRoll();
    
    _lastSeen_sec = _currentTime_sec;
    
    switch(_currentState)
    {
      case State::LOOKING_AROUND:
      case State::MOVING:
      {
        // If we aren't already tracking, start tracking the next face we see
        _firstSeen_ms = _lastSeen_ms = face->GetTimeStamp();
        _robot.EnableTrackToFace(faceID, false);
        
        // Record baseline eyebrow heights to compare to for checking if they've
        // raised/lowered in the future
        const Face::Feature& leftEyeBrow  = face->GetFeature(Face::FeatureName::LeftEyebrow);
        const Face::Feature& rightEyeBrow = face->GetFeature(Face::FeatureName::RightEyebrow);
      
        // TODO: Roll correction (normalize roll before checking height?)
        _baselineLeftEyebrowHeight = GetAverageHeight(leftEyeBrow, face->GetLeftEyeCenter(), faceAngle);
        _baselineRightEyebrowHeight = GetAverageHeight(rightEyeBrow, face->GetRightEyeCenter(), faceAngle);
        
        _baselineEyeHeight = GetEyeHeight(face);
        
        _currentState = State::TRACKING_FACE;
        break;
      }
        
      case State::TRACKING_FACE:
        //TODO: if(faceID == _robot.GetTrackToFace())
        //if(!_animationStarted)
        {
          // If eyebrows have raised/lowered (based on distance from eyes), mimic their position:
          const Face::Feature& leftEyeBrow  = face->GetFeature(Face::FeatureName::LeftEyebrow);
          const Face::Feature& rightEyeBrow = face->GetFeature(Face::FeatureName::RightEyebrow);
          //const Face::Feature& leftEye      = face->GetFeature(Face::FeatureName::LeftEye);
          //const Face::Feature& rightEye     = face->GetFeature(Face::FeatureName::RightEye);
        
          const f32 leftEyebrowHeight  = GetAverageHeight(leftEyeBrow, face->GetLeftEyeCenter(), faceAngle);
          const f32 rightEyebrowHeight = GetAverageHeight(rightEyeBrow, face->GetRightEyeCenter(), faceAngle);
          
          // Map current eyebrow heights onto Cozmo's face, based on measured baseline values
          const f32 distLeftEyeTopToImageTop = static_cast<f32>(ProceduralFace::NominalEyeCenY - _crntProceduralFace.GetParameter(ProceduralFace::WhichEye::Left, ProceduralFace::EyeHeight)/2);
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Left,
                                           ProceduralFace::Parameter::BrowShiftY,
                                           (_baselineLeftEyebrowHeight-leftEyebrowHeight)/_baselineLeftEyebrowHeight *
                                           distLeftEyeTopToImageTop);
          
          const f32 distRightEyeTopToImageTop = static_cast<f32>(ProceduralFace::NominalEyeCenY - _crntProceduralFace.GetParameter(ProceduralFace::WhichEye::Left, ProceduralFace::EyeHeight)/2);
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Right,
                                           ProceduralFace::Parameter::BrowShiftY,
                                           (_baselineRightEyebrowHeight-rightEyebrowHeight)/_baselineRightEyebrowHeight *
                                           distRightEyeTopToImageTop);
          
          const f32 eyeHeightFraction = GetEyeHeight(face)/_baselineEyeHeight;
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Left, ProceduralFace::Parameter::EyeHeight,
                                           static_cast<f32>(ProceduralFace::NominalEyeHeight)*eyeHeightFraction);
          
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Right, ProceduralFace::Parameter::EyeHeight,
                                           static_cast<f32>(ProceduralFace::NominalEyeHeight)*eyeHeightFraction);
          
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Left, ProceduralFace::Parameter::PupilHeight,
                                           static_cast<f32>(ProceduralFace::NominalPupilHeight)*eyeHeightFraction);
          
          _crntProceduralFace.SetParameter(ProceduralFace::WhichEye::Right, ProceduralFace::Parameter::PupilHeight,
                                           static_cast<f32>(ProceduralFace::NominalPupilHeight)*eyeHeightFraction);
          
          // If face angle is rotated, mirror the rotation
          _crntProceduralFace.SetFaceAngle(faceAngle.getDegrees());
          
          ProceduralFace streamingFace;
          MessageAnimKeyFrame_FaceImage keyframeMsg;
          std::vector<u8> rleData;
          
          // Stream this face, interpolating from last
          TimeStamp_t time = _lastSeen_ms + FaceImageKeyFrame::SAMPLE_LENGTH_MS;
          while(time < face->GetTimeStamp()) {
            // Get the interpolated procedural face
            const f32 blendFraction = (static_cast<f32>(time - _lastSeen_ms) /
                                       static_cast<f32>(face->GetTimeStamp() - _lastSeen_ms));
            streamingFace.Interpolate(_lastProceduralFace, _crntProceduralFace, blendFraction);
            
            // Get the image for that face
            cv::Mat_<u8> faceImg = streamingFace.GetFace();
            //static int fileCtr=0;
            //cv::imwrite("/Users/andrew/temp/faceImg_" + std::to_string(fileCtr++) + ".png", faceImg);

            FaceAnimationManager::getInstance()->AddImage(FaceAnimationManager::ProceduralAnimName, faceImg);
            
            /*
            PRINT_NAMED_INFO("BehaviorLookForFaces.HandleRobotObservedFace.StreamingProceduralFace",
                             "Streaming face at t=%d from interval [%d %d], with blendFraction=%.2f [ProceduralFaceAnim=%dframes long]",
                             time, _lastSeen_ms, face->GetTimeStamp(), blendFraction,
                             FaceAnimationManager::getInstance()->GetNumFrames(FaceAnimationManager::ProceduralAnimName));
            */
            
            time += FaceImageKeyFrame::SAMPLE_LENGTH_MS;
          }
          
          // Update the stored face and time for next time
          _lastSeen_ms = face->GetTimeStamp();
          std::swap(_lastProceduralFace, _crntProceduralFace);
          
          if(!_animationStarted) {
            _robot.GetActionList().Cancel(IBehavior::sActionSlot, RobotActionType::PLACE_OBJECT_LOW);
            _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlayAnimationAction(FaceAnimationManager::ProceduralAnimName));
            _animationStarted = true;
          }
        }
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotObservedFace.UnknownState",
                          "Reached state = %d\n", _currentState);
    } // switch(_currentState)
  }
  
  void BehaviorLookForFaces::SetNextMovementTime()
  {
    // TODO: Get the timing variability from config
    const f32 minTime_sec = 0.2f;
    const f32 maxTime_sec = 1.5f;
    _nextMovementTime_sec = _rng.RandDblInRange(minTime_sec, maxTime_sec);
  }
  
  void BehaviorLookForFaces::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
  {
    if(msg.actionType == RobotActionType::COMPOUND && msg.idTag == _movementActionTag)
    {
      // Last movement finished: set next time to move
      SetNextMovementTime();
      if(_currentState == State::MOVING) {
        // Only switch back to looking around state if we didn't see a face
        // while we were moving (and thus are now in FACE_TRACKING state)
        _currentState = State::LOOKING_AROUND;
      }
    }
    else if(msg.actionType == RobotActionType::PLAY_ANIMATION) {
      if(msg.completionInfo.animName != FaceAnimationManager::ProceduralAnimName) {
        PRINT_NAMED_ERROR("BehaviorLookForFaces.HandleRobotCompletedAction.UnexpectedAnimationCompleted",
                          "Unexpected '%s' animation finished.", msg.completionInfo.animName.c_str());
      }
      _animationStarted = false;
      FaceAnimationManager::getInstance()->ClearAnimation(FaceAnimationManager::ProceduralAnimName);
    }
  }
} // namespace Cozmo
} // namespace Anki