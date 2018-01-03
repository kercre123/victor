/**
 * File: stateConceptStrategyMessageHelper.h
 *
 * Author: Brad Neuman
 * Created: 2017-12-07
 *
 * Description: Object to handle subscribing to messages and calling callbacks properly
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_BEIConditionMessageHelper_H__
#define __Engine_AiComponent_StateConceptStrategies_BEIConditionMessageHelper_H__

#include "util/signals/simpleSignal_fwd.h"

#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
enum class MessageEngineToGameTag : uint8_t;
enum class MessageGameToEngineTag : uint8_t;
}

class BehaviorExternalInterface;
class IBEIConditionEventHandler;

class BEIConditionMessageHelper
{
public:

  // create this component which will call the callbacks on the given handler. In most cases, users will pass
  // in `this` as the handler
  BEIConditionMessageHelper(IBEIConditionEventHandler* handler, BehaviorExternalInterface& bei);

  ~BEIConditionMessageHelper();

  void SubscribeToTags(std::set<ExternalInterface::MessageEngineToGameTag>&& tags);
  void SubscribeToTags(std::set<ExternalInterface::MessageGameToEngineTag>&& tags);

private:

  BehaviorExternalInterface& _bei;
  IBEIConditionEventHandler* _handler = nullptr;
  
  std::vector<::Signal::SmartHandle> _eventHandles;
};

}
}

#endif
