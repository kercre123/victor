/**
 * File: investorDemoMotionBehaviorChooser.h
 *
 * Author: Brad Neuman
 * Created: 2015-11-25
 *
 * Description: The behavior chooser for the investor demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __COZMO_BASESTATION_INVESTORDEMOMOTIONBEHAVIORCHOOSER_H__
#define __COZMO_BASESTATION_INVESTORDEMOMOTIONBEHAVIORCHOOSER_H__

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

class InvestorDemoMotionBehaviorChooser : public ReactionaryBehaviorChooser
{
public:
  InvestorDemoMotionBehaviorChooser(Robot& robot, const Json::Value& config);

  virtual Result Update(double currentTime_sec) override;
  
  virtual const char* GetName() const override { return "IDMotion"; }

protected:
  void SetupBehaviors(Robot& robot, const Json::Value& config);
  
private:
  using super = ReactionaryBehaviorChooser;

};

}
}

#endif
