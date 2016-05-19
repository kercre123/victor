/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif and other chips in Cozmo
 * @author Daniel Casner <daniel@anki.com>
 */
extern "C" {
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "client.h"
#include "driver/i2spi.h"
#include "sha1.h"
#include "rboot.h"
}
#include "upgradeController.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/rec_protocol.h"
#include "rtip.h"
#include "animationController.h"
#include "face.h"
#include "clad/robotInterface/otaMessages.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#define ESP_FW_WRITE_ADDRESS (0x100004)
#define ESP_FW_MAX_SIZE      (0x080000)
#define ESP_FW_ADDR_MASK     (0x07FFFF)

#define WRITE_DATA_SIZE (1024)

#define MAX_RETRIES 2
#define SHA_CHECK_READ_LENGTH 512

#define CRC32_POLYNOMIAL (0xedb88320L)

#define FLASH_WRITE_COUNT SECTOR_SIZE
#define RTIP_WRITE_COUNT  TRANSMIT_BLOCK_SIZE

#define DEBUG_OTA 1

namespace Anki {
namespace Cozmo {
namespace UpgradeController {

  #define DEBUG_OTA 1

  typedef enum
  {
    OTAT_Uninitalized = 0,
    OTAT_Ready,
    OTAT_Delay,
    OTAT_Flash_Erase,
    OTAT_Sync_Recovery,
    OTAT_Flash_Write,
    OTAT_Wait,
    OTAT_Sig_Check,
    OTAT_Reject,
    OTAT_Apply_WiFi,
    OTAT_Apply_RTIP,
    OTAT_Wait_For_RTIP_Ack,
    OTAT_Reboot,
    OTAT_Wait_For_Reboot,
  } OTATaskPhase;

  uint8_t* buffer; ///< Pointer to RX buffer
  int32_t  bufferSize; ///< Number of bytes of storage at buffer
  int32_t  bufferUsed; ///< Number of bytes of storage buffer currently filled
  int32_t  bytesProcessed; ///< The number of bytes of OTA data that have been processed from the buffer
  uint32_t counter; ///< A in phase counter
  int16_t  acceptedPacketNumber; ///< Highest packet number we have accepted
  int8_t   retries; ///< Flash operation retries
  OTATaskPhase phase; ///< Keeps track of our current task state
  bool didEsp; ///< We have new firmware for the Espressif
  bool haveTermination; ///< Have received termination

  bool Init()
  {
    
    buffer = NULL;
    bufferSize = 0;
    bufferUsed = 0;
    bytesProcessed = 0;
    counter = system_get_time();
    acceptedPacketNumber = -1;
    retries = MAX_RETRIES;
    phase = OTAT_Ready;
    didEsp = false;
    haveTermination = false;
    return true;
  }
  
  LOCAL void Reset()
  {
    AnkiDebug( 170, "UpdateController", 466, "Reset()", 0);
    buffer = NULL;
    bufferSize = 0;
    bufferUsed = 0;
    bytesProcessed = 0;
    acceptedPacketNumber = -1;
    retries = MAX_RETRIES;
    phase = OTAT_Ready;
    didEsp = false;
    haveTermination = false;
    AnimationController::ResumeAndRestoreBuffer();
    i2spiBootloaderCommandDone();
    //i2spiSwitchMode(I2SPI_REBOOT);
  }

  LOCAL uint32_t calc_crc32(const uint8_t* data, int length)
  {
    uint32_t checksum = 0xFFFFFFFF;

    while (length-- > 0) {
      checksum ^= *(data++);
      for (int b = 0; b < 8; b++) {
        checksum = (checksum >> 1) ^ ((checksum & 1) ? CRC32_POLYNOMIAL : 0);
      }
    }

    return ~checksum;
  }

  LOCAL bool VerifyFirmwareBlock(const FirmwareBlock* fwb)
  {
    return calc_crc32(reinterpret_cast<const uint8*>(fwb->flashBlock), TRANSMIT_BLOCK_SIZE) == fwb->checkSum;
  }

