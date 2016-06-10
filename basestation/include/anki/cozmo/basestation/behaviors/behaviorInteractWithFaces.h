/**
 * File: behaviorInteractWithFaces.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 * overhaul: Brad Neuman 2016-04-21
 *
 * Description: Defines Cozmo's "InteractWithFaces" behavior, which tracks/interacts with faces if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include <string>
#include <unordered_map>

namespace Anki {

namespace Vision {
class TrackedFace;
}

namespace Cozmo {
  
  class BehaviorInteractWithFaces : public IBehavior
  {
  protected:
    
    // Enforce creation through BehaviorFactory
    friend class BehaviorFactory;
    BehaviorInteractWithFaces(Robot& robot, const Json::Value& config);
    virtual ~BehaviorInteractWithFaces() override;
    
  public:
        
    virtual bool IsRunnableInternal(const Robot& robot) const override;
    
  protected:

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // IBehavior API
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    virtual Result InitInternal(Robot& robot) override;
    virtual void   StopInternal(Robot& robot) override;
    
    // score
    virtual float EvaluateRunningScoreInternal(const Robot& robot) const override;
    virtual float EvaluateScoreInternal(const Robot& robot) const override;

  private:
  
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Types
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    using BaseClass = IBehavior;
    
    // this is a struct to help score smarter for the demo. Ideally every type of reaction should be its own
    // behavior. Then it would be easy to set different scores and cooldowns, for example to score high with low cooldown
    // for interacting with a new face, but having low score or high cooldown to interact with old ones. All that
    // complexity is here because this behavior does everything.
    struct SmartFaceScore {
      bool  _isValid      = false;
      float _whileRunning = 0.0f;   // score while running
      float _newKnownFace = 0.0f;   // score if we have a new known face
      float _unknownFace  = 0.0f;   // score if we don't have a new known face, but we have an unknown
      float _unknownFace_cooldown = 0.0f; // cooldown applied to not bounce between unknown faces all the time
      float _unknownFace_bonusToDistanceTo1 = 0.0f; // factor applied to distance to face to give bonus to closer ones. MinDistance will get this bonus, MaxDistance gets 0
      float _oldKnownFace = 0.0f;   // score if we don't have new known or unknown faces, but we have old known
      float _oldKnownFace_secondsToVal = 0.0f; // seconds it takes _oldKnownFace to reach the value (from 0)
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
    using Face = Vision::TrackedFace;
    using FaceID_t = Vision::FaceID_t;
    
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) final override;
    
    void HandleRobotObservedFace(const Robot& robot, const EngineToGameEvent& event);
    void HandleRobotDeletedFace(const EngineToGameEvent& event);
    void HandleRobotChangedObservedFaceID(const EngineToGameEvent& event);
    void HandleGameDeniedRequest(Robot& robot);
    void HandleEnrollNamedFaceCompleted(const EngineToGameEvent& event);
    
    void MarkFaceDeleted(FaceID_t faceID);
      
    enum class State {
      Dispatch,
      GlancingDown,
      RecognizingFace,
      WaitingForRecognition,
      ReactingToFace,
      RequestingFaceEnrollment
    };
    
    State _currentState = State::Dispatch;

    void TransitionToDispatch(Robot& robot);
    void TransitionToGlancingDown(Robot& robot);
    void TransitionToRecognizingFace(Robot& robot);
    void TransitionToWaitingForRecognition(Robot& robot);
    void TransitionToReactingToFace(Robot& robot);
    void TransitionToRequestingFaceEnrollment(Robot& robot);

    void SetState_internal(State state, const std::string& stateName);

    bool ShouldEnrollCurrentFace(const Robot& robot);

    void SendDenyRequest(Robot& robot);
    
    struct FaceData
    {
      FaceData() = default;
      explicit FaceData(float currTime_sec) : FaceData()
      {
        _lastSeen_sec = currTime_sec;
      }
      float _lastSeen_sec = 0;
      float _coolDownUntil_sec = 0;
      int   _numTimesSeenFrontal = 0;
      bool  _playedNewFaceAnim = false;
      bool  _deleted = false;
    };

    // gets the faceworld pointer and a pointer to our FaceData (NOTE: do not save this pointer) for current
    // face. Returns true if both were successful, false if there were any problems
    bool GetCurrentFace(const Robot& robot, const Face*& face, FaceData*& faceData);

    float ComputeFaceScore(FaceID_t faceID, const FaceData& faceData, const Robot& robot) const;
    float ComputeFaceSmartScore(FaceID_t faceID, const FaceData& faceData, const Robot& robot) const;
    float ComputeDistanceBonus(FaceID_t faceID, const Robot& robot) const;
    
    // return the face id of a face to track, or Vision::UnknownFaceID if there is none
    FaceID_t GetNewFace(const Robot& robot);

    ////////////////////////////////////////////////////////////////////////////////
    // Members
    ////////////////////////////////////////////////////////////////////////////////

    // ID of face we are currently interested int
    FaceID_t _currentFace = Vision::UnknownFaceID;
    s32 _currentFaceNumTimesSeen = 0;

    float _requestEnrollOnCooldownUntil_s = -1.0f;

    float _lastGlanceTime = 0;
    
    float _lastInteractionEndedTime_sec = 0.0f;

    // Known face data
    std::unordered_map<FaceID_t, FaceData> _interestingFacesData;

    ////////////////////////////////////////////////////////////////////////////////
    // Config parameters
    ////////////////////////////////////////////////////////////////////////////////
    
    // smart score
    SmartFaceScore _smartScore;
   
    // config can specify whether or not we do this at all
    bool _faceEnrollEnabled;
    
    // The animation flow is as follows. Anytime we get a new face ID, we do the "take" anim, to give the
    // recognition system some more time. Then if we need more time, we play the _wait group (which can be cut
    // at any time). Then we check if the face data has _playedNewFaceAnim, and either play a new group or a
    // regular group. In either case, we play named or unnamed based on whether or not the face has an
    // associated name. The Named or Unnamed are part of the "GameEvents" clad enum
    std::string _initialTakeAnimGroup;
    std::string _waitAnimGroup;

  }; // BehaviorInteractWithFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
