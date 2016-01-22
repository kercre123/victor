/** @file Implementation of RTIP interface to WiFi processor
 * @author Daniel Casner <daniel@anki.com>
 */

#include "wifi.h"
#include "messages.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "anki/cozmo/robot/logging.h"
#include "clad/robotInterface/messageEngineToRobot.h"

/// Code below assumes buffer elements = uint8_t also assumes power of two size
#define TX_BUF_SIZE (256)
#define TX_BUF_SIZE_MASK (TX_BUF_SIZE-1)
#define RX_BUF_SIZE (256)
#define RX_BUF_SIZE_MASK (RX_BUF_SIZE-1)

static uint8_t txBuf[TX_BUF_SIZE];
static uint8_t rxBuf[RX_BUF_SIZE];

/// Write index for tx buffer, only altered by clientSendMessage
static volatile uint8_t txWind;
/// Read index for the tx buffer, only altered by GetTxData
static volatile uint8_t txRind;
/// Write index for the rx buffer, only altered by ReceiveMessage
static volatile uint8_t rxWind;
/// Read index for the rx buffer, only altered by Update
static volatile uint8_t rxRind;

static u8 wifiState = 0;
static u8 blueState = 0;

namespace Anki {
namespace Cozmo {
namespace HAL {
  bool RadioSendMessage(const void* buffer, const u16 size, const u8 msgID, const bool reliable, const bool hot)
  {
    if (!RadioIsConnected())
    {
      return true; // Not connected, message was sent to everyone listening
    }
    else
    {
      AnkiConditionalErrorAndReturnValue(size <= RTIP_MAX_CLAD_MSG_SIZE, false, 41, "WiFi", 260, "Can't send message %x[%d] to WiFi, max size %d\r\n", 3, msgID, size, RTIP_MAX_CLAD_MSG_SIZE);
      const uint8_t rind = txRind;
      uint8_t wind = txWind;
      const int available = TX_BUF_SIZE - ((wind - rind) & TX_BUF_SIZE_MASK);
      while (available < (size + 4))
			{ // Wait for room for message plus tag plus header plus one more so we can tell empty from full
				if (!reliable) return false;
			}
      const uint16_t sizeWHeader = size+1;
      const u8* msgPtr = (u8*)buffer;
      txBuf[wind++] = sizeWHeader & 0xff; // Size low byte
      txBuf[wind++] = (reliable ? RTIP_CLAD_MSG_RELIABLE_FLAG : 0x00) |
                      (hot      ? RTIP_CLAD_MSG_HOT_FLAG      : 0x00) |
                      ((sizeWHeader >> 8) & RTIP_CLAD_SIZE_HIGH_MASK); // Size high byte plus flags
      txBuf[wind++] = msgID;
      for (int i=0; i<size; i++) txBuf[wind++] = msgPtr[i];
      txWind = wind;
      return true;
    }
  }

  bool RadioIsConnected()
  {
    return wifiState;
  }

  void RadioUpdateState(u8 wifi, u8 blue)
  {
    wifiState = wifi;
    blueState = blue;
  }

  namespace WiFi {
    uint8_t GetTxData(uint8_t* dest, uint8_t maxLength)
    {
      const uint8_t wind = txWind;
      uint8_t rind = txRind;
      uint8_t i = 0;
      while ((rind != wind) && i < maxLength)
      {
        dest[i++] = txBuf[rind++];
      }
      txRind = rind;
      return i;
    }

    bool ReceiveMessage(uint8_t* data, uint8_t length)
    {
      const uint8_t rind = rxRind;
      uint8_t wind = rxWind;
      const int available = RX_BUF_SIZE - ((wind - rind) & RX_BUF_SIZE_MASK);
      if (length < available)
      {
        int i;
        for (i=0; i<length; i++) rxBuf[wind++] = data[i];
        rxWind = wind;
        return true;
      }
      else
      {
        return false;
      }
    }

    Result Update()
    {
      const uint8_t wind = rxWind;
      uint8_t rind = rxRind;
      if (wind == rind) return RESULT_OK; // Nothing available
      else
      {
        uint8_t available = RX_BUF_SIZE - ((rind - wind) & RX_BUF_SIZE_MASK);
        RobotInterface::EngineToRobot msg;
        while (available)
        {
          if (available >= msg.MIN_SIZE) // First pass, read the minimum there could possible be for any message
          {
            uint8_t i;
            uint8_t* msgBuffer = msg.GetBuffer();
            for (i=0; i<msg.MIN_SIZE; i++) msgBuffer[i] = rxBuf[rind++];
            uint8_t size = msg.Size();
            if (available >= size) // Second pass, read the minimum there could possibly be for this message type
            {
              for (; i<size; i++) msgBuffer[i] = rxBuf[rind++];
              size = msg.Size();
              if (available >= size) // Final pass, read the full size of this actual message
              {
                for (; i<size; i++) msgBuffer[i] = rxBuf[rind++];
                rxRind = rind;
                available = RX_BUF_SIZE - ((rind - wind) & RX_BUF_SIZE_MASK);
                Messages::ProcessMessage(msg);
                continue;
              }
            }
          }
          break;
        }
        return RESULT_OK;
      }
    }

  } // namespace wifi
} // namespace HAL
} // namespace Cozmo
} // namespace Anki
