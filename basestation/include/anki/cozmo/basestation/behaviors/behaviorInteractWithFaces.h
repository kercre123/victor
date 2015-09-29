/**
 * File: behaviorInteractWithFaces.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines Cozmo's "InteractWithFaces" behavior, which tracks/interacts with faces if it finds one.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/vision/basestation/trackedFace.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include <list>

namespace Anki {
namespace Cozmo {
  
  template<typename> class AnkiEvent;
  namespace ExternalInterface {
    class MessageEngineToGame;
  }
  
  class BehaviorInteractWithFaces : public IBehavior
  {
  public:
    
    BehaviorInteractWithFaces(Robot& robot, const Json::Value& config);
    
    virtual Result Init(double currentTime_sec) override;
    
    virtual bool IsRunnable(double currentTime_sec) const override;
    
    virtual Status Update(double currentTime_sec) override;
    
    virtual Result Interrupt(double currentTime_sec) override;
    
    virtual const std::string& GetName() const override {
      static const std::string name("InteractWithFaces");
      return name;
    }
    
    virtual bool GetRewardBid(Reward& reward) override;
    
  private:
    using Face = Vision::TrackedFace;
    
    void HandleRobotObservedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    void HandleRobotDeletedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    void HandleRobotCompletedAction(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    
    void UpdateBaselineFace(const Face* face);
    void RemoveFaceID(Face::ID_t faceID);
    void UpdateProceduralFace(ProceduralFace& proceduralFace, const Face& face) const;
    void PlayAnimation(const std::string& animName);
    
    enum class State {
      Inactive,
      TrackingFace,
      Interrupted
    };
    
    State _currentState = State::Interrupted;
    
    f32 _trackingTimeout_sec = 3;
    
    ProceduralFace _lastProceduralFace, _crntProceduralFace;;
    
    f32 _baselineEyeHeight;
    f32 _baselineIntraEyeDistance;
    f32 _baselineLeftEyebrowHeight;
    f32 _baselineRightEyebrowHeight;
    
    u32 _lastActionTag;
    bool _isActing = false;
    double _lastGlanceTime = 0;
    
    std::vector<::Signal::SmartHandle> _eventHandles;
    
    struct FaceData
    {
      double _lastSeen_sec = 0;
      double _trackingStart_sec = 0;
      bool _playedInitAnim = false;
    };
    
    std::list<Face::ID_t> _interestingFacesOrder;
    std::unordered_map<Face::ID_t, FaceData> _interestingFacesData;
    std::unordered_map<Face::ID_t, double> _cooldownFaces;
    
    // Length of time in seconds to keep interacting with the same face non-stop
    constexpr static float kFaceInterestingDuration_sec = 20;
    
    // Length of time in seconds to ignore a specific face that has hit the kFaceInterestingDuration limit
    constexpr static float kFaceCooldownDuration_sec = 20;
    
    // Distance inside of which Cozmo will start noticing a face
    constexpr static float kCloseEnoughDistance_mm = 1500;
    
    // Defines size of zone between "close enough" and "too far away", which prevents faces quickly going back and forth
    // over threshold of close enough or not
    constexpr static float kFaceBufferDistance_mm = 350;
    
    // Distance to trigger Cozmo to start ignoring a face
    constexpr static float kTooFarDistance_mm = kCloseEnoughDistance_mm + kFaceBufferDistance_mm;
    
    // Distance to trigger Cozmo to get further away from the focused face
    constexpr static float kTooCloseDistance_mm = 300;
    
    // Maximum frequency that Cozmo should glance down when interacting with faces (could be longer if he has a stable
    // face to focus on; this interval shouln't interrupt his interaction)
    constexpr static float kGlanceDownInterval_sec = 12;
    
    // Min time between plays of the animation when we see a new face
    constexpr static float kSeeNewFaceAnimationCooldown_sec = 10;
    
  }; // BehaviorInteractWithFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
