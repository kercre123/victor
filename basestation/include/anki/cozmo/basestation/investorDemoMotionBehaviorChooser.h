/**
 * File: investorDemoBehaviorChooser.h
 *
 * Author: Brad Neuman
 * Created: 2015-11-25
 *
 * Description: The behavior chooser for the investor demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __COZMO_BASESTATION_INVESTORDEMOBEHAVIORCHOOSER_H__
#define __COZMO_BASESTATION_INVESTORDEMOBEHAVIORCHOOSER_H__

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

class InvestorDemoBehaviorChooser : public ReactionaryBehaviorChooser
{
public:
  InvestorDemoBehaviorChooser(Robot& robot, const Json::Value& config);

  virtual Result Update(double currentTime_sec) override;
  
  virtual const char* GetName() const override { return "InvestorDemo"; }

protected:
  void SetupBehaviors(Robot& robot, const Json::Value& config);
  
private:
  using super = ReactionaryBehaviorChooser;

};

}
}

#endif
