/**
* File: gatewayInterface
*
* Author: Shawn Blakesley
* Created: 6/13/18
*
* Description: The interface used for protoMessageHandler
*
* Copyright: Anki, inc. 2018
*/

#ifndef __Anki_Cozmo_ExternalInterface_GatewayInterface_H__
#define __Anki_Cozmo_ExternalInterface_GatewayInterface_H__

#include "engine/events/ankiEvent.h"
#include "util/signals/simpleSignal.hpp"
#include "proto/external_interface/messages.pb.h"
#include <vector>
#include <utility>

namespace Anki {
namespace Cozmo {

class IGatewayInterface {
public:
  virtual ~IGatewayInterface() {};
  
  virtual void Broadcast(const external_interface::GatewayWrapper& message) = 0;
  virtual void Broadcast(external_interface::GatewayWrapper&& message) = 0;
  
  virtual Signal::SmartHandle Subscribe(const external_interface::GatewayWrapper::OneofMessageTypeCase& tagType, std::function<void(const AnkiEvent<external_interface::GatewayWrapper>&)> messageHandler) = 0;

  virtual uint32_t GetMessageCountOutgoing() const = 0;
  virtual uint32_t GetMessageCountIncoming() const = 0;
  virtual void     ResetMessageCounts() = 0;

protected:
  virtual void DeliverToExternal(const external_interface::GatewayWrapper& message) = 0;
};

} // end namespace Cozmo
} // end namespace Anki

#endif //__Anki_Cozmo_ExternalInterface_GatewayInterface_H__
