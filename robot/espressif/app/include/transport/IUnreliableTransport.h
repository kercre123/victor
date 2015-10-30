#ifndef __I_UNRELIABLE_TRANSPORT_H
#define __I_UNRELIABLE_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/// Unreliable transport backhaul MTU
#define UnreliableTransport_MAX_BYTES_PER_PACKET 1420 ///< Apparently the largest MTU which is reliable on the espressif

/// To transmit a buffer over unreliable backhaul
bool UnreliableTransport_SendPacket(uint8_t* buffer, uint16_t bufferSize);

// end extern "C"
#ifdef __cplusplus
}
#endif

#endif
