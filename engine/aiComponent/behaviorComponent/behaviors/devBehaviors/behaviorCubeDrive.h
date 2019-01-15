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

struct ActiveAccel;

struct PanelCell {
  std::string Text;
  int         Action;
};

struct Panel {
  int        NumRows; 
  int        NumCols; 
  PanelCell* Cells;   
  bool       IsSelectMenu;
};

struct PanelSet {
  int     NumPanels;
  Panel** Panels;
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
    Vision::Image image;                 
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  bool _liftIsUp;
  bool _buttonPressed;

  int  _controlScheme;
  PanelSet*   _panelSet;
  std::string _promptText;
  std::string _userText;
  int  _currPanel, _row, _col, _firstScreenRow;
  int  _deadZoneTicksLeft;
  int  _dirHoldCount[NUM_DIR];
  bool _dirCountEventMap[MAX_DIR_COUNT];
  
  void ClearHoldCounts();
  void NewDirEvent(int dir, int maxRow, int maxCol);
  void DisplayHeaderText(std::string text, ColorRGBA color, float maxWidth);

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