  void Write(RobotInterface::OTA::Write& msg)
  {
    using namespace RobotInterface::OTA;
    Ack ack;
    ack.bytesProcessed = bytesProcessed;
    ack.packetNumber = acceptedPacketNumber;
    #if DEBUG_OTA > 1
    os_printf("UpC: Write(%d, %x...)\r\n", msg.packetNumber, msg.data[0]);
    #endif
    switch (phase)
    {
      case OTAT_Uninitalized:
      {
        ack.result = NOT_ENABLED;
        RobotInterface::SendMessage(ack);
        break;
      }
      case OTAT_Ready:
      {
        if (msg.packetNumber != 0)
        {
          #if DEBUG_OTA
          os_printf("\tOOO %d\r\n", msg.packetNumber);
          #endif
          ack.result = OUT_OF_ORDER;
          RobotInterface::SendMessage(ack);
          break;
        }
        else
        {
          #if DEBUG_OTA
          os_printf("\tStarting upgrade\r\n");
          #endif
          bufferSize = AnimationController::SuspendAndGetBuffer(&buffer);
          Face::FacePrintf("Starting FOTA\nupgrade...");
          counter = system_get_time() + 100000; /// 100 ms delay
          phase = OTAT_Delay;
          // Explicit fallthrough to next case
        }
      }
      case OTAT_Delay:
      case OTAT_Flash_Erase:
      case OTAT_Sync_Recovery:
      case OTAT_Flash_Write:
      case OTAT_Wait:
      {
        if ((bufferSize - bufferUsed) < WRITE_DATA_SIZE)
        {
          #if DEBUG_OTA
          os_printf("\tNo mem p=%d s=%d u=%d\r\n", bytesProcessed, bufferSize, bufferUsed);
          #endif
          ack.result = ERR_NO_MEM;
          RobotInterface::SendMessage(ack);
        }
        else if (msg.packetNumber == -1) // End of data flag
        {
          #if DEBUG_OTA
          os_printf("\tTermination\r\n");
          #endif
          AnkiDebug( 171, "UpgradeController", 467, "Received termination", 0)
          haveTermination = true;
        }
        else if (msg.packetNumber != (acceptedPacketNumber + 1))
        {
          #if DEBUG_OTA
          os_printf("\tOOO\r\n");
          #endif
          ack.result = OUT_OF_ORDER;
          RobotInterface::SendMessage(ack);
        }
        else
        {
          os_memcpy(buffer + bufferUsed, msg.data, WRITE_DATA_SIZE);
          bufferUsed += WRITE_DATA_SIZE;
          acceptedPacketNumber = msg.packetNumber;
        }
        break;
      }
      case OTAT_Sig_Check:
      case OTAT_Reject:
      case OTAT_Apply_WiFi:
      case OTAT_Apply_RTIP:
      case OTAT_Wait_For_RTIP_Ack:
      case OTAT_Reboot:
      case OTAT_Wait_For_Reboot:
      {
        ack.result = BUSY;
        RobotInterface::SendMessage(ack);
        return;
      }
    }
  }
  
