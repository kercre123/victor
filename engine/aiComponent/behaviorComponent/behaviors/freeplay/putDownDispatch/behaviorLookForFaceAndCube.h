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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include <set>

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
class BehaviorLookForFaceAndCube : public ICozmoBehavior
{
private:
  
  using BaseClass = ICozmoBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorLookForFaceAndCube(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorLookForFaceAndCube() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This behavior is activatable if when we check the memory map around the current robot position, there are still
  // undiscovered areas
  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Standard});
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:
  using FaceID_t = Vision::FaceID_t;

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

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    // turn speeds
    Radians bodyTurnSpeed_radPerSec;
    Radians headTurnSpeed_radPerSec;
    // faces
    Radians face_headAngleAbsRangeMin_rad;  // min head angle when looking for faces
    Radians face_headAngleAbsRangeMax_rad;  // max head angle when looking for faces
    Radians face_bodyAngleRelRangeMin_rad;  // min body angle to turn a little when looking for faces
    Radians face_bodyAngleRelRangeMax_rad;  // max body angle to turn a little when looking for faces
    uint8_t face_sidePicks;                 // in addition to center, how many angle picks we do per side - face (x per left, x per right)
    bool verifySeenFaces;                   // if true, turn towards and verify any faces we see during this behavior
    // cubes
    Radians cube_headAngleAbsRangeMin_rad;  // min head angle when looking for cubes
    Radians cube_headAngleAbsRangeMax_rad;  // max head angle when looking for cubes
    Radians cube_bodyAngleRelRangeMin_rad;  // min body angle to turn a little when looking for cubes
    Radians cube_bodyAngleRelRangeMax_rad;  // max body angle to turn a little when looking for cubes
    uint8_t cube_sidePicks;                 // in addition to center, how many angle picks we do per side - cube (x per left, x per right)
    // anims
    AnimationTrigger lookInPlaceAnimTrigger;
    // early stopping
    bool stopBehaviorOnAnyFace;   // leave the behavior as soon as any face is seen
    bool stopBehaviorOnNamedFace; // leave the behavior as soon as a named face is seen
    bool stopBehaviorOnCube;      // leave the behavior as soon as a cube is seen
  };

  struct DynamicVariables {
    DynamicVariables(Radians&& startingRads);

    // facing direction when we start the behavior
    Radians startingBodyFacing_rad;
    
    // number of angle picks we have done for the current state
    uint8_t currentSidePicksDone;
    
    // current state so that we resume at the proper stage (react to cube interrupts behavior for example)
    State currentState;

    // set of face ID's that we have "verified" with a turn to action (if desired)
    std::set<FaceID_t> verifiedFaces;
    bool isVerifyingFace;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State transitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  
  // S0: look for face center
  void TransitionToS1_FaceOnLeft();
  void TransitionToS2_FaceOnRight();
  void TransitionToS3_CubeOnRight();
  void TransitionToS4_CubeOnLeft();
  void TransitionToS5_Center();
  void TransitionToS6_Done();
 
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  IAction* CreateBodyAndHeadTurnAction(
            const Radians& bodyRelativeMin_rad, const Radians& bodyRelativeMax_rad, // body min/max range added relative to target
            const Radians& bodyAbsoluteTargetAngle_rad,                             // center of the body rotation range
            const Radians& headAbsoluteMin_rad, const Radians& headAbsoluteMax_rad, // head min/max range absolute
            const Radians& bodyTurnSpeed_radPerSec,                                 // body turn speed
            const Radians& headTurnSpeed_radPerSec);                                // head turn speed

  void ResumeCurrentState();

  // stop the behavior if desired based on observing the given face
  void StopBehaviorOnFaceIfNeeded(FaceID_t observedFace);
  
  // true if a cube is in blockworld
  bool DoesCubeExist() const;

  // cancel the current action and do a verify face action instead
  void CancelActionAndVerifyFace(FaceID_t observedFace);

};

} // namespace Cozmo
} // namespace Anki

#endif //
