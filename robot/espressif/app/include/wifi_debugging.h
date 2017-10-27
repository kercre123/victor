#ifndef _WIFI_DEBUGGING_H_
#define _WIFI_DEBUGGING_H_
/** Definitions for wifi debugging system
 * @author Daniel Casner <daniel@anki.com>
 * Original concept Nathan Monson:
 * 1.On the Espressif, every 100ms, fill an array of uint16_t state[25] (50 bytes) with:
 *   Queue heads and tails for animation/CLAD/video/RT
 *   Count of packets sent/received, good drop count, idle loop count, error counts
 *   Stutter detect flag (animation buffer starved)
 *   Free RAM, current timestamp, time of last SPIFlash command, etc
 * 2. Every 5 states (500ms), send a UDP packet containing the last 10 state messages (500 bytes)
 *    Put a special header on it and use the RT port - don't use any part of RT (just in case the bug is in there)
 * 3. On the phone, bypass RT if you see the special UDP header, and queue up the last 100 state messages
 *    When the stutter bit goes from unset to set, dump out the last 5 seconds of states
 *    When you disconnect, dump out the last 5 seconds of states
 *
 * Actual implementation has drifted somewhat away from the above but the key concept is the same.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "c_types.h"

#define WIFI_DEBUG_STATES_PER_PAYLOAD 8
#define WIFI_DEBUG_STATES_MASK (WIFI_DEBUG_STATES_PER_PAYLOAD-1)
#define WIFI_DEBUG_PACKET_HEADER_PREFIX "COZ\x03WDBG"
#define WIFI_DEBUG_PACKET_HEADER_PREFIX_LENGTH 8

typedef struct {
  uint16_t head;
  uint16_t tail;
} QueueState;

/// Telemetry "state" sample. Treated on engine as a raw array of u16s
typedef struct {
  /** Increment when firmware things a dump should be triggered on the engine.
   * Must be first element as it's the only element the engine will look at.
   */
  uint16_t   badThingsCount;
  uint16_t   system_timestamp;
  uint16_t   freeMem;
  QueueState animationRx;
  uint16_t   rtTxQueued;
  uint16_t   rtTxPendingReliableBytes;
  uint16_t   rtTxNumPendingReliableMessages;
  uint16_t   rtTxPacketCount;
  uint16_t   rtTxMessageCount;
  int16_t    rtTimeSinceLastUAMST;
  int16_t    rtTimeSinceLastRx;
  uint16_t   videoQueued[2];
  uint16_t   i2spiDropCount;
  uint16_t   i2spiErrorCount;
  int16_t    i2spiIntegralDrift;
  QueueState i2spiRtipRx;
  QueueState i2spiRtipTx;
  uint16_t   backgroundLoopCount;
  uint16_t   clientDropCount;
  uint16_t   rtCanaries:3;
  uint16_t   sendHoldoff:1;
  uint16_t   animBufferStarved:1;
  uint16_t   animPlaying:1;
  uint16_t   clientConnected:1;
} WiFiDebugState;

#define WIFI_DEBUG_STATE_U16_ARR_SIZE (sizeof(WiFiDebugState)/sizeof(uint16_t))

typedef struct {
  char prefix[WIFI_DEBUG_PACKET_HEADER_PREFIX_LENGTH];
  WiFiDebugState payload[WIFI_DEBUG_STATES_PER_PAYLOAD];
} WiFiDebugPacket;

/// Forward declaration of wifi debug tick function
void wifi_debug_tick(void* arg);

/// Turn on wifi debugging, must not be called until after clientInit
void wifi_debug_enable(void);

#ifdef __cplusplus
} //Extern C
#endif

#endif
