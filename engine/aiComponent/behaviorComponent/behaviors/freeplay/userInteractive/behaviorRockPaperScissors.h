/**
 * File: behaviorRockPaperScissors.h
 *
 * Author: Andrew Stein
 * Created: 2017-12-18
 *
 * Description: R&D Behavior to play rock, paper, scissors.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "anki/vision/basestation/image.h"

namespace Anki {
namespace Cozmo {

class BehaviorRockPaperScissors : public ICozmoBehavior
{
  friend class BehaviorContainer;
  explicit BehaviorRockPaperScissors(const Json::Value& config);

public:

  virtual ~BehaviorRockPaperScissors();

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override
  {
    return true;
  }

protected:

  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override { return false; }

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual BehaviorStatus UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event,  BehaviorExternalInterface& behaviorExternalInterface) override;  
  
  
private:
  
  // -------------
  // From Config:
  float    _waitAfterButton_sec = 1.f;
  float    _waitTimeBetweenTaps_sec = 0.5f;
  float    _minDetectionScore = 0.7f;
  uint32_t _displayHoldTime_ms = 1000;
  uint32_t _tapHeight_mm = 10;
  bool     _showDetectionString = false;
  uint8_t  _shootOnCount = 3;
  
  // -------------
  // State Machine
  enum State {
    WaitForButton,
    PlayCadence,
    WaitForResult,
    ShowResult,
    React
  };

  State _state = WaitForButton;

  // --------------------
  // Game Play Selections
  enum Selection {
    Unknown  = -1,
    Rock     = 0,
    Paper    = 1,
    Scissors = 2
  };
  
  Selection _robotSelection = Selection::Unknown;
  Selection _humanSelection = Selection::Unknown;
  
  std::map<Selection, Vision::ImageRGB> _displayImages;
  
  void DisplaySelection(BehaviorExternalInterface& behaviorExternalInterface, const std::string& overlayStr = "");
  
  void CopyToHelper(Vision::ImageRGB& toImg, const Selection selection, const s32 xOffset, const bool swapGB) const;
  
};

}
}


#endif /* __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorRockPaperScissors_H__ */
