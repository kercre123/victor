/**
 * File: iNetTransportDataReceiver
 *
 * Author: baustin
 * Created: 1/28/15
 *
 * Description: Interface for callbacks that will be invoked when network messages are received
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/transport/iNetTransportDataReceiver.h"

namespace Anki {
namespace Util {

static const uint8_t DataReceiverOnConnected{};
static const uint8_t DataReceiverOnConnectRequest{};
static const uint8_t DataReceiverOnDisconnected{};

const uint8_t* const INetTransportDataReceiver::OnConnected = &DataReceiverOnConnected;
const uint8_t* const INetTransportDataReceiver::OnConnectRequest = &DataReceiverOnConnectRequest;
const uint8_t* const INetTransportDataReceiver::OnDisconnected = &DataReceiverOnDisconnected;

} // end namespace Util
} // end namespace Anki
