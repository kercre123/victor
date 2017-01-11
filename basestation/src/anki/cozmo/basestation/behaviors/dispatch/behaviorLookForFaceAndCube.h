/**
 * File: behaviorLookForFaceAndCube
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Look for faces and cubes from the current position.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorLookForFaceAndCube_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLookForFaceAndCube_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace ExternalInterface {
struct RobotObservedObject;
}
class IAction;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorLookForFaceAndCube
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorLookForFaceAndCube : public IBehavior
{
private:
  
  using BaseClass = IBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorLookForFaceAndCube(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorLookForFaceAndCube() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This behavior is runnable if when we check the memory map around the current robot position, there are still
  // undiscovered areas
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set attributes from the given config
  void LoadConfig(const Json::Value& config);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // store state to resume
  enum class State {
    S0FaceOnCenter,
    S1FaceOnLeft,
    S2FaceOnRight,
    S3CubeOnRight, // because we ended right for face, start on right for cube
    S4CubeOnLeft,
    S5Center,
    Done
  };
  
  // S0: look for face center
  void TransitionToS1_FaceOnLeft(Robot& robot);
  void TransitionToS2_FaceOnRight(Robot& robot);
  void TransitionToS3_CubeOnRight(Robot& robot);
  void TransitionToS4_CubeOnLeft(Robot& robot);
  void TransitionToS5_Center(Robot& robot);
  void TransitionToS6_Done(Robot& robot);
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // attributes specifically for configuration of every state
  struct Configuration
  {
    // turn speeds
    Radians bodyTurnSpeed_radPerSec;
    Radians headTurnSpeed_radPerSec;
    // faces
    Radians face_headAngleAbsRangeMin_rad;  // min head angle when looking for faces
    Radians face_headAngleAbsRangeMax_rad;  // max head angle when looking for faces
    Radians face_bodyAngleRelRangeMin_rad;  // min body angle to turn a little when looking for faces
    Radians face_bodyAngleRelRangeMax_rad;  // max body angle to turn a little when looking for faces
    uint8_t face_sidePicks;               // in addition to center, how many angle picks we do per side - face (x per left, x per right)
    // cubes
    Radians cube_headAngleAbsRangeMin_rad;  // min head angle when looking for cubes
    Radians cube_headAngleAbsRangeMax_rad;  // max head angle when looking for cubes
    Radians cube_bodyAngleRelRangeMin_rad;  // min body angle to turn a little when looking for cubes
    Radians cube_bodyAngleRelRangeMax_rad;  // max body angle to turn a little when looking for cubes
    uint8_t cube_sidePicks;               // in addition to center, how many angle picks we do per side - cube (x per left, x per right)
    // anims
    AnimationTrigger lookInPlaceAnimTrigger;
    // goals to be dispatched
    
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  IAction* CreateBodyAndHeadTurnAction(Robot& robot,
            const Radians& bodyRelativeMin_rad, const Radians& bodyRelativeMax_rad, // body min/max range added relative to target
            const Radians& bodyAbsoluteTargetAngle_rad,                             // center of the body rotation range
            const Radians& headAbsoluteMin_rad, const Radians& headAbsoluteMax_rad, // head min/max range absolute
            const Radians& bodyTurnSpeed_radPerSec,                                 // body turn speed
            const Radians& headTurnSpeed_radPerSec);                                // head turn speed

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // parsed configurations params from json
  Configuration _configParams;

  // facing direction when we start the behavior
  Radians _startingBodyFacing_rad;
  
  // number of angle picks we have done for the current state
  uint8_t _currentSidePicksDone;
  
  // current state so that we resume at the proper stage (react to cube interrupts behavior for example)
  State _currentState;
};

} // namespace Cozmo
} // namespace Anki

#endif //
