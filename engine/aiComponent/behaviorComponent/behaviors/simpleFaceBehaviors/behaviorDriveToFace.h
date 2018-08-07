/**
* File: behaviorDriveToFace.h
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that drives to a face from voice command
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Vector {
  
class BehaviorFindFaceAndThen;
struct PathMotionProfile;

class BehaviorDriveToFace : public ICozmoBehavior
{
public:
  void SetInterruptionEndTick( size_t tick );
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDriveToFace(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  
  enum class State {
    Invalid=0,
    FindingFace,
    DriveToFace,
    FinalAnim,
    AlreadyCloseEnough,
    TrackFace,
    ResumeAnim,
  };
  
  struct InstanceConfig {
    InstanceConfig();
    
    float timeUntilCancelTracking_s;
    
    float minDriveToFaceDistance_mm;
    
    bool trackFaceOnceKnown; // if true, this behavior spends time tracking the face after driving up to it
    
    AnimationTrigger animTooClose;
    AnimationTrigger animDriveOverride;
    AnimationTrigger initialAnim;
    AnimationTrigger successAnim;
    AnimationTrigger resumeAnim;
    
    int maxNumResumes;
    
    std::string findFaceBehaviorStr;
    std::shared_ptr<BehaviorFindFaceAndThen> findFaceBehavior;
    
    std::unique_ptr<PathMotionProfile> customMotionProfile;
  };

  struct DynamicVariables {
    DynamicVariables();
    State currentState;
    
    float trackEndTime_s;
    
    int numResumesRemaining;
    
    SmartFaceID targetFace;
    
    size_t interruptionEndTick;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void TransitionToAlreadyCloseEnough();
  
  void TransitionToFindingFace();
  void TransitionToDrivingToFace();
  void TransitionToPlayingFinalAnim();
  void TransitionToTrackingFace();
  void TransitionToResumeAnimation();
  
  // Returns true if able to calculate distance to face - false otherwise
  bool CalculateDistanceToFace(Vision::FaceID_t faceID, float& distance);
  
  void SetState_internal(State state, const std::string& stateName);
  
  // Returns true if the robot is close enough to the face that he won't actually
  // drive forward when this behavior runs
  bool IsRobotAlreadyCloseEnoughToFace(Vision::FaceID_t faceID);

};


} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__
