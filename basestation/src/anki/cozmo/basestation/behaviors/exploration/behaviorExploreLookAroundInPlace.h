/**
 * File: behaviorExploreLookAroundInPlace
 *
 * Author: Raul
 * Created: 03/11/16
 *
 * Description: Behavior for looking around the environment for stuff to interact with. This behavior starts
 * as a copy from behaviorLookAround, which we expect to eventually replace. The difference is this new
 * behavior will simply gather information and put it in a place accessible to other behaviors, rather than
 * actually handling the observed information itself.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorExploreLookAroundInPlace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorExploreLookAroundInPlace_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"

#include <list>
#include <set>
#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace ExternalInterface {
struct RobotObservedObject;
}
class IAction;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreLookAroundInPlace
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorExploreLookAroundInPlace : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreLookAroundInPlace(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreLookAroundInPlace() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // todo: document. Is this behavior alway runnable, or we won't look around in an area we already know everything?
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override { return 1.0f; }
  
protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
  virtual IBehavior::Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override { return Result::RESULT_OK; } // TODO?
  virtual void StopInternal(Robot& robot, double currentTime_sec) override {} // TODO?
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void TransitionToS1_OppositeTurn(Robot& robot);
  void TransitionToS2_Pause(Robot& robot);
  void TransitionToS3_MainTurn(Robot& robot);
  void TransitionToS4_HeadOnlyUp(Robot& robot);
  void TransitionToS5_HeadOnlyDown(Robot& robot);
  void TransitionToS6_MainTurnFinal(Robot& robot);
  void TransitionToS7_IterationEnd(Robot& robot);
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Turn direction
  enum class EClockDirection { CW, CCW };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // get the sign of the turn depending on clock direction
  static inline float GetTurnSign(const EClockDirection direction) { return (direction == EClockDirection::CW) ? -1.0f : 1.0f; }

  // get the opposite turn/clock direction to the one given
  static inline EClockDirection GetOpposite(const EClockDirection direction) {
    return (direction == EClockDirection::CW) ? EClockDirection::CCW : EClockDirection::CW; }

  // request the proper action given the parameters so that the robot turns and moves head
  IAction* CreateBodyAndHeadTurnAction(Robot& robot, EClockDirection clockDirection,  // body turn clock direction
            float bodyStartRelativeMin_deg, float bodyStartRelativeMax_deg,           // body min/max range relative to starting angle (from original state)
            float headAbsoluteMin_deg, float headAbsoluteMax_deg,                     // head min/max range absolute
            float bodyTurnSpeed_degPerSec, float headTurnSpeed_degPerSec);            // turn speeds
  
  // request the proper action given the parameters so that the robot moves its head
  IAction* CreateHeadTurnAction(Robot& robot,
            float headAbsoluteMin_deg, float headAbsoluteMax_deg,                     // head min/max range absolute
            float headTurnSpeed_degPerSec);                                           // turn speed

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // facing direction when we start an iteration
  Radians _iterationStartingBodyFacing_rad;
  
  // amount of radians we have turned since we started at this location
  float _behaviorBodyFacingDone_rad;

  // the robot completes a turn in this direction, but in between angles, it moves back and forth and it moves the head
  // up and down
  EClockDirection _mainTurnDirection;
  
  // number of head moves left for step4
  uint8_t _s4HeadMovesLeft;
  
  // positions we have recently done
  std::list<Vec3f> _visitedLocations;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
