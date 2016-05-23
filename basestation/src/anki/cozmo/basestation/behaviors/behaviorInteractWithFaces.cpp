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

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/basestation/faceTracker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

#define DEBUG_BEHAVIOR_INTERACT_WITH_FACES 0

#define SKIP_REQUIRE_KNOWN_FACE 1

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace Anki {
namespace Cozmo {

  using namespace ExternalInterface;

  static const char * const kInitialTakeAnimGroupKey = "initial_take_anim_group";
  static const char * const kWaitAnimGroupKey = "wait_anim_group";
  static const char * const kFaceEnrollRequestEnabledKey = "enable_face_enrollment_request";

  // Length of time in seconds to ignore a specific face that has hit the kFaceInterestingDuration limit
  // NOTE: if this is shorter than the length of the animations, this behavior may get stuck in a loop
  CONSOLE_VAR_RANGED(f32, kFaceCooldownDuration_sec, "Behavior.InteractWithFaces", 6.0f, 0.0f, 20.0f);

  // Distance inside of which Cozmo will start noticing a face
  CONSOLE_VAR_RANGED(f32, kCloseDistance_mm, "Behavior.InteractWithFaces", 1250.0f, 500.0f, 2000.0f);

  // Defines size of zone between "close enough" and "too far away", which prevents faces quickly going back
  // and forth over threshold of close enough or not
  CONSOLE_VAR_RANGED(f32, kFaceBufferDistance_mm, "Behavior.InteractWithFaces", 350.0f, 0.0f, 500.0f);

  // Maximum frequency that Cozmo should glance down when interacting with faces (could be longer if he has a
  // stable face to focus on; this interval shouln't interrupt his interaction)
  CONSOLE_VAR_RANGED(f32, kGlanceDownInterval_sec, "Behavior.InteractWithFaces", 12.0f, -1.0f, 30.0f);

  CONSOLE_VAR(u32, kRequestDelayNumLoops, "Behavior.InteractWithFaces", 5);

  CONSOLE_VAR_RANGED(f32, kEnrollRequestCooldownInterval_s, "Behavior.InteractWithFaces", 10.0f, 0.0f, 30.0f);

  BehaviorInteractWithFaces::BehaviorInteractWithFaces(Robot &robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    SetDefaultName("Faces");

    _initialTakeAnimGroup = config.get(kInitialTakeAnimGroupKey, "").asString();
    _waitAnimGroup = config.get(kWaitAnimGroupKey, "").asString();

    _faceEnrollEnabled = config.get(kFaceEnrollRequestEnabledKey, false).asBool();
    
    SubscribeToTags({{
      EngineToGameTag::RobotObservedFace,
      EngineToGameTag::RobotDeletedFace,
      EngineToGameTag::RobotChangedObservedFaceID
    }});

    SubscribeToTags({
      GameToEngineTag::DenyGameStart
    });

    if( SKIP_REQUIRE_KNOWN_FACE ) {
      PRINT_NAMED_WARNING("BehaviorInteractWithFaces.DEMO",
                          "Disabling requirement to see a known face in order to enroll. This needs to be changed for the demo at some point");
      _readyToEnrollFace = true;
    }
    
    // - - - -
    // parse smart score
    // - - - -
    // _smartScore
    const Json::Value& smartScoreConfig = config["smartScore"];
    if ( !smartScoreConfig.isNull() ) {
      _smartScore._isValid = true; // something is defined, trust data
      _smartScore._whileRunning = JsonTools::ParseFloat(smartScoreConfig, "flatScore_whileRunning", "BehaviorInteractWithFaces");
      _smartScore._newKnownFace = JsonTools::ParseFloat(smartScoreConfig, "flatScore_newKnownFace", "BehaviorInteractWithFaces");
      _smartScore._unknownFace  = JsonTools::ParseFloat(smartScoreConfig, "flatScore_unknownFace", "BehaviorInteractWithFaces");
      _smartScore._unknownFace_cooldown = JsonTools::ParseFloat(smartScoreConfig, "flatScore_unknownFace_cooldown", "BehaviorInteractWithFaces");
      _smartScore._oldKnownFace = JsonTools::ParseFloat(smartScoreConfig, "flatScore_oldKnownFace", "BehaviorInteractWithFaces");
      _smartScore._oldKnownFace_secondsToVal = JsonTools::ParseFloat(smartScoreConfig, "flatScore_oldKnownFace_secondsToVal", "BehaviorInteractWithFaces");
    }
  }
  
  Result BehaviorInteractWithFaces::InitInternal(Robot& robot)
  {
    TransitionToDispatch(robot);
    return RESULT_OK;
  }
  
  BehaviorInteractWithFaces::~BehaviorInteractWithFaces()
  {
    
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BehaviorInteractWithFaces::IsRunnableInternal(const Robot& robot) const
  {
    // runnable if there are any faces that we might want to interact with
    
    bool isRunnable = false;
    for(auto & faceData : _interestingFacesData) {
      if( ComputeFaceScore(faceData.first, faceData.second) > 0.0f ) {
        isRunnable = true;
        break;
      }
    }

    return isRunnable;
  }

  bool BehaviorInteractWithFaces::GetCurrentFace(const Robot& robot, const Face*& face, FaceData*& faceData)
  {
    if( _currentFace == Vision::UnknownFaceID ) {
      return false;
    }
    
    face = robot.GetFaceWorld().GetFace(_currentFace);
    if(nullptr == face) {
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.NullFace",
                        "FaceWorld returned null face for ID %d", _currentFace);
      MarkFaceDeleted(_currentFace);
      _currentFace = Vision::UnknownFaceID;
      return false;
    }
    
    auto dataIter = _interestingFacesData.find(_currentFace);
    if (_interestingFacesData.end() == dataIter)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.MissingInteractionData",
                        "Failed to find interaction data associated with faceID %d", _currentFace);
      return false;
    }
    faceData = &(dataIter->second);

    return true;
  }

  void BehaviorInteractWithFaces::TransitionToDispatch(Robot& robot)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
            
    SET_STATE(Dispatch);

    // first check if we should glance down
    if (kGlanceDownInterval_sec > 0 && currentTime_sec - _lastGlanceTime >= kGlanceDownInterval_sec)
    {
      TransitionToGlancingDown(robot);
      return;
    }

    // figure out which face to track
    if( _interestingFacesData.empty() ) {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.NoFace",
                        "InteractWithFaces is running, but there are no interesting faces");
      return;
    }

    FaceID_t bestFace = GetNewFace(robot);

    if( bestFace == Vision::UnknownFaceID ) {
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.NoGoodFace",
                        "we have %zu faces but none have positive score (all are probably on cooldown)",
                        _interestingFacesData.size());
      return;
    }

    _currentFace = bestFace;
    
    TransitionToRecognizingFace(robot);
  }


  Vision::FaceID_t BehaviorInteractWithFaces::GetNewFace(const Robot& robot)
  {
    FaceID_t bestFace = Vision::UnknownFaceID;
    float bestScore = 0.0f;

    for( const auto& facePair : _interestingFacesData ) {
      float score = ComputeFaceScore(facePair.first, facePair.second);
      BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_INTERACT_WITH_FACES,
                             "BehaviorInteractWithFaces.ScoreFace",
                             "face %d -> %f",
                             facePair.first,
                             score);
      
      if( score > bestScore ) {
        bestScore = score;
        bestFace = facePair.first;
      }
    }

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_INTERACT_WITH_FACES,
                           "BehaviorInteractWithFaces.Dispatch",
                           "selecting Face %d (with score of %f)",
                           bestFace,
                           bestScore);

    return bestFace;
  }


  float BehaviorInteractWithFaces::ComputeFaceScore(FaceID_t faceID, const FaceData& faceData) const
  {
    // use new score system from json config
    const float faceScore = ComputeFaceSmartScore(faceID, faceData);
    return faceScore;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  float BehaviorInteractWithFaces::ComputeFaceSmartScore(FaceID_t faceID, const FaceData& faceData) const
  {
    // classes:
    // + deleted/cooldown
    // + known - new
    // + known - old
    // + unknown
  
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( currentTime_sec < faceData._coolDownUntil_sec || faceData._deleted ) {
      return -1.0f;
    }
        
    float score = 0.0f;
    const bool isKnown = ( faceID > 0 );
    const bool hasInteracted = FLT_GT(faceData._coolDownUntil_sec, 0.0f);
    if ( isKnown )
    {
      if ( hasInteracted )
      {
        float factor = 1.0f;
        const bool doFactorOverTime = FLT_GT(_smartScore._oldKnownFace_secondsToVal, 0.0f);
        if ( doFactorOverTime ) {
          // get the factor [0,1] depending on time passed and how long it takes to reach the full value
          float timePassed = currentTime_sec - faceData._coolDownUntil_sec;
          factor = Util::Clamp(timePassed / _smartScore._oldKnownFace_secondsToVal, 0.0f, 1.0f);
        }
        // KNOWN OLD
        score = factor * _smartScore._oldKnownFace;
      }
      else
      {
        // KNOWN NEW
        score = _smartScore._newKnownFace;
      }
    }
    else
    {
      // UNKNOWN
      ASSERT_NAMED(!hasInteracted, "BehaviorInteractWithFaces.UnknownFaceHasBeenSeen");
      float timeSinceLastStop = currentTime_sec - _lastInteractionEndedTime_sec;
      if ( timeSinceLastStop > _smartScore._unknownFace_cooldown ) {
        score = _smartScore._unknownFace;
      } else {
        // we have interacted recently, we don't need to chase unknown faces yet, keep chilling to see
        // if they become known, which would give us more information on cooldowns and desires
        score = 0.0f;
      }
    }

    return score;
  }
 
  void BehaviorInteractWithFaces::TransitionToGlancingDown(Robot& robot)
  {
    SET_STATE(GlancingDown);
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    _lastGlanceTime = currentTime_sec;

    float headAngle = robot.GetHeadAngle();
    if(headAngle > 0.f) { // don't bother if we're already at or below zero degrees
      // Move head down to check for a block, then look back up.
      CompoundActionSequential* moveHeadAction = new CompoundActionSequential(robot, {
          new MoveHeadToAngleAction(robot, 0.),
          new MoveHeadToAngleAction(robot, headAngle),
          });

      StartActing(moveHeadAction, &BehaviorInteractWithFaces::TransitionToDispatch);
    }
    else {
      // go back to dispatch since we don't need to look down
      TransitionToDispatch(robot);
    }
  }

  void BehaviorInteractWithFaces::TransitionToRecognizingFace(Robot& robot)
  {
    SET_STATE(RecognizingFace);
    
    const Face* face = nullptr;
    FaceData* faceData = nullptr;
    if( ! GetCurrentFace(robot, face, faceData) ) {
      return;
    }

    CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        
    // Always turn to look at the face before any reaction
    TurnTowardsPoseAction* turnTowardsPoseAction = new TurnTowardsPoseAction(robot,
                                                                             face->GetHeadPose(),
                                                                             DEG_TO_RAD(179));
    turnTowardsPoseAction->SetPanTolerance( DEG_TO_RAD(2.0) );

    compoundAction->AddAction(turnTowardsPoseAction);
    compoundAction->AddAction( new PlayAnimationGroupAction(robot, _initialTakeAnimGroup) );

    StartActing(compoundAction, &BehaviorInteractWithFaces::TransitionToWaitingForRecognition);
  }

  void BehaviorInteractWithFaces::TransitionToWaitingForRecognition(Robot& robot)
  {
    SET_STATE(WaitingForRecognition);

    // check if we know the face yet. If it is still unknown (negative id) keep waiting, otherwise skip to
    // reacting

    if( _currentFace == Vision::UnknownFaceID ) {
      return;
    }

    // TODO:(bn/as) use some confidence here? Want to wait to see if a "new session only" id is going to merge
    // into a known name
    if( _currentFace < 0 ) {
      // continue to look
      const Face* face = nullptr;
      FaceData* faceData = nullptr;
      if( ! GetCurrentFace(robot, face, faceData) ) {
        return;
      }

      CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        
      // Always turn to look at the face before any reaction
      TurnTowardsPoseAction* turnTowardsPoseAction = new TurnTowardsPoseAction(robot,
                                                                               face->GetHeadPose(),
                                                                               DEG_TO_RAD(179));
      turnTowardsPoseAction->SetPanTolerance( DEG_TO_RAD(2.0) );

      compoundAction->AddAction(turnTowardsPoseAction);
      compoundAction->AddAction( new PlayAnimationGroupAction(robot, _waitAnimGroup) );

      StartActing(compoundAction, &BehaviorInteractWithFaces::TransitionToReactingToFace);
    }
    else {
      if( ShouldEnrollCurrentFace(robot) ) {
        TransitionToRequestingFaceEnrollment(robot);
      }
      else {
        TransitionToReactingToFace(robot);
      }
    }
  }


  void BehaviorInteractWithFaces::TransitionToReactingToFace(Robot& robot)
  {
    SET_STATE(ReactingToFace);
    
    const Face* face = nullptr;
    FaceData* faceData = nullptr;
    if( ! GetCurrentFace(robot, face, faceData) ) {
      return;
    }
    
    CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        
    // Always turn to look at the face before any reaction. Even if we were already looking at it, this will
    // serve to adjust the position
    compoundAction->AddAction(new TurnTowardsPoseAction(robot, face->GetHeadPose(), DEG_TO_RAD(179)));

    if( !faceData->_playedNewFaceAnim ) {
      if( face->GetName().empty() ) {
        compoundAction->AddAction( new PlayAnimationGroupAction(robot, GameEvent::OnSawNewUnnamedFace) );
        robot.GetMoodManager().TriggerEmotionEvent("NewUnnamedFace", MoodManager::GetCurrentTimeInSeconds());
      }
      else {
        // He's happy to see you so play an anim first
        compoundAction->AddAction( new PlayAnimationGroupAction(robot, GameEvent::OnWiggle) );
        SayTextAction* sayTextAction = new SayTextAction(robot, face->GetName(), SayTextStyle::Name_Normal, true);
        sayTextAction->SetGameEvent(Anki::Cozmo::GameEvent::OnSawNewNamedFace);
        compoundAction->AddAction( sayTextAction );
        robot.GetMoodManager().TriggerEmotionEvent("NewNamedFace", MoodManager::GetCurrentTimeInSeconds());

        if( _faceEnrollEnabled ) {
          _readyToEnrollFace = true;
        }
      }

      faceData->_playedNewFaceAnim = true;
      // NOTE: this is a bit wrong because something might interrupt this from happening, in which case it
      // didn't actually play the new animation, but its much simpler this way than to handle what might
      // happen if face ids change or merge before the action is complete

    }
    else {
      if( face->GetName().empty() ) {
        compoundAction->AddAction( new PlayAnimationGroupAction(robot, GameEvent::OnSawOldUnnamedFace) );
        robot.GetMoodManager().TriggerEmotionEvent("OldUnnamedFace", MoodManager::GetCurrentTimeInSeconds());
        EnumToString(GameEvent::OnSawNewNamedFace);
      }
      else {
        SayTextAction* sayTextAction = new SayTextAction(robot, face->GetName(), SayTextStyle::Name_Normal, true);
        sayTextAction->SetGameEvent(Anki::Cozmo::GameEvent::OnSawOldNamedFace);
        compoundAction->AddAction( sayTextAction );
        robot.GetMoodManager().TriggerEmotionEvent("OldNamedFace", MoodManager::GetCurrentTimeInSeconds());
        
        if( _faceEnrollEnabled ) {
          _readyToEnrollFace = true;
        }
      }
    }

    StartActing(compoundAction, [this](Robot& robot) {
        const Face* waste;
        FaceData* data;
        if(GetCurrentFace(robot, waste, data)) {
          const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          data->_coolDownUntil_sec = currentTime_sec + kFaceCooldownDuration_sec;
          PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.Cooldown",
                            "face %d is on cooldown until t=%f",
                            _currentFace,
                            data->_coolDownUntil_sec);
        }
      
        // StopInternal also sets this time, however, we want to use if for scoring, and StopInternal won't
        // be called until we have already been kicked out as the running behavior, which won't probably happen
        // if we don't set this cooldown now
        const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _lastInteractionEndedTime_sec = currentTime_sec;

        // We should give up IsRunning after a successful iteration, to let chooser pick something else
        // without having to interrupt us
        // TransitionToDispatch(robot);
      });
  }

  void BehaviorInteractWithFaces::TransitionToRequestingFaceEnrollment(Robot& robot)
  {
    SET_STATE(RequestingFaceEnrollment);

    robot.Broadcast( MessageEngineToGame( RequestEnrollFace(true, _currentFace) ) );

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _requestEnrollOnCooldownUntil_s = currTime_s + kEnrollRequestCooldownInterval_s;

    // play a few loops of the wait animation. If this completes without a response, this is a time out
    // condition, so go to the usual reaction (which should be the unenrolled one). We expect to either be
    // interrupted by a denied message, or to be stopped by unity to run the activity
    StartActing(new PlayAnimationGroupAction(robot, _waitAnimGroup, kRequestDelayNumLoops),
                [this](Robot& robot) {
                  // this is a timeout, so let the UI know
                  SendDenyRequest(robot);
                  TransitionToReactingToFace(robot);
                });
  }
  
  void BehaviorInteractWithFaces::StopInternal(Robot& robot)
  {
    if( _currentState == State::RequestingFaceEnrollment ) {
      // behavior was stopped during a request, cancel the request
      SendDenyRequest(robot);
    }
    _currentState = State::Dispatch;
    
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _lastInteractionEndedTime_sec = currentTime_sec;
  }
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorInteractWithFaces::EvaluateRunningScoreInternal(const Robot& robot) const
{
  float ret = 0.0f;
  if ( _smartScore._isValid ) {
    ret = _smartScore._whileRunning;
  } else {
    ret = BaseClass::EvaluateRunningScoreInternal(robot);
  }

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorInteractWithFaces::EvaluateScoreInternal(const Robot& robot) const
{
  float ret = 0.0f;
  if ( _smartScore._isValid )
  {
    // the score for the behavior is that of the best face's score
    for( const auto& facePair : _interestingFacesData )
    {
      float score = ComputeFaceSmartScore(facePair.first, facePair.second);
      if ( score > ret ) {
        ret = score;
      }
    }
  }
  else
  {
    ret = BaseClass::EvaluateScoreInternal(robot);
  }
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BehaviorInteractWithFaces::ShouldEnrollCurrentFace(const Robot& robot)
  {
    if( !_readyToEnrollFace ) {
      return false;
    }

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    if( _requestEnrollOnCooldownUntil_s >= 0.0f && currTime_s < _requestEnrollOnCooldownUntil_s ) {
      return false;
    }
      
    // never enroll "temporary" face IDs
    if( _currentFace <= 0 ) {
      return false;
    }
    
    const Face* face = nullptr;
    FaceData* faceData = nullptr;
    if( ! GetCurrentFace(robot, face, faceData) ) {
      return false;
    }
    
    // do not enroll if face is too far
    const float kMinEyeDistanceForEnrollment = Vision::FaceTracker::GetMinEyeDistanceForEnrollment();
    const float eyeDistance = face->GetIntraEyeDistance();
    if ( eyeDistance < kMinEyeDistanceForEnrollment ) {
      return false;
    }

    // TODO:(bn) eventually this should have some logic to check if a "slot" is available, or however this
    // will work in the shipping game

    // if we've said a name we know and we are looking at a face we don't know, enroll it
    return face->GetName().empty();
  }


#pragma mark -
#pragma mark Signal Handlers

  
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
        
      case EngineToGameTag::RobotChangedObservedFaceID:
        HandleRobotChangedObservedFaceID(event);
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorInteractWithFaces.AlwaysHandle.InvalidTag",
                          "Received event with unhandled tag %hhu.",
                          event.GetData().GetTag());
        break;
    }
  }

  void BehaviorInteractWithFaces::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
  {
    if( event.GetData().GetTag() == GameToEngineTag::DenyGameStart ) {
      HandleGameDeniedRequest(robot);
    }
    else {
      PRINT_NAMED_WARNING("IBehaviorRequestGame.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
    }
  }
  
  void BehaviorInteractWithFaces::HandleRobotObservedFace(const Robot& robot, const EngineToGameEvent& event)
  {    
    assert(event.GetData().GetTag() == EngineToGameTag::RobotObservedFace);

    const RobotObservedFace& msg = event.GetData().Get_RobotObservedFace();

    FaceID_t faceID = static_cast<FaceID_t>(msg.faceID);

    // PRINT_NAMED_DEBUG("BehaviorInteractWithFaces::HandleRobotObservedFace",
    //                   "observed %d at t=%f",
    //                   faceID,
    //                   event.GetCurrentTime());

    // We need a valid face to work with
    const Face* face = robot.GetFaceWorld().GetFace(faceID);
    if(face == nullptr)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.InvalidFaceID",
                        "Got event that face ID %d was observed, but it wasn't found.", faceID);
      return;
    }
        
    Pose3d headPose;
    bool gotPose = face->GetHeadPose().GetWithRespectTo(robot.GetPose(), headPose);
    if (!gotPose)
    {
      PRINT_NAMED_ERROR("BehaviorInteractWithFaces.HandleRobotObservedFace.InvalidFacePose",
                        "Could not get head pose of face ID %d w.r.t. robot.", faceID);

      return;
    }

    // Determine if head is close enough to bother with
    Vec3f distVec = headPose.GetTranslation();
    distVec.z() = 0;
    const float farDistance_mm   = kCloseDistance_mm + kFaceBufferDistance_mm;
    const float distSq           = distVec.LengthSq();
    const bool  withinCloseRange = distSq < (kCloseDistance_mm * kCloseDistance_mm);
    const bool  outsideFarRange  = distSq > (farDistance_mm * farDistance_mm);

    auto dataIter = _interestingFacesData.find(faceID);

    if (_interestingFacesData.end() != dataIter)
    {
      // This is a face we already knew about. If it's close enough, update the last seen time. If not, remove
      // it.
      if(!outsideFarRange) {
        dataIter->second._lastSeen_sec = event.GetCurrentTime();
        dataIter->second._deleted = false;
      } else {
        PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.RemoveFace",
                          "face %d is too far (%f > %f), marking as deleted",
                          faceID,
                          distVec.Length(),
                          farDistance_mm);
        MarkFaceDeleted(faceID);
        return;
      }
    } else if(withinCloseRange) {
      // This is not a face we already knew about, but it's close enough. Add it as one we could choose to
      // track
      _interestingFacesData.insert( { faceID, FaceData(event.GetCurrentTime()) } );
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.NewFace",
                        "Added new face %d observed at t=%f",
                        faceID,
                        event.GetCurrentTime());
    }
    
  } // HandleRobotObservedFace()
  
  
  void BehaviorInteractWithFaces::HandleRobotDeletedFace(const EngineToGameEvent& event)
  {    
    const RobotDeletedFace& msg = event.GetData().Get_RobotDeletedFace();

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_INTERACT_WITH_FACES,
                           "BehaviorInteractWithFaces::HandleRobotDeletedFace",
                           "deleted face %d",
                           msg.faceID);
    
    // if it was a temp face, delete it for real, otherwise keep it around but mark it deleted
    FaceID_t faceID = static_cast<FaceID_t>(msg.faceID);
    if( msg.faceID < 0 ) {
      auto dataIter = _interestingFacesData.find(faceID);
      if( dataIter != _interestingFacesData.end() ) {
        _interestingFacesData.erase(dataIter);
      }
    }
    else {
      MarkFaceDeleted(faceID);
    }
  }

  void BehaviorInteractWithFaces::HandleRobotChangedObservedFaceID(const EngineToGameEvent& event)
  {
    const RobotChangedObservedFaceID& msg = event.GetData().Get_RobotChangedObservedFaceID();

    BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_INTERACT_WITH_FACES,
                           "BehaviorInteractWithFaces::HandleRobotChangedObservedFaceID",
                           "%d -> %d",
                           msg.oldID,
                           msg.newID);

    const FaceID_t oldFaceID = static_cast<FaceID_t>(msg.oldID);
    const FaceID_t newFaceID = static_cast<FaceID_t>(msg.newID);

    auto oldIt = _interestingFacesData.find(oldFaceID);
    const auto& newIt = _interestingFacesData.find(newFaceID);

    // we should have data about at least once face
    if( oldIt == _interestingFacesData.end() && newIt == _interestingFacesData.end() ) {
      PRINT_NAMED_INFO("BehaviorInteractWithFaces.ChangeID.NoData",
                       "Got message that robot change face ID %d to %d, but dont have data about either",
                       oldFaceID,
                       newFaceID);
      return;
    }
    
    // if we don't have any data about the new face, this is simply a "rename"
    if( newIt == _interestingFacesData.end() ) {
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.FaceRename",
                        "face ID %d now known by %d",
                        oldFaceID,
                        newFaceID);
      _interestingFacesData.insert( std::make_pair(newFaceID, oldIt->second) );
      _interestingFacesData.erase(oldIt);
      oldIt = _interestingFacesData.end(); // clear the old iterator
    }
    else if( oldIt != _interestingFacesData.end() ) {
      // this is a merge because we had data for both
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.FaceMerge",
                        "face ID %d merged into %d",
                        oldFaceID,
                        newFaceID);

      newIt->second._lastSeen_sec = std::max(oldIt->second._lastSeen_sec, newIt->second._lastSeen_sec);
      newIt->second._coolDownUntil_sec = std::max(oldIt->second._coolDownUntil_sec, newIt->second._coolDownUntil_sec);

      // if we played a new face anim for either face, then we played it for the new face
      newIt->second._playedNewFaceAnim = newIt->second._playedNewFaceAnim || oldIt->second._playedNewFaceAnim;

      // if the old id was unknown and the new id is known, and we are in the Wait state (where the robot is
      // squinting to buy time), then stop the current action
      if( _currentState == State::WaitingForRecognition &&
          oldIt->first == _currentFace &&
          oldIt->first < 0 &&
          newIt->first > 0 ) {
        PRINT_NAMED_INFO("BehaviorInteractWithFaces.RecognizedWhileWaiting",
                         "stopping action so we can play recognition animation");
        StopActing();
      }

      _interestingFacesData.erase(oldIt);
      oldIt = _interestingFacesData.end(); // clear the old iterator
    }
    // else, we have data about the new face but not the old one, so ignore this message because there is
    // nothing to do about it


    // if we are changing the old face, update current face as well
    if( _currentFace != Vision::UnknownFaceID && _currentFace == oldFaceID ) {
      _currentFace = newFaceID;
      PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.UpdateCurrentFace",
                        "Updating current face because of merge");
    }
  }

  void BehaviorInteractWithFaces::HandleGameDeniedRequest(Robot& robot)
  {
    if( _currentState == State::RequestingFaceEnrollment ) {
      // stop the animation action (with no callback)
      StopActing(false);
      // go to the reacting state, which should choose the "suspicious" animation for an unknown person
      TransitionToReactingToFace(robot);
    }
  }
  
  void BehaviorInteractWithFaces::MarkFaceDeleted(FaceID_t faceID)
  {
    auto dataIter = _interestingFacesData.find(faceID);
    if (_interestingFacesData.end() != dataIter)
    {
      dataIter->second._deleted = true;
    }    
  }

  void BehaviorInteractWithFaces::SendDenyRequest(Robot& robot)
  {
    using namespace ExternalInterface;

    // update request cooldown
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _requestEnrollOnCooldownUntil_s = currTime_s + kEnrollRequestCooldownInterval_s;

    robot.Broadcast( MessageEngineToGame( DenyGameStart() ) );
  }


  void BehaviorInteractWithFaces::SetState_internal(State state, const std::string& stateName)
  {
    _currentState = state;
    PRINT_NAMED_DEBUG("BehaviorInteractWithFaces.TransitionTo", "%s", stateName.c_str());
    std::string debugString = "[" +std::to_string(_currentFace) + "]" + stateName;
    SetStateName(debugString);
  }

  
} // namespace Cozmo
} // namespace Anki
