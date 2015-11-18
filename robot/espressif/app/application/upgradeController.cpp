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
#include "foregroundTask.h"
#include "driver/i2spi.h"
#include "sha1.h"
#include "rboot.h"
}
#include "upgradeController.h"
#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/otaMessages.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#define MAX_RETRIES 2
#define SHA_CHECK_READ_LENGTH 512

namespace Anki {
namespace Cozmo {
namespace UpgradeController {

static uint8 retries;

/// Stores the state nessisary for the over the air upgrade task
typedef struct 
{
  SHA1_CTX ctx;     ///< SHA1 calculation state
  uint32   fwStart; ///< Starting address in flash of the firmware imageId
  uint32   fwSize;  ///< Number of bytes of firmware
  uint32   version; ///< Version number associated with this folder
  int32    index;   ///< Index through reading, writing etc.
  int32    phase;   ///< Operation phase
  uint8    sig[SHA1_DIGEST_LENGTH]; ///< The digest we are looking for
  RobotInterface::OTACommand cmd; ///< The command we are processing
} OTAUpgradeTaskState;

LOCAL bool TaskEraseFlash(uint32 param)
{
  RobotInterface::EraseFlash* msg = reinterpret_cast<RobotInterface::EraseFlash*>(param);
  // TODO: Add block based erase
  const uint16 sector = msg->start / SECTOR_SIZE;
  RobotInterface::FlashWriteAcknowledge resp;
  const SpiFlashOpResult rslt = spi_flash_erase_sector(sector);
  switch (rslt)
  {
    case SPI_FLASH_RESULT_OK:
    {
      os_printf("Erased sector %x\r\n", sector);
      if (msg->size > SECTOR_SIZE) // This wasn't the last sector
      {
        msg->start += SECTOR_SIZE;
        msg->size  -= SECTOR_SIZE;
        retries = MAX_RETRIES;
        return true; // Repost ourselves to erase the next sector
      }
      else // We're done erasing
      {
        resp.address = msg->start;
        resp.length  = msg->size;
        resp.writeNotErase = false;
        RobotInterface::SendMessage(resp);
        break;
      }
    }
    case SPI_FLASH_RESULT_ERR:
    {
      os_printf("Failed to erase sector %x\r\n", sector);
      resp.address = msg->start;
      resp.length  = 0; // Indicates failure
      resp.writeNotErase = false;
      RobotInterface::SendMessage(resp);
      break;
    }
    case SPI_FLASH_RESULT_TIMEOUT:
    {
      if (retries-- > 0)
      {
        foregroundTaskPost(TaskEraseFlash, param);
        return true;
      }
      else
      {
        os_printf("Timed out erasing sector %x\r\n", sector);
        resp.address = msg->start;
        resp.length  = 0; // Indicates failure
        resp.writeNotErase = false;
        RobotInterface::SendMessage(resp);
        break;
      }
    }
    default:
    {
      PRINT("ERROR: Unexpected SPI Flash Erase operation result\r\n");
      break;
    }
  }
  
  i2spiSwitchMode(I2SPI_NORMAL);
  os_free(msg);
  return false;
}

LOCAL bool TaskWriteFlash(uint32 param)
{
  RobotInterface::WriteFlash* msg = reinterpret_cast<RobotInterface::WriteFlash*>(param);
  RobotInterface::FlashWriteAcknowledge resp;
  switch(spi_flash_write(msg->address, reinterpret_cast<uint32*>(msg->data), msg->data_length))
  {
    case SPI_FLASH_RESULT_OK:
    {
      os_printf("Wrote to flash %x[%d]\r\n", msg->address, msg->data_length);
      resp.address = msg->address;
      resp.length  = msg->data_length;
      resp.writeNotErase = true;
      RobotInterface::SendMessage(resp);
      break;
    }
    case SPI_FLASH_RESULT_ERR:
    {
      os_printf("Failed to write to address %x\r\n", msg->address);
      resp.address = msg->address;
      resp.length  = 0; // Indicates failure
      resp.writeNotErase = true;
      RobotInterface::SendMessage(resp);
      break;
    }
    case SPI_FLASH_RESULT_TIMEOUT:
    {
      if (retries-- > 0)
      {
        foregroundTaskPost(TaskWriteFlash, param);
        return true;
      }
      else
      {
        os_printf("Timed out writting to address %x\r\n", msg->address);
        resp.address = msg->address;
        resp.length  = 0; // Indicates failure
        resp.writeNotErase = true;
        RobotInterface::SendMessage(resp);
        break;
      }
    }
    default:
    {
      PRINT("ERROR: Unexpected SPI Flash write operation result\r\n");
      break;
    }
  }
  
  os_free(msg);
  return false;
}

LOCAL bool TaskOtaAsset(uint32 param)
{
  OTAUpgradeTaskState* state = reinterpret_cast<OTAUpgradeTaskState*>(param);
  // TODO Do something with this new asset update
  PRINT("Asset OTA successful\r\n");
  os_free(state);
  return false;
}

LOCAL bool TaskOtaWiFi(uint32 param)
{
  BootloaderConfig blcfg;
  OTAUpgradeTaskState* state = reinterpret_cast<OTAUpgradeTaskState*>(param);
  
  blcfg.header = BOOT_CONFIG_HEADER;
  blcfg.newImageStart = state->fwStart / SECTOR_SIZE;
  blcfg.newImageSize  = state->fwSize  / SECTOR_SIZE;
  blcfg.version       = state->version;
  blcfg.chksum        = calc_chksum((uint8*)&blcfg, &blcfg.chksum);
  
  switch (spi_flash_write(BOOT_CONFIG_SECTOR * SECTOR_SIZE, reinterpret_cast<uint32*>(&blcfg), sizeof(BootloaderConfig)))
  {
    case SPI_FLASH_RESULT_OK:
    {
      // TODO tell RTIP we're going away
      os_printf("WiFi OTA rebooting for version %d\r\n", state->version);
      os_free(state);
      system_restart();
      return false;
    }
    case SPI_FLASH_RESULT_ERR:
    {
      PRINT("ERROR: Could not write bootloader config to flash for ota upgrade version %d.\r\n", state->version);
      os_free(state);
      return false;
    }
    case SPI_FLASH_RESULT_TIMEOUT:
    {
      if (retries-- > 0)
      {
        return true;
      }
      else
      {
        PRINT("ERROR: Timed out writing bootlaoder config to flash for ota upgrade version %d.\r\n", state->version);
        os_free(state);
        return false;
      }
    }
    default:
    {
      PRINT("ERROR: Unexpected flash write result writing bootloader config\r\n");
      os_free(state);
      return false;
    }
  }
}

LOCAL bool TaskOtaRTIP(uint32 param)
{
  OTAUpgradeTaskState* state = reinterpret_cast<OTAUpgradeTaskState*>(param);
  
  switch (i2spiGetRtipBootloaderState())
  {
    case STATE_NACK:
    {
      if (retries-- == 0)
      {
        PRINT("RTIP OTA transfer failure! Aborting.\r\n");
        os_free(state);
        return false;
      }
      // Have retries left, fall through and try writing
    }
    case STATE_IDLE:
    {
      switch (state->phase)
      {
        case 0: // Really idle
        {
          FirmwareBlock chunk;
          switch (spi_flash_read(state->fwStart + state->index, reinterpret_cast<uint32*>(&chunk), sizeof(FirmwareBlock)))
          {
            case SPI_FLASH_RESULT_OK:
            {
              retries = MAX_RETRIES;
              state->index += sizeof(FirmwareBlock);
              i2spiBootloaderPushChunk(&chunk);
              state->phase = 1;
              return true;
            }
            case SPI_FLASH_RESULT_ERR:
            {
              PRINT("RTIP OTA flash readback failure, aborting\r\n");
              os_free(state);
              return false;
            }
            case SPI_FLASH_RESULT_TIMEOUT:
            {
              if (retries-- > 0)
              {
                return true;
              }
              else
              {
                PRINT("RTIP OTA flash readback timeout, aborting\r\n");
                os_free(state);
                return false;
              }
            }
          }
        }
        case 1: // Returning from writing a chunk
        {
          state->index += sizeof(FirmwareBlock);
          if (static_cast<uint32>(state->index) < state->fwSize) // Have more firmware left to write
          {
            retries = MAX_RETRIES;
            state->phase = 0;
            return true;
          }
          else // Done writing firmware
          {
            i2spiSwitchMode(I2SPI_NORMAL);
            PRINT("RTIP OTA transfer complete\r\n");
            os_free(state);
            return false;
          }
        }
      }
    }
    case STATE_BUSY: // Just waiting
    case STATE_SYNC: // We will read garbage instead of sync so default is the same
    default:
    {
      return true;
    }
  }
}

LOCAL bool TaskOtaBody(uint32 param)
{
  OTAUpgradeTaskState* state = reinterpret_cast<OTAUpgradeTaskState*>(param);
  
  switch (i2spiGetBodyBootloaderState())
  {
    case STATE_NACK:
    {
      if (retries-- == 0)
      {
        PRINT("Body OTA transfer failure! Aborting.\r\n");
        os_free(state);
        return false;
      }
      // Have retries left, fall through and try writing
    }
    case STATE_IDLE:
    {
      switch (state->phase)
      {
        case 0: // Really idle
        {
          RobotInterface::EngineToRobot msg;
          msg.tag = RobotInterface::EngineToRobot::Tag_bodyUpgradeData;
          switch (spi_flash_read(state->fwStart + state->index, msg.bodyUpgradeData.data, msg.Size()))
          {
            case SPI_FLASH_RESULT_OK:
            {
              retries = MAX_RETRIES;
              state->index += sizeof(FirmwareBlock);
              i2spiQueueMessage(msg.GetBuffer(), msg.Size());
              state->phase = 1;
              return true;
            }
            case SPI_FLASH_RESULT_ERR:
            {
              PRINT("Body OTA flash readback failure, aborting\r\n");
              os_free(state);
              return false;
            }
            case SPI_FLASH_RESULT_TIMEOUT:
            {
              if (retries-- > 0)
              {
                return true;
              }
              else
              {
                PRINT("Body OTA flash readback timeout, aborting\r\n");
                os_free(state);
                return false;
              }
            }
          }
        }
        case 1: // Returning from writing a chunk
        {
          state->index += sizeof(FirmwareBlock);
          if (static_cast<uint32>(state->index) < state->fwSize) // Have more firmware left to write
          {
            retries = MAX_RETRIES;
            state->phase = 0;
            return true;
          }
          else // Done writing firmware
          {
            PRINT("Body OTA transfer complete\r\n");
            os_free(state);
            return false;
          }
        }
      }
    }
    default:
    {
      return true;
    }
  }
}

LOCAL bool TaskCheckSig(uint32 param)
{
  OTAUpgradeTaskState* state = reinterpret_cast<OTAUpgradeTaskState*>(param);
  uint32 buffer[SHA_CHECK_READ_LENGTH/4]; // Allocate as uint32 for alignment
  const uint32 remaining  = state->fwSize - state->index;
  const uint32 readLength = SHA_CHECK_READ_LENGTH < remaining ? SHA_CHECK_READ_LENGTH : remaining;
  switch (spi_flash_read(state->fwStart + state->index, buffer, readLength))
  {
    case SPI_FLASH_RESULT_OK:
    {
      SHA1Update(&(state->ctx), reinterpret_cast<uint8*>(buffer), readLength);
      retries = MAX_RETRIES;
      break;
    }
    case SPI_FLASH_RESULT_ERR:
    {
      PRINT("ERROR reading back flash at %x for signature check\r\n", state->index);
      os_free(state);
      return false;
    }
    case SPI_FLASH_RESULT_TIMEOUT:
    {
      if (retries-- > 0)
      {
        return true; // Just retry
      }
      else
      {
        PRINT("ERROR timed out reading back flash at %x for signature check\r\n", state->index);
        os_free(state);
        return false;
      }
    }
    default:
    {
      PRINT("ERROR unexpected flash result when reading back at %x for signature check\r\n", state->index);
    }
  }
  
  if (remaining > SHA_CHECK_READ_LENGTH)
  {
    state->index += readLength;
    return true;
  }
  else
  {
    int i;
    uint8 digest[SHA1_DIGEST_LENGTH];
    SHA1Final(digest, &(state->ctx));
    for (i=0; i<SHA1_DIGEST_LENGTH; i++)
    {
      if (digest[i] != state->sig[i])
      {
        PRINT("Firmware signature missmatch at character %d, %02x != %02x\r\n", i, digest[i], state->sig[i]);
        os_free(state);
        return false;
      }
    }
    // If we got here, the signature matched
    retries = MAX_RETRIES;
    state->index = 0;
    switch(state->cmd)
    {
      case RobotInterface::OTA_none:
      {
        PRINT("Successfully confirmed flash signature for OTA none\r\n");
        os_free(state);
        return false;
      }
      case RobotInterface::OTA_asset:
      {
        foregroundTaskPost(TaskOtaAsset, param);
        return false;
      }
      case RobotInterface::OTA_WiFi:
      {
        foregroundTaskPost(TaskOtaWiFi, param);
        return false;
      }
      case RobotInterface::OTA_RTIP:
      {
        uint8_t msg[2]; // This is based on the EnterBootloader message which is defined in otaMessages.clad
        msg[0] = RobotInterface::EngineToRobot::Tag_enterBootloader;
        msg[1] = WiFiToRTIP::BOOTLOAD_RTIP;
        i2spiQueueMessage(msg, 2); // Send message to put the RTIP in bootloader mode
        i2spiSwitchMode(I2SPI_BOOTLOADER);
        retries = MAX_RETRIES;
        foregroundTaskPost(TaskOtaRTIP, param);
        return false;
      }
      case RobotInterface::OTA_body:
      {
        uint8_t msg[2]; // This is based on the EnterBootloader message which is defined in otaMessages.clad
        msg[0] = RobotInterface::EngineToRobot::Tag_enterBootloader;
        msg[1] = WiFiToRTIP::BOOTLOAD_BODY;
        i2spiQueueMessage(msg, 2); // Send message to put the RTIP in bootloader mode
        retries = MAX_RETRIES;
        foregroundTaskPost(TaskOtaBody, param);
        return false;
      }
      default:
      {
        PRINT("ERROR: Unexpected OTA command %d\r\n", state->cmd);
        os_free(state);
        return false;
      }
    }
  }
}
  
void EraseFlash(RobotInterface::EraseFlash& msg)
{
  if (msg.start < FLASH_WRITE_START_ADDRESS) // Refuse to erase addresses that are too low
  {
    PRINT("WARNING: Refusing to erase flash address %x, below %x\r\n", msg.start, FLASH_WRITE_START_ADDRESS);
  }
  else
  {
    RobotInterface::EraseFlash* taskMsg = static_cast<RobotInterface::EraseFlash*>(os_zalloc(msg.Size()));
    if (taskMsg == NULL)
    {
      PRINT("Failed to allocate memory for flash erase task\r\n");
    }
    else
    {
      os_memcpy(taskMsg, &msg, msg.Size());
      retries = MAX_RETRIES;
      i2spiSwitchMode(I2SPI_PAUSED);
      foregroundTaskPost(TaskEraseFlash, reinterpret_cast<uint32>(taskMsg));
    }
  }
}

void WriteFlash(RobotInterface::WriteFlash& msg)
{
  if (msg.address < FLASH_WRITE_START_ADDRESS) // Refuse to write addresses that are too low
  {
    PRINT("WARNING Refusing to write flash address %x, below %x\r\n", msg.address, FLASH_WRITE_START_ADDRESS);
  }
  else
  {
    RobotInterface::WriteFlash* taskMsg = static_cast<RobotInterface::WriteFlash*>(os_zalloc(msg.Size()));
    if (taskMsg == NULL)
    {
      PRINT("Failed to allocate memory for flash write task\r\n");
    }
    else
    {
      os_memcpy(taskMsg, &msg, msg.Size());
      retries = MAX_RETRIES;
      foregroundTaskPost(TaskWriteFlash, reinterpret_cast<uint32>(taskMsg));
    }
  }
}
  
  void Trigger(RobotInterface::OTAUpgrade& msg)
  {
    OTAUpgradeTaskState* otaState = static_cast<OTAUpgradeTaskState*>(os_zalloc(sizeof(OTAUpgradeTaskState)));
    if (otaState == NULL)
    {
      PRINT("Failed to allocate memory for upgrade task\r\n");
    }
    else
    {
      os_printf("OTA finish: fwStart=%x, fwSize=%x, version=%d, command=%d\r\n",
                msg.start, msg.size, msg.version, msg.command);
      retries = MAX_RETRIES;
      SHA1Init(&(otaState->ctx)); // Initalize the SHA1
      otaState->fwStart = msg.start;
      otaState->fwSize  = msg.size;
      otaState->version = msg.version;
      otaState->index   = 0;
      otaState->phase   = 0;
      otaState->cmd     = msg.command;
      os_memcpy(&(otaState->sig), msg.sig, SHA1_DIGEST_LENGTH);
      foregroundTaskPost(TaskCheckSig, reinterpret_cast<uint32>(otaState));
    }
  }
}
}
}
