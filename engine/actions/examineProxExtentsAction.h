/**
 * File: examineProxExtentsAction.h
 *
 * Author: ross
 * Date:   May 2 2018
 *
 * Description: Action that rotates the robot when near an obstacle to help sense the object's size with the prox sensor.
 *              If the action is started and there is no obstacle in front, it will end (successfully).
 *              The action then rotates in some config-driven combination of left/right/center, stopping
 *              rotation and moving to the next stage whenever the prox reading is large enough (config)
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/actions/actionInterface.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Cozmo {
  
class ICompoundAction;

class ExamineProxExtentsAction : public IAction
{
public:
  
  enum class Ordering : uint8_t
  {
    RandomRandomCenter, // randomly chooses one of the next two
    LeftRightCenter,
    RightLeftCenter
    // options without center might be useful, but are omitted until theyre needed
  };
  
  // in the given order, turn by at least minAngle and no more than maxAngle, and stopping
  // when no obstacle is detected with the prox sensor within freeDistance_mm
  ExamineProxExtentsAction( Ordering order, const Radians& minAngle, const Radians& maxAngle, u16 freeDistance_mm );
  
  virtual ~ExamineProxExtentsAction();
  
protected:
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override {}
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  
private:
  Ordering _order;
  Radians _minAngle;
  Radians _maxAngle;
  u16 _distance_mm;
  Radians _initialAngle;
  
  std::unique_ptr<ICompoundAction> _subAction;
};

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
class TurnUntilFreeAction : public IAction
{
public:
  
  // Turns in place until the prox sensor doesn't detect any obstacle within freeDistance_mm,
  // or until angle is reached (which can be an absolute or relative angle).
  // Even if the sensor is initially clear, the robot will turn by at least minAngle (in absolute value).
  // If !isAbsoluteAngle, minAngle is a relative angle in the direction of turning. If isAbsoluteAngle
  // and minAngle is non-zero, the robot's rotation must cross the absolute angle minAngle. If
  // minAngle is 0, the action will stop as soon as the prox is free.
  // If the lift is in the way, this action will attempt to move it and abort if unsuccessful
  TurnUntilFreeAction( const Radians& angle, bool isAbsoluteAngle, u16 freeDistance_mm, const Radians& minAngle={} );
  
  virtual ~TurnUntilFreeAction();

protected:
  
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override {}
  
  virtual ActionResult Init() override;
  
  virtual ActionResult CheckIfDone() override;

private:
  
  bool IsLiftInFOV() const;
  
  bool IsFree() const;
  
  std::unique_ptr<ICompoundAction> _subAction;

  Radians _angle;
  bool _isAbsoluteAngle;
  u16 _distance_mm;
  
  float _minAngleForExit; // relative to initial angle in absolute value if !_isAbsoluteAngle, or an abs angle if _isAbsoluteAngle
  
  Radians _initialAngle;
};

}
}

