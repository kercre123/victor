/**
 * File: behaviorReactToMotion.h
 *
 * Author: ross
 * Created: 2018-03-12
 *
 * Description: behavior that activates upon and turns toward motion, with options for first looking with the eyes
 *              and using either animation or procedural eyes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToMotion__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToMotion__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "util/container/circularBuffer.h"

namespace Anki {
namespace Vector {
  
class ConditionMotionDetected;

class BehaviorReactToMotion : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToMotion();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToMotion(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;

private:
  
  enum class MotionArea : uint8_t {
    None = 0,
    Left,
    Right,
    Top
  };
  
  void TransitionToEyeAnimation( MotionArea area );
  void TransitionToProceduralEyes( MotionArea area );
  void TransitionToTurnedAndWaiting();
  void TransitionToTurnAnimation();
  void TransitionToProceduralTurn();
  void TransitionToCompleted( bool revertHead = true );
  
  void UpdateConditions();
  
  MotionArea GetAreaWithMostMotion() const;
  
  struct AreaCondition {
    AreaCondition(MotionArea a, unsigned int intervalSize, Json::Value& config, const std::string& ownerDebugLabel);
    void Reset();
    
    MotionArea area;
    // keep our own conditions here instead of the base class so that it can be reused once the behavior is activated
    std::shared_ptr<ConditionMotionDetected> condition;
    // if true, motion was spotted the last time UpdateConditions was called
    bool sawMotionLastTick;
    // the last time motion was spotted
    float lastMotionTime_s;
    // % of area covered in motion
    float motionLevel;
    // how many ticks there was motion within some recent interval of ticks
    int intervalCount;
    // a circular buffer to help us compute the above
    Util::CircularBuffer<bool> historyBuff;
  };

  struct InstanceConfig {
    InstanceConfig( const Json::Value& config );

    // if true, uses actions and procedural eyes. if false, uses animation groups
    bool procedural;
    // if eyes have already moved, an no more motion has been detected after this long, exit
    float eyeTimeout_s;
    // if eyes have moved, no motion is detected in that direction for this long, and motion is
    // detected elsewhere, shift there
    float eyeShiftTimeout_s;
    // if there are this many ticks with motion, within in some interval, turn
    unsigned int ticksInIntervalForTurn;
    // and this is the interval size
    unsigned int turnIntervalSize;
    // if procedural turns are used, the amount to turn
    float procTurnAngle_deg;
    // if procedural turns are used, the head angle
    float procHeadAngle_deg;
    
  };
  
  enum class State : uint8_t {
    Invalid=0,
    EyeTracking,
    Turning,
    TurnedAndWaiting,
    Completed
  };

  struct DynamicVariables {
    DynamicVariables();
    void Reset();
    
    State state;
    MotionArea lookingDirection;
    bool proceduralEyeShiftActive;
    bool animationEyeShiftActive;
    bool proceduralHeadUp;
    float timeFinishedTurning_s;
    
    // persistent
    std::vector<AreaCondition> motionConditions;
    bool inMotion; // when the robot is moved or moves. used to reset some stuff or exit when picked up
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToMotion__
