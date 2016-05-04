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
        
    virtual bool IsRunnable(const Robot& robot) const override;
    
  protected:
    
    virtual Result InitInternal(Robot& robot) override;
    virtual void   StopInternal(Robot& robot) override;

  private:
    using Face = Vision::TrackedFace;
    using FaceID_t = Vision::FaceID_t;
    
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    
    void HandleRobotObservedFace(const Robot& robot, const EngineToGameEvent& event);
    void HandleRobotDeletedFace(const EngineToGameEvent& event);
    void HandleRobotChangedObservedFaceID(const EngineToGameEvent& event);

    void MarkFaceDeleted(FaceID_t faceID);
      
    enum class State {
      Dispatch,
      GlancingDown,
      RecognizingFace,
      WaitingForRecognition,
      ReactingToFace
    };
    
    State _currentState = State::Dispatch;

    void TransitionToDispatch(Robot& robot);
    void TransitionToGlancingDown(Robot& robot);
    void TransitionToRecognizingFace(Robot& robot);
    void TransitionToWaitingForRecognition(Robot& robot);
    void TransitionToReactingToFace(Robot& robot);
    void TransitionToSwitchingFace(Robot& robot);

    void SetState_internal(State state, const std::string& stateName);

    struct FaceData
    {
      FaceData() = default;
      explicit FaceData(float currTime_sec) : FaceData() {_lastSeen_sec = currTime_sec;}
      float _lastSeen_sec = 0;
      float _coolDownUntil_sec = 0;
      bool  _playedNewFaceAnim = false;
      bool  _deleted = false;
    };

    // gets the faceworld pointer and a pointer to our FaceData (NOTE: do not save this pointer) for current
    // face. Returns true if both were successful, false if there were any problems
    bool GetCurrentFace(const Robot& robot, const Face*& face, FaceData*& faceData);

    float ComputeFaceScore(FaceID_t faceID, const FaceData& faceData) const;

    // return the face id of a face to track, or Vision::UnknownFaceID if there is none
    FaceID_t GetNewFace(const Robot& robot);
    
    // ID of face we are currently interested int
    FaceID_t _currentFace = Vision::UnknownFaceID;
        
    float _lastGlanceTime = 0;

    // Known face data
    std::unordered_map<FaceID_t, FaceData> _interestingFacesData;
        
    // The animation flow is as follows. Anytime we get a new face ID, we do the "take" anim, to give the
    // recognition system some more time. Then if we need more time, we play the _wait group (which can be cut
    // at any time). Then we check if the face data hs _playedNewFaceAnim, and either play the new face group
    // or the known face (unenrolled) group
    std::string _initialTakeAnimGroup;
    std::string _waitAnimGroup;
    std::string _newFaceAnimGroup;
    std::string _unenrolledFaceAnimGroup;

  }; // BehaviorInteractWithFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
