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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/pose.h"

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
struct PathMotionProfile;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreLookAroundInPlace
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorExploreLookAroundInPlace : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreLookAroundInPlace(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreLookAroundInPlace() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // todo: document. Is this behavior alway activatable, or we won't look around in an area we already know everything?
  virtual bool WantsToBeActivatedBehavior() const override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // a number which increases each time the state machine reaches the end. It is never reset for the behavior
  unsigned int GetNumIterationsCompleted() const { return _numIterationsCompleted; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = _configParams.behavior_CanCarryCube;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingPets, EVisionUpdateFrequency::Standard });
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set attributes from the given config
  void LoadConfig(const Json::Value& config);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual void OnBehaviorActivated() final override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // called by Init to enter the state machine after setup
  virtual void BeginStateMachine();
  
  void TransitionToS1_OppositeTurn();
  void TransitionToS2_Pause();
  void TransitionToS3_MainTurn();
  void TransitionToS4_HeadOnlyUp();
  void TransitionToS5_HeadOnlyDown();
  void TransitionToS6_MainTurnFinal();
  void TransitionToS7_IterationEnd();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State helpers accessible to derived classes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // request the proper action given the parameters so that the robot moves its head.
  // Animation team suggested to also do a small body move
  IAction* CreateHeadTurnAction(
    float bodyRelativeMin_deg, float bodyRelativeMax_deg,           // to provide mini-turns with head moves (animation team suggests)
    float bodyReference_deg,                                        // angle the relative ones use as referance (to turn absolute)
    float headAbsoluteMin_deg, float headAbsoluteMax_deg,           // head min/max range absolute
    float bodyTurnSpeed_degPerSec, float headTurnSpeed_degPerSec);  // turn speed

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Setters
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  void SetInitialBodyDirection(const Radians& rads) { _initialBodyDirection = rads; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // attributes specifically for configuration of every state
  struct Configuration
  {
    float   behavior_DistanceFromRecentLocationMin_mm;
    bool    behavior_CanCarryCube;
    uint8_t behavior_RecentLocationsMax;
    bool    behavior_ShouldResetTurnDirection; // if true, a new clock direction is picked every run, otherwise picked once and locked
    bool    behavior_ResetBodyFacingOnStart; // whether or not to reset the "cone" when the robot is put down
    bool    behavior_ShouldLowerLift;
    float   behavior_AngleOfFocus_deg;
    uint8_t behavior_NumberOfScansBeforeStop; // if 0 it will loop as long as it's not kicked out

    // turn speed (optional, specify this XOR the motion profile)
    float sx_BodyTurnSpeed_degPerSec = 0.0f;
    float sxt_HeadTurnSpeed_degPerSec = 0.0f; // for turn states
    float sxh_HeadTurnSpeed_degPerSec = 0.0f; // for head move states

    // instead of turn speeds, the user can optionally specify a motion profile (must specify this XOR turn
    // speeds above)
    std::unique_ptr<PathMotionProfile> customMotionProfile;
    
    // chance that the main turn will be counter clockwise (vs ccw)
    float s0_MainTurnCWChance;
    // [min,max] range for random turn angles for step 1
    float s1_BodyAngleRangeMin_deg;
    float s1_BodyAngleRangeMax_deg;
    float s1_HeadAngleRangeMin_deg;
    float s1_HeadAngleRangeMax_deg;
    // [min,max] range for pause for step 2
    float s2_WaitMin_sec;
    float s2_WaitMax_sec;
    std::string s2_WaitAnimTrigger; // name of the animation that will play instead of the wait
    // [min,max] range for random angle turns for step 3
    float s3_BodyAngleRangeMin_deg;
    float s3_BodyAngleRangeMax_deg;
    float s3_HeadAngleRangeMin_deg;
    float s3_HeadAngleRangeMax_deg;
    // [min,max] range for head move for step 4
    float s4_BodyAngleRelativeRangeMin_deg; // this is relative with respect to s4 start
    float s4_BodyAngleRelativeRangeMax_deg; // this is relative with respect to s4 start
    float s4_HeadAngleRangeMin_deg;
    float s4_HeadAngleRangeMax_deg;
    float s4_WaitBetweenChangesMin_sec;
    float s4_WaitBetweenChangesMax_sec;
    uint8_t s4_HeadAngleChangesMin;
    uint8_t s4_HeadAngleChangesMax;
    std::string s4_WaitAnimTrigger; // name of the animation that will play instead of the wait
    // [min,max] range for head move  for step 5
    float s5_BodyAngleRelativeRangeMin_deg;
    float s5_BodyAngleRelativeRangeMax_deg;
    float s5_HeadAngleRangeMin_deg;
    float s5_HeadAngleRangeMax_deg;
    // [min,max] range for random angle turns for step 6
    float s6_BodyAngleRangeMin_deg;
    float s6_BodyAngleRangeMax_deg;
    float s6_HeadAngleRangeMin_deg;
    float s6_HeadAngleRangeMax_deg;
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // parsed configurations params from json
  Configuration _configParams;

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

  // decide which direction to turn
  void DecideTurnDirection();
  
  // request the proper action given the parameters so that the robot turns and moves head
  IAction* CreateBodyAndHeadTurnAction(
            EClockDirection clockDirection,                                 // body turn clock direction
            float bodyStartRelativeMin_deg, float bodyStartRelativeMax_deg, // body min/max range relative to starting angle (from original state)
            float headAbsoluteMin_deg, float headAbsoluteMax_deg,           // head min/max range absolute
            float bodyTurnSpeed_degPerSec, float headTurnSpeed_degPerSec);  // turn speeds
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  // facing direction when we start an iteration
  Radians _iterationStartingBodyFacing_rad;
  
  // amount of radians we have turned since we started at this location
  float _behaviorBodyFacingDone_rad;
  
  // number of times we have bounced off a cone side (requires cone of focus). 2 means an entire cone was done, 4 twice, ...
  uint8_t _coneSidesReached;

  // count the total number of iterations through the state machine. This number will only increase (it never gets reset)
  unsigned int _numIterationsCompleted = 0;

  // the robot completes a turn in this direction, but in between angles, it moves back and forth and it moves the head
  // up and down
  EClockDirection _mainTurnDirection;
  
  // s4 specific vars
  Radians _s4_s5StartingBodyFacing_rad; // starting facing for s4 (because angle changes are relative to this angle)
  uint8_t _s4HeadMovesRolled;           // number of head moves rolled for step4
  uint8_t _s4HeadMovesLeft;             // number of head moves left for step4
  
  // positions we have recently done
  std::list<Pose3d> _visitedLocations;
  
  // Initial direction on behavior init
  Radians _initialBodyDirection;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
