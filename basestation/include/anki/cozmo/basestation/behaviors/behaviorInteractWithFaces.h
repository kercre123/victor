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
    
    virtual bool IsRunnable(double currentTime_sec) const;
    
    virtual Status Update(double currentTime_sec) override;
    
    virtual Result Interrupt(double currentTime_sec) override;
    
    virtual const std::string& GetName() const override {
      static const std::string name("InteractWithFaces");
      return name;
    }
    
    virtual bool GetRewardBid(Reward& reward);
    
  private:
    using Face = Vision::TrackedFace;
    
    void HandleRobotObservedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    void HandleRobotDeletedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    void HandleRobotCompletedAction(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
    
    void UpdateBaselineFace(const Face* face);
    
    void UpdateProceduralFace(ProceduralFace& proceduralFace, const Face& face) const;
    
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
    
    u32 _animationActionTag;
    
    bool _isAnimating = false;
    
    std::vector<::Signal::SmartHandle> _eventHandles;
    
    struct FaceData
    {
      double _lastSeen_sec = 0;
    };
    
    std::list<Face::ID_t> _interestingFacesOrder;
    std::unordered_map<Face::ID_t, FaceData> _interestingFacesData;
    std::unordered_map<Face::ID_t, double> _cooldownFaces;
    
    constexpr static float kFaceInterestingDuration = 30;
    constexpr static float kFaceCooldownDuration = 30;
    constexpr static float kMinProceduralFaceWait = 0.15;
    
  }; // BehaviorInteractWithFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
