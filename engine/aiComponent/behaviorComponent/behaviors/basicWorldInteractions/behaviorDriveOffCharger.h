/**
 * File: behaviorDriveOffCharger.h
 *
 * Author: Molly Jameson (or talk to ross)
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to off a charger and deal with being on/off treads while on the charger.
 *              Supply a prioritized list of options to choose the direction based on the most recent face,
 *              cube, or mic direction, or simply drive randomly or straight. If a supplied direction
 *              is not applicable (e.g., no faces) or is unsuccessful, the next one is attempted.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/micDirectionTypes.h"

namespace Anki {
namespace Vector {

class BehaviorDriveOffCharger : public ICozmoBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDriveOffCharger(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

private:
  
  enum class DriveDirection : uint8_t {
    Straight,           // an animation action
    Left,               // an animation action
    Right,              // an animation action
    ProceduralStraight, // a drive action
    Face,               // an animation action in the direction of the most recent face
    Cube,               // an animation action in the direction of the most recent cube
    MicDirection,       // an animation action in the mic direction
    Randomly,           // delegates to a random dispatcher with animations
  };
  
  enum class State {
    NotStarted=0,
    WaitForOnTreads,
    Driving,
  };
  
  struct InstanceConfig {
    InstanceConfig();
    std::vector<DriveDirection> driveDirections;
    float proceduralDistToDrive_mm;
    ICozmoBehaviorPtr driveRandomlyBehavior;
    int maxFaceAge_s;
    int maxCubeAge_s;
  };

  struct DynamicVariables {
    DynamicVariables();
    State state;
    unsigned int directionIndex; // gets incremented either if a direction is unknown or the drive attempt fails
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  
  DriveDirection ParseDriveDirection( const std::string& driveDirectionStr ) const;
  
  void DriveToPose( const Pose3d& pose );
  
  MicDirectionIndex GetDirectionFromMicHistory() const;
  
  void SelectAndDrive();
  
  // Each of these is tried in the order supplied to _dVars.driveDirections. If one does not delegate,
  // or if it delegates but afterwards the robot is still on the charger, the next one is tried.
  // After all entries in _dVars.driveDirections are attempted, we attempt TransitionToDrivingStraightProcedural
  // until it's off the charger
  void TransitionToDrivingAnim( const AnimationTrigger& animTrigger );
  void TransitionToDrivingStraightProcedural();
  void TransitionToDrivingRandomly();
  void TransitionToDrivingFace();
  void TransitionToDrivingCube();
  void TransitionToDrivingMicDirection();

};

} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
