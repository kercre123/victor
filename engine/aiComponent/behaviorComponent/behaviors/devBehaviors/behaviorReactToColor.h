/**
 * File: BehaviorReactToColor.h
 *
 * Author: Patrick Doran
 * Created: 2018/11/19
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToColor__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToColor__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "clad/types/salientPointTypes.h"

#include <list>

namespace Anki {
namespace Vector {

class BehaviorReactToColor : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToColor();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToColor(const Json::Value& config);  

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

    //! Number of Hues. This is used to round SalientPoint Colors to he nearest hue
    u32 numHues;

    //! Target distance to the object to move to in millimeters
    u32 targetDistance_mm;

    //! Number of times to spin when he gets to the bright color
    u32 celebrationSpins;

    //! The age of salient points to consider in ms
    u32 salientPointAge_ms;

    //! Animation to play when a bright color is first detected
    AnimationTrigger animDetect;

    //! Animation to play when a bright color A is reached
    AnimationTrigger animReactColorA;

    //! Animation to play when a bright color B is reached
    AnimationTrigger animReactColorB;

    //! Animation to play when giving up for any reason
    AnimationTrigger animGiveUp;

    //! Amount of error for close enough to matching a hue. This is computed from numHues, not set from config.
    f32 closeHueEpsilon;
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
   * @brief Round the input SalientPoints that are Colors to the nearest hue. Modifies elements of list.
   * @see InstanceConfig::numHueSectors
   */
  void RoundToNearestHue(std::list<Vision::SalientPoint>& points);

  /**
   * @brief Find a salient point for the current color the behavior is interested
   * @details Finds a salient point within an epsilon from the current color and treats that as the same SalientPoint.
   * @return true if a similar color point is found, and sets point to the found value, returns false otherwise.
   */
  bool FindCurrentColor(Vision::SalientPoint& point);

  /**
   * @brief Find a salient point for a new color
   * @details Find a salient point for a new color that the behavior has not interacted with in a while.
   * @return true if point has been set to a new Color SalientPoint, false otherwise
   * @todo Replace this structure with returning a std::optional<Vision::SalientPoint>
   */
  bool FindNewColor(Vision::SalientPoint& point);

  void TurnOnLights(const Vision::SalientPoint& point);
  void TurnOffLights();

  bool CheckIfShouldStop();

  void TransitionToStart();
  void TransitionToNoticedColor();
  void TransitionToTurnTowardsPoint();
  void TransitionToLookForCurrentColor();
  void TransitionToDriveTowardsPoint();
  void TransitionToCheckGotToObject();
  void TransitionToCelebrate();
  void TransitionToGiveUpLostColor();
  void TransitionToGiveUpTooFarAway();
  void TransitionToGiveUpLostObject();
  void TransitionToCompleted();

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToColor__