  void Update()
  {
    using namespace RobotInterface::OTA;
    Ack ack;
    ack.bytesProcessed = bytesProcessed;
    ack.packetNumber   = acceptedPacketNumber;
    switch (phase)
    {
      case OTAT_Uninitalized:
      case OTAT_Ready:
      {
        break;
      }
      case OTAT_Delay:
      {
        if ((system_get_time() > counter) && i2spiMessageQueueIsEmpty())
        {
          i2spiSwitchMode(I2SPI_RECOVERY);
          phase = OTAT_Flash_Erase;
          retries = MAX_RETRIES;
          counter = 0;
          AnkiDebug( 172, "UpgradeController.state", 468, "flash erase", 0);
        }
        break;
      }
      case OTAT_Flash_Erase:
      {
        if (counter < ESP_FW_MAX_SIZE)
        {
          const uint32 sector = (ESP_FW_WRITE_ADDRESS + counter) / SECTOR_SIZE;
          #if DEBUG_OTA
          os_printf("Erase sector 0x%x\r\n", sector);
          #endif
          const SpiFlashOpResult rslt = spi_flash_erase_sector(sector);
          if (rslt != SPI_FLASH_RESULT_OK)
          {
            if (retries-- <= 0)
            {
              ack.result = rslt == SPI_FLASH_RESULT_ERR ? ERR_ERASE_ERROR : ERR_ERASE_TIMEOUT;
              RobotInterface::SendMessage(ack);
              Reset();
            }
          }
          else
          {
            retries = MAX_RETRIES;
            counter += SECTOR_SIZE;
          }
        }
        else // Done with erase, advance state
        {
          AnkiDebug( 172, "UpgradeController.state", 469, "sync recovery", 0);
          i2spiSwitchMode(I2SPI_BOOTLOADER); // Start synchronizing with the bootloader
          phase = OTAT_Sync_Recovery;
          counter = system_get_time() + 2000000; // 2 second timeout
          retries = MAX_RETRIES;
        }
        break;
      }
      case OTAT_Sync_Recovery:
      {
        if (i2spiGetRtipBootloaderState() == STATE_IDLE)
        {
          AnkiDebug( 172, "UpgradeController.state", 470, "flash write", 0);
          phase = OTAT_Flash_Write;
          counter = 0; // 2 second timeout
          retries = MAX_RETRIES;
          ack.result = OKAY;
          RobotInterface::SendMessage(ack);
        }
        else if (system_get_time() > counter)
        {
          #if DEBUG_OTA
          os_printf("Sync recovery timeout!\r\n");
          #endif
          ack.result = ERR_RTIP_TIMEOUT;
          RobotInterface::SendMessage(ack);
          Reset();
        }
        break;
      }
      case OTAT_Flash_Write:
      {
        if (bufferUsed < (int)sizeof(FirmwareBlock)) // We've reached the end of the buffer
        {
          if (haveTermination) // If there's no more, advance now
          {
            AnkiDebug( 172, "UpgradeController.state", 457, "Sig check", 0);
            counter = 0;
            retries = MAX_RETRIES;
            phase = OTAT_Sig_Check;
          }
        }
        else
        {
          FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);
          if (VerifyFirmwareBlock(fwb) == false)
          {
            #if DEBUG_OTA
            os_printf("Bad block CRC at %d\r\n", bytesProcessed);
            #endif
            ack.result = ERR_BAD_DATA;
            RobotInterface::SendMessage(ack);
            Reset();
          }
          else
          {
            if (fwb->blockAddress == 0xFFFFffff)
            {
              // TODO something with signature header
              #if DEBUG_OTA
              os_printf("OTA Sig header\r\n");
              #endif
              retries = MAX_RETRIES;
              bufferUsed -= sizeof(FirmwareBlock);
              os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
              bytesProcessed += sizeof(FirmwareBlock);
              ack.bytesProcessed = bytesProcessed;
              ack.result = OKAY;
              RobotInterface::SendMessage(ack);
            }
            else if (fwb->blockAddress == 0xFFFFfffc) // Comment block, ignore
            {
              #if DEBUG_OTA
              os_printf("OTA comment\r\n");
              #endif
              retries = MAX_RETRIES;
              bufferUsed -= sizeof(FirmwareBlock);
              os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
              bytesProcessed += sizeof(FirmwareBlock);
              ack.bytesProcessed = bytesProcessed;
              ack.result = OKAY;
              RobotInterface::SendMessage(ack);
            }
            else if (fwb->blockAddress & 0x40000000) // Destined for the Espressif flash
            {
              const uint32 destAddr = ESP_FW_WRITE_ADDRESS + (fwb->blockAddress & ESP_FW_ADDR_MASK);
              #if DEBUG_OTA
              os_printf("WF 0x%x\r\n", destAddr);
              #endif
              const SpiFlashOpResult rslt = spi_flash_write(destAddr, fwb->flashBlock, TRANSMIT_BLOCK_SIZE);
              if (rslt != SPI_FLASH_RESULT_OK)
              {
                if (retries-- <= 0)
                {
                  ack.result = rslt == SPI_FLASH_RESULT_ERR ? ERR_WRITE_ERROR : ERR_WRITE_TIMEOUT;
                  RobotInterface::SendMessage(ack);
                  Reset();
                }
              }
              else
              {
                didEsp = true;
                retries = MAX_RETRIES;
                bufferUsed -= sizeof(FirmwareBlock);
                os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
                bytesProcessed += sizeof(FirmwareBlock);
                ack.bytesProcessed = bytesProcessed;
                ack.result = OKAY;
                RobotInterface::SendMessage(ack);
              }
            }
            else // This is bound for the RTIP or body
            {
              switch (i2spiGetRtipBootloaderState())
              {
                case STATE_NACK:
                case STATE_ACK:
                case STATE_IDLE:
                {
                  if (i2spiBootloaderPushChunk(fwb))
                  {
                    #if DEBUG_OTA
                    os_printf("Write RTIP 0x%x\t", fwb->blockAddress);
                    #endif
                    phase = OTAT_Wait;
                  }
                  // Else try again next time
                  break;
                }
                case STATE_BUSY:
                case STATE_SYNC:
                default:
                {
                  // Just wait for this to clear
                  #if DEBUG_OTA > 2
                  os_printf("w4r %x\r\n", i2spiGetRtipBootloaderState());
                  #endif
                  break;
                }
              }
            }
          }
        }
        break;
      }
      case OTAT_Wait:
      {
        switch(i2spiGetRtipBootloaderState())
        {
          case STATE_NACK:
          {
            os_printf("RTIP NACK!\r\n");
            if (retries-- <= 0)
            {
              ack.result = ERR_I2SPI;
              RobotInterface::SendMessage(ack);
              Reset();
            }
            phase = OTAT_Flash_Write; // try again
            break;
          }
          case STATE_ACK:
          case STATE_BUSY:
          {
            os_printf("Done\r\n");
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            phase = OTAT_Flash_Write; // Finished operation
          }
          case STATE_IDLE:
          {
            break;
          }
          default:
          {
            os_printf("OW %d\r\n", i2spiGetRtipBootloaderState());
            break;
          }
        }
        break;
      }
      case OTAT_Sig_Check:
      {
        AnkiWarn( 171, "UpgradeController", 458, "Signature check not implemented", 0);
        AnkiDebug( 172, "UpgradeController.state", 459, "Apply WiFi", 0)
        phase   = OTAT_Apply_WiFi;
        retries = MAX_RETRIES;
        break;
      }
      case OTAT_Reject:
      {
        AnkiError( 171, "UpgradeController", 460, "Factory reset not yet implemented", 0);
        Reset();
        break;
      }
      case OTAT_Apply_WiFi:
      {
        SpiFlashOpResult rslt;
        if (didEsp)
        {
          BootloaderConfig blcfg;
          blcfg.header        = BOOT_CONFIG_HEADER;
          blcfg.newImageStart = ESP_FW_WRITE_ADDRESS / SECTOR_SIZE;
          blcfg.newImageSize  = ESP_FW_MAX_SIZE      / SECTOR_SIZE;
          blcfg.version       = 0;
          blcfg.chksum        = calc_chksum((uint8*)&blcfg, &blcfg.chksum);
          rslt = spi_flash_write(BOOT_CONFIG_SECTOR * SECTOR_SIZE, reinterpret_cast<uint32*>(&blcfg), sizeof(BootloaderConfig));
        }
        else
        {
          rslt = SPI_FLASH_RESULT_OK;
        }
        if (rslt != SPI_FLASH_RESULT_OK)
        {
          if (retries-- <= 0)
          {
            ack.result = rslt == SPI_FLASH_RESULT_ERR ? ERR_WRITE_ERROR : ERR_WRITE_TIMEOUT;
            RobotInterface::SendMessage(ack);
            Reset();
          }
        }
        else
        {
          AnkiDebug( 172, "UpgradeController.state", 461, "Reboot", 0);
          phase   = OTAT_Reboot;
          //phase   = OTAT_Apply_RTIP;
          //counter = system_get_time() + 10000000; // 10 second timeout
          retries = MAX_RETRIES;
        }
        break;
      }
      case OTAT_Apply_RTIP:
      {
        if(i2spiGetRtipBootloaderState() == STATE_IDLE)
        {
          if (i2spiBootloaderCommandDone() == false)
          {
            #if DEBUG_OTA
            os_printf("NSD\r\n");
            #endif
          }
          else
          {
            AnkiDebug( 172, "UpgradeController.state", 462, "Wait for RTIP Ack", 0);
            phase = OTAT_Wait_For_RTIP_Ack;
            counter = system_get_time() + 2000000; // 2 second timeout
          }
        }
        else if (system_get_time() > counter)
        {
          #if DEBUG_OTA
          os_printf("Apply RTIP timeout, state %x\r\n", i2spiGetRtipBootloaderState());
          #endif
          ack.result = ERR_RTIP_TIMEOUT;
          RobotInterface::SendMessage(ack);
          Reset();
        }
        break;
      }
      case OTAT_Wait_For_RTIP_Ack:
      {
        switch (i2spiGetRtipBootloaderState())
        {
          case STATE_BUSY:
          {
            // Wait
            if (system_get_time() > counter)
            {
              ack.result = ERR_RTIP_TIMEOUT;
              RobotInterface::SendMessage(ack);
              Reset();
            }
            break;
          }
          case STATE_NACK:
          {
            ack.result = ERR_RTIP_NAK;
            RobotInterface::SendMessage(ack);
            Reset();
            break;
          }
          case STATE_ACK:
          case STATE_IDLE:
          default: // If the RTIP has booted we'll get other gibberish
          {
            AnkiDebug( 172, "UpgradeController.state", 463, "Reboot", 0);
            phase = OTAT_Reboot;
            break;
          }
        }
        break;
      }
      case OTAT_Reboot:
      {
        i2spiBootloaderCommandDone();
        ack.result = SUCCESS;
        RobotInterface::SendMessage(ack);
        AnkiDebug( 172, "UpgradeController.state", 464, "Wait for reboot", 0);
        phase = OTAT_Wait_For_Reboot;
        break;
      }
      case OTAT_Wait_For_Reboot:
      {
        break;
      }
      default:
      {
        AnkiAssert(false, 465);
      }
    }
  }

}
}
}
