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

#ifndef __Engine_AiComponent_StateConceptStrategies_ConditionLambda_H__
#define __Engine_AiComponent_StateConceptStrategies_ConditionLambda_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Cozmo {

class ConditionLambda : public IBEICondition
{
public:
  
  // NOTE: this strategy takes no config, because it can't be data defined
  ConditionLambda(std::function<bool(BehaviorExternalInterface& bei)> areConditionsMetFunc);

protected:

  bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  std::function<bool(BehaviorExternalInterface& bei)> _lambda;

};

}
}



#endif
