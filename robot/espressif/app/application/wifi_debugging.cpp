extern "C" {
  #include "wifi_debugging.h"
  #include "osapi.h"
  #include "user_interface.h"
  #include "mem.h"
  #include "driver/i2spi.h"
  bool clientConnected(void);
  bool clientSendHoldoff(void);
  uint32_t clientDropCount(void);
  void Client_raw_send(uint8_t* data, uint16_t len);
}

#include "animationController.h"
#include "anki/cozmo/transport/reliableTransport.h"
#include "anki/cozmo/robot/ctassert.h"
#include "anki/cozmo/robot/logging.h"

extern "C" {
  extern ReliableConnection g_conn;
}

extern uint16_t cozmoBackgroundTaskLoopCount;
extern bool sendHoldoff;


#define wdbs wifiDebugPacket->payload[wd_phase]
#define prev_wdbs wifiDebugPacket->payload[(wd_phase - 1) & WIFI_DEBUG_STATES_MASK] 

ct_assert(sizeof(WiFiDebugPacket) < 500);

static os_timer_t* debug_timer = NULL;
static WiFiDebugPacket* wifiDebugPacket = NULL;

extern "C" void wifi_debug_enable(void)
{
  if (wifiDebugPacket == NULL)
  {
    debug_timer = (os_timer_t*)os_zalloc(sizeof(os_timer_t));
    AnkiConditionalErrorAndReturn(debug_timer != NULL, 1237, "wifi_telemetry.alloc", 651, "Couldn't allocate memory for time", 0);
    wifiDebugPacket = (WiFiDebugPacket*)os_zalloc(sizeof(WiFiDebugPacket));
    if (wifiDebugPacket == NULL)
    {
      AnkiError(1237, "wifi_telemetry.alloc", 652, "Couldn't allocate memory for debug packet", 0);
      os_free(debug_timer);
      debug_timer = NULL;
      return;
    }
    os_memset(wifiDebugPacket, 0, sizeof(WiFiDebugPacket));
    os_memcpy(wifiDebugPacket->prefix, WIFI_DEBUG_PACKET_HEADER_PREFIX, WIFI_DEBUG_PACKET_HEADER_PREFIX_LENGTH);
    os_timer_disarm(debug_timer);
    os_timer_setfn(debug_timer, wifi_debug_tick, NULL);
    os_timer_arm(debug_timer, 100, 1);
  }
}

extern "C" void wifi_debug_tick(void* arg)
{
  static uint16_t badThingsAccumulator = 0;
  static int8_t wd_phase = 0;
  const uint32_t now = system_get_time();
  bool starved = false;
  bool playing = false;
  
  wdbs.system_timestamp = now / 10000; // 10ms ticks
  wdbs.freeMem = xPortGetFreeHeapSize() & 0xffff;
  Anki::Cozmo::AnimationController::GetDebugInfo(&wdbs.animationRx.head, &wdbs.animationRx.tail, &starved, &playing);
  if (starved)
  {
    wdbs.animBufferStarved = 1;
    if (prev_wdbs.animBufferStarved == 0) badThingsAccumulator++;
  }
  else wdbs.animBufferStarved = 0;
  wdbs.animPlaying = playing ? 1 : 0;
  wdbs.rtTxQueued = g_conn.txQueued;
  wdbs.rtTxPendingReliableBytes = g_conn.pendingReliableBytes;
  wdbs.rtTxNumPendingReliableMessages = g_conn.numPendingReliableMessages;
  wdbs.rtTxPacketCount = g_conn.txPacketCount & 0xFFFF;
  wdbs.rtTxMessageCount = g_conn.txMessageCount & 0xFFFF;
  wdbs.rtTimeSinceLastUAMST = (now - g_conn.latestUnackedMessageSentTime) / 1000; // Convert to ms
  wdbs.rtTimeSinceLastRx = (now - g_conn.latestRecvTime) / 1000; // Convert to ms
  wdbs.rtCanaries = (g_conn.canary1 == 0 ? 0 : 1) | (g_conn.canary2 == 0 ? 0 : 2) | (g_conn.canary3 == 0 ? 0 : 4);
  if (wdbs.rtCanaries != 0) badThingsAccumulator++;
  if (wdbs.rtTimeSinceLastRx > 1000) badThingsAccumulator++;
  if (wdbs.rtTimeSinceLastUAMST > 1000) badThingsAccumulator++;
  i2spiGetDebugTelemetry(wdbs.videoQueued, &wdbs.i2spiDropCount,
                         &wdbs.i2spiErrorCount, &wdbs.i2spiIntegralDrift,
                         &wdbs.i2spiRtipRx.head, &wdbs.i2spiRtipRx.tail,
                         &wdbs.i2spiRtipTx.head, &wdbs.i2spiRtipTx.tail);
  if (wdbs.i2spiErrorCount > prev_wdbs.i2spiErrorCount) badThingsAccumulator++;
  wdbs.backgroundLoopCount = cozmoBackgroundTaskLoopCount;
  wdbs.clientDropCount = clientDropCount() & 0xFFFF;
  if (wdbs.clientDropCount > prev_wdbs.clientDropCount) badThingsAccumulator++;
  wdbs.sendHoldoff = clientSendHoldoff() ? 1 : 0;
  wdbs.clientConnected = clientConnected() ? 1 : 0;
  if (wdbs.clientConnected == 0 and prev_wdbs.clientConnected == 1) badThingsAccumulator++;
  
  wdbs.badThingsCount = badThingsAccumulator;
  
  wd_phase++;
  if (wd_phase >= WIFI_DEBUG_STATES_PER_PAYLOAD)
  {
    // Make sure we have enough heap available to send since we know there are malloc bugs in Espressif
    if (xPortGetFreeHeapSize() > (int)(sizeof(WiFiDebugPacket) + 200))
    {
      Client_raw_send((uint8_t*)wifiDebugPacket, sizeof(WiFiDebugPacket));
    }
    wd_phase = 0;
  }
}
