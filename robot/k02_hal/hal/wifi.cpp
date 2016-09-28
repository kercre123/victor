/** @file Implementation of RTIP interface to WiFi processor
 * @author Daniel Casner <daniel@anki.com>
 */

#include "wifi.h"
#include "messages.h"
#include "spine.h"
#include "uart.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "anki/cozmo/robot/logging.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "MK02F12810.h"

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
static u32 rxDroppedCount = 0;

namespace Anki {
namespace Cozmo {
namespace HAL {
  bool RadioSendMessage(const void* buffer, const u16 size, const u8 msgID)
  {
    const uint16_t sizeWHeader = size+1;
    const bool reliable = msgID < RobotInterface::TO_ENG_UNREL;
    const u8 tag = ((msgID == RobotInterface::GLOBAL_INVALID_TAG) ? *reinterpret_cast<const u8*>(buffer) : msgID);

    if (tag < RobotInterface::TO_RTIP_START)
    {
      return Spine::Enqueue(buffer, size, msgID);
    }
    else if (tag <= RobotInterface::TO_RTIP_END)
    {
      AnkiWarn( 143, "WiFi.RadioSendMessage", 395, "Refusing to send message %x[%d] to self!", 2, tag, size);
      return false;
    }
    else
    {
      AnkiConditionalErrorAndReturnValue(sizeWHeader <= RTIP_MAX_CLAD_MSG_SIZE, false, 41, "WiFi", 260, "Can't send message %x[%d] to WiFi, max size %d\r\n", 3, msgID, size, RTIP_MAX_CLAD_MSG_SIZE);
      uint8_t wind = txWind;
      int available = TX_BUF_SIZE - ((wind - txRind) & TX_BUF_SIZE_MASK);
      while (available < (sizeWHeader + 2)) // Wait for room for message plus tag plus header plus one more so we can tell empty from full
      {
        if (!reliable) return false;
        else available = TX_BUF_SIZE - ((wind - txRind) & TX_BUF_SIZE_MASK);
      }
      const u8* msgPtr = (u8*)buffer;
      txBuf[wind++] = sizeWHeader;
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
      
      if ((length+1) < available)
      {
        int i;
        rxBuf[wind++] = length;
        for (i=0; i<length; i++) rxBuf[wind++] = data[i];
        rxWind = wind;
        return true;
      }
      else
      {
        rxDroppedCount++;
        return false;
      }
    }

    Result Update()
    {
      const uint8_t wind = rxWind;
      uint8_t rind = rxRind;
      
      if (rxDroppedCount > 0)
      {
        AnkiError( 395, "wifi.dropped_incomming_messages", 615, "%d messages dropped, tick %d, time %d, buffer %x %x %d", 6, rxDroppedCount, HAL::GetTimeStamp(), SysTick->VAL, rxBuf[rind], rxBuf[rind+1], RX_BUF_SIZE - ((rind - wind) & RX_BUF_SIZE_MASK));
        rxDroppedCount = 0;
      }

      
      if (wind == rind) return RESULT_OK; // Nothing available
      else
      {
        uint8_t available = RX_BUF_SIZE - ((rind - wind) & RX_BUF_SIZE_MASK);
        while ((available) && (available > rxBuf[rind]))
        {
          uint32_t cladBuffer[DROP_TO_RTIP_MAX_VAR_PAYLOAD]  __attribute__ ((aligned (4)));

          RobotInterface::EngineToRobot& msg = *reinterpret_cast<RobotInterface::EngineToRobot*>(cladBuffer);
          uint8_t* msgBuffer = msg.GetBuffer();
          
          const uint8_t msgLen = rxBuf[rind++];
          for (uint8_t i=0; i<msgLen; i++) msgBuffer[i] = rxBuf[rind++];
          rxRind = rind;
          available = RX_BUF_SIZE - ((rind - wind) & RX_BUF_SIZE_MASK);

          if ((msg.tag > RobotInterface::TO_RTIP_END) && ((msg.tag < RobotInterface::ANIM_RT_START) || (msg.tag > RobotInterface::ANIM_RT_END)))
          {
            AnkiError( 141, "WiFi.Update", 380, "Got message 0x%x that seems bound above.", 1, msg.tag);
          }
          else if (msg.tag < RobotInterface::TO_RTIP_START && ((msg.tag < RobotInterface::RTIP_TO_BODY_START) || (msg.tag > RobotInterface::RTIP_TO_BODY_END)))
          {
            while (Spine::Enqueue(msg.GetBuffer() + 1, msg.Size() - 1, msg.tag) == false) ; // Spin until success
          }
          else if (msg.Size() != msgLen)
          {
            AnkiError( 141, "WiFi.Update", 381, "CLAD message 0x%x size %d doesn't match size in buffer %d", 3, msg.tag, msg.Size(), msgLen);
          }
          else
          {
            Messages::ProcessMessage(msg);
          }
        }

        return RESULT_OK;
      }
    }

  } // namespace WiFi
} // namespace HAL
} // namespace Cozmo
} // namespace Anki
