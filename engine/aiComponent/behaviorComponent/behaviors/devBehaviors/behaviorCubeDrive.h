/**
 * File: BehaviorCubeDrive.h
 *
 * Author: Ron Barry
 * Created: 2019-01-07
 *
 * Description: A three-dimensional steering wheel - with 8 corners.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
#pragma once

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoObservableObject.h"
#include "coretech/vision/engine/image_impl.h"

#include <chrono>
#include <memory>

namespace Anki {
namespace Vector {

namespace CubeAccelListeners {
class LowPassFilterListener;
} // namespace CubeAccelListeners

struct ActiveAccel;

struct PanelCell {
  std::string Text;
  int         Action;
};

struct Panel {
  int NumRows;
  int NumCols;
  PanelCell* Cells;
};

struct PanelSet {
  int    NumPanels;
  Panel* Panels;
};

class BehaviorCubeDrive : public ICozmoBehavior
{
public: 
  virtual ~BehaviorCubeDrive();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCubeDrive(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  void SetLiftState(bool up);
  void RestartAnimation();

private:

  enum {
        DIR_U   = 0, // up
        DIR_D   = 1, // down
        DIR_L   = 2, // left
        DIR_R   = 3, // right
        NUM_DIR = 4,
        MAX_DIR_COUNT = 100
  };

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID                                                    object_id;
    std::shared_ptr<ActiveAccel>                                filtered_cube_accel;
    std::shared_ptr<CubeAccelListeners::LowPassFilterListener>  low_pass_filter_listener;
    Vision::Image                                               image;
    double                                                      last_lift_action_time;
    bool                                                        lift_up;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  bool _liftIsUp;

  std::string _promptText;
  std::string _userText;
  int _col, _row;
  int _deadZoneTicksLeft;
  int _dirHoldCount[NUM_DIR];
  bool _dirCountEventMap[MAX_DIR_COUNT];
  
  void ClearHoldCounts();
  void NewDirEvent(int dir, int maxRow, int maxCol);

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
