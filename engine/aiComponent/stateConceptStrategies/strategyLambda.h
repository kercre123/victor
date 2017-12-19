/**
 * File: strategyLambda.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-30
 *
 * Description: A StateConceptStrategy that is implemented as a lmabda. This one cannot be created (solely)
 *              from config, it must be created in code. This can be useful to use the existing infrastructure
 *              and framework with a custom hard-coded strategy
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_StrategyLambda_H__
#define __Engine_AiComponent_StateConceptStrategies_StrategyLambda_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

namespace Anki {
namespace Cozmo {

class StrategyLambda : public IStateConceptStrategy
{
public:
  
  // NOTE: this strategy takes no config, because it can't be data defined
  StrategyLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc);

protected:

  bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  std::function<bool(BehaviorExternalInterface& bei)> _lambda;

};

}
}



#endif
