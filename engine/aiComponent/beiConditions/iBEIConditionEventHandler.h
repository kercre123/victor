/**
 * File: iStateConceptStrategyEventHandler.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-30
 *
 * Description: Interface for state concept strategies which need to handle events
 * // TODO:(bn) this could be combined with behaviors in a single interface
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_IBEIConditionEventHandler_H__
#define __Engine_AiComponent_StateConceptStrategies_IBEIConditionEventHandler_H__

#include "engine/events/ankiEvent.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
class MessageGameToEngine;
class MessageEngineToGame;
enum class MessageEngineToGameTag : uint8_t;
enum class MessageGameToEngineTag : uint8_t;
}

class BehaviorExternalInterface;
class IExternalInterface;

class IBEIConditionEventHandler
{
public:
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;

  virtual void HandleEvent(const GameToEngineEvent& event, BehaviorExternalInterface& bei) {}
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& bei) {}
};

}
}

#endif
