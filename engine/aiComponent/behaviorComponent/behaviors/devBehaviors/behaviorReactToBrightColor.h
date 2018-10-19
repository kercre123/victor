/**
 * File: BehaviorReactToBrightColor.h
 *
 * Author: Patrick Doran
 * Created: 2018-10-16
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "clad/types/salientPointTypes.h"

#include <list>

namespace Anki {
namespace Vector {

class BehaviorReactToBrightColor : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToBrightColor();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToBrightColor(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);

    //! Number of Hues. This is used to round SalientPoint BrightColors to he nearest hue
    u32 numHues;

    //! Target distance to the object to move to in millimeters
    u32 targetDistance_mm;

    //! Number of times to spin when he gets to the bright color
    u32 celebrationSpins;
  };

  struct DynamicVariables {
    struct Persistent {
      RobotTimeStamp_t lastSeenTimeStamp;
    } persistent;

    DynamicVariables();
    void Reset(bool keepPersistent);

    //! The point that started this initial reaction (TODO: replace with optional<Vision::SalientPoint>)
    std::shared_ptr<Vision::SalientPoint> point;
  };

  /**
   * @brief Round the input SalientPoints that are BrightColors to the nearest hue
   * @see InstanceConfig::numHueSectors
   */
  void RoundToNearestHue(std::list<Vision::SalientPoint>& points);

  /**
   * @brief Find a salient point for the current color the behavior is interested
   * @return
   */
  void FindCurrentColor();

  /**
   * @brief Find a salient point for a new color
   * @details Find a salient point for a new color that the behavior has not interacted with in a while.
   * @return true if point has been set to a new BrightColor SalientPoint, false otherwise
   * @todo Replace this structure with returning a std::optional<Vision::SalientPoint>
   */
  bool FindNewColor(Vision::SalientPoint& point);

  void TurnOnLights(const Vision::SalientPoint& point);
  void TurnOffLights();

  bool CheckIfShouldStop();

  void TransitionToStart();
  void TransitionToTurnTowardsPoint();
  void TransitionToDriveTowardsPoint();
  void TransitionToCelebrate();
  void TransitionToCompleted();

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
