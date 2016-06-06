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
#include "flash_map.h"
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

#include "sha512.h"
#include "aes.h"
#include "bignum.h"
#include "publickey.h"
#define ESP_FW_MAX_SIZE  (0x07c000)
#define ESP_FW_ADDR_MASK (0x07FFFF)

#define WRITE_DATA_SIZE (1024)

#define MAX_RETRIES 2
#define SHA_CHECK_READ_LENGTH 512

#define CRC32_POLYNOMIAL (0xedb88320L)

#define FLASH_WRITE_COUNT SECTOR_SIZE
#define RTIP_WRITE_COUNT  TRANSMIT_BLOCK_SIZE

#define DEBUG_OTA 1

void gen_random(void* ptr, int length)
{
  os_get_random((unsigned char *)ptr, length);
}

namespace Anki {
namespace Cozmo {
namespace UpgradeController {

  #define DEBUG_OTA 1

  typedef enum
  {
    OTAT_Uninitalized = 0,
    OTAT_Ready,
    OTAT_Enter_Recovery,
    OTAT_Delay,
    OTAT_Flash_Erase,
    OTAT_Sync_Recovery,
    OTAT_Flash_Verify,
    OTAT_Flash_Write,
    OTAT_Wait,
    OTAT_Sig_Check,
    OTAT_Reject,
    OTAT_Apply_WiFi,
    OTAT_Apply_RTIP,
    OTAT_Wait_For_RTIP_Ack,
    OTAT_Reboot,
    OTAT_Wait_For_Reboot,
    OTATR_Set_Evil_A,
    OTATR_Set_Evil_B,
    OTATR_Delay,
    OTATR_Get_Size,
    OTATR_Read,
    OTATR_Write,
    OTATR_Wait,
    OTATR_Apply,
  } OTATaskPhase;

  uint32_t fwWriteAddress; ///< Where we will write the next image
  uint32_t nextImageNumber; ///< Image number for the next upgrade
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
  
  static sha512_state firmware_digest;
  static uint8_t aes_iv[AES_KEY_LENGTH];
  static bool aes_enabled;

  void FactoryUpgrade()
  {
    #include "cboot.bin.h"
    uint32_t buffer[(firmware_cboot_bin_len/4)+1];
    if (spi_flash_read(0, buffer, firmware_cboot_bin_len) != SPI_FLASH_RESULT_OK)
    {
      os_printf("Couldn't read back existing bootloader aborting.\r\n");
    }
    else
    {
      uint8_t* charBuffer = (uint8_t*)buffer;
      if (os_memcmp(charBuffer, firmware_cboot_bin, firmware_cboot_bin_len))
      {
        os_printf("Bootloader doesn't match\r\nValidating our own header\r\n");
        uint32_t headerValidation[] = {1, 0};
        while (spi_flash_write((APPLICATION_A_SECTOR * SECTOR_SIZE) + APP_IMAGE_HEADER_OFFSET + 4, headerValidation, 8));
        os_printf("Upgrading bootloader\r\n");
        while (spi_flash_erase_sector(0) != SPI_FLASH_RESULT_OK);
        while (spi_flash_write(0, (uint32_t*)firmware_cboot_bin, firmware_cboot_bin_len) != SPI_FLASH_RESULT_OK);
        os_printf("Bootloader upgraded\r\n");
      }
    }
  }

  bool Init()
  {
    buffer = NULL;
    bufferSize = 0;
    bufferUsed = 0;
    bytesProcessed = 0;
    counter = system_get_time();
    acceptedPacketNumber = -1;
    retries = MAX_RETRIES;
    didEsp = false;
    haveTermination = false;
    sha512_init(firmware_digest);
    aes_enabled = false;
    
    if (FACTORY_FIRMWARE == 0) FactoryUpgrade();
    
    // No matter which of the three images we're loading, we can get a header here
    const AppImageHeader* const ourHeader = (const AppImageHeader* const)(FLASH_MEMORY_MAP + APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET);
    
    if (FACTORY_FIRMWARE)
    {
      // Factory firmware can read A segment header so it's still valid
      fwWriteAddress = APPLICATION_A_SECTOR * SECTOR_SIZE;
      nextImageNumber = ourHeader->imageNumber + 2; // In case slot B was higher
    }
    else
    {
      uint32 selectedImage;
      system_rtc_mem_read(RTC_IMAGE_SELECTION, &selectedImage, 4);
      nextImageNumber = ourHeader->imageNumber + 1;
      switch (selectedImage)
      {
        case FW_IMAGE_A:
        {
          os_printf("Am image A\r\n");
          fwWriteAddress = APPLICATION_B_SECTOR * SECTOR_SIZE;
          break;
        }
        case FW_IMAGE_B:
        {
          os_printf("Am image B\r\n");
          fwWriteAddress = APPLICATION_A_SECTOR * SECTOR_SIZE;
          break;
        }
        default:
        {
          os_printf("UPC: Unexpected selectedImage key %08x. Not enabling OTA\r\n", selectedImage);
          phase = OTAT_Uninitalized;
          return false;
        }
      }
      
    }
    
    phase = OTAT_Ready;
    return true;
  }
  
  LOCAL void Reset()
  {
    os_printf("upgradeController Reset!!!\r\n");
    AnkiDebug( 170, "UpdateController", 466, "Reset()", 0);
    if (phase < OTAT_Enter_Recovery)
    {
      i2spiSwitchMode(I2SPI_REBOOT);
      while (true);
    }
    else 
    {
      while (true) i2spiBootloaderCommandDone();
    }
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
          #if FACTORY_FIRMWARE == 0
          Face::FacePrintf("Starting FOTA\nupgrade...");
          #endif
          phase = OTAT_Enter_Recovery;
          // Explicit fallthrough to next case
        }
      }
      case OTAT_Enter_Recovery:
      case OTAT_Delay:
      case OTAT_Flash_Erase:
      case OTAT_Sync_Recovery:
      case OTAT_Flash_Verify:
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
          os_printf("\tEOF\r\n");
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
      default:
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
      case OTAT_Enter_Recovery:
      {
        const int8_t mode = OTA_Mode;
        if (RTIP::SendMessage((const uint8_t*)&mode, 1, RobotInterface::EngineToRobot::Tag_enterRecoveryMode))
        {
          counter = 0xFFFFffff;
          phase = OTAT_Delay;
        }
        break;
      }
      case OTAT_Delay:
      {
        if ((counter == 0xFFFFffff) && i2spiMessageQueueIsEmpty() && (Face::GetRemainingRects() == 0))
        {
          counter = system_get_time() + 20000; // Wait for i2c
        }
        else if (system_get_time() > counter)
        {
          i2spiSwitchMode(I2SPI_PAUSED);
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
          const uint32 sector = (fwWriteAddress + counter) / SECTOR_SIZE;
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
          phase = OTAT_Flash_Verify;
          counter = 0;
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
      case OTAT_Flash_Verify:
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
          else if (fwb->blockAddress != CERTIFICATE_BLOCK)
          {
            /*
            sha512_process(firmware_digest, &fwb, sizeof(FirmwareBlock));
            if ((fwb->blockAddress & SPECIAL_BLOCK) != SPECIAL_BLOCK) {
              aes_cfb_decode(
                AES_KEY,
                aes_iv,
                (uint8_t*) fwb->flashBlock, 
                (uint8_t*) fwb->flashBlock, 
                sizeof(fwb->flashBlock), 
                aes_iv);
            }
            */
            retries = MAX_RETRIES;
            phase = OTAT_Flash_Write;
          }
        }
        break;
      }
      case OTAT_Flash_Write:
      {
        FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);

        if ((fwb->blockAddress & SPECIAL_BLOCK) == SPECIAL_BLOCK)
        {
          if (fwb->blockAddress == CERTIFICATE_BLOCK)
          {
            // TODO: VERIFY SIGNATURE WITH CURRENT DIGEST
            // TODO: RESET DIGEST USING sha512_init

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
          else if (fwb->blockAddress == COMMENT_BLOCK)
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
          else if (fwb->blockAddress == HEADER_BLOCK)
          {
            FirmwareHeaderBlock* head = reinterpret_cast<FirmwareHeaderBlock*>(fwb->flashBlock);
            
            #if DEBUG_OTA
            os_printf("OTA header block: %s\r\n", head->c_time);
            #endif

            memcpy(aes_iv, head->aes_iv, AES_KEY_LENGTH);
            aes_enabled = true;

            retries = MAX_RETRIES;
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
          }
          else
          {
	    AnkiWarn( 171, "UpgradeController", 488, "Unhandled special block 0x%x", 1, fwb->blockAddress);
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
          }
        }
        else if (fwb->blockAddress & ESPRESSIF_BLOCK) // Destined for the Espressif flash
        {
          const uint32 destAddr = fwWriteAddress + (fwb->blockAddress & ESP_FW_ADDR_MASK);
          #if DEBUG_OTA
          os_printf("WF 0x%x\r\n", destAddr);
          #endif
          const SpiFlashOpResult rslt = spi_flash_write(destAddr, fwb->flashBlock, TRANSMIT_BLOCK_SIZE);
          if (rslt != SPI_FLASH_RESULT_OK)
          {
            if (retries-- <= 0)
            {
	      #if DEBUG_OTA
	      os_printf("\tRan out of retries writing to Espressif flash\r\n");
	      #endif
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
	    phase = OTAT_Flash_Verify;
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
                os_printf("Write RTIP 0x08%x\t", fwb->blockAddress);
                #endif
                counter = system_get_time() + 20000; // 20ms
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
          {
            #if DEBUG_OTA
            os_printf("Done\r\n");
            #endif
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            phase = OTAT_Flash_Verify; // Finished operation
            break;
          }
          case STATE_IDLE:
          {
            if (system_get_time() > counter)
            {
              #if DEBUG_OTA
              os_printf("Timeout\r\n");
              #endif
              phase = OTAT_Flash_Write;
            }
            break;
          }
          default:
          {
            // Just wait for this to clear
            #if DEBUG_OTA > 2
            os_printf("w4r %x\r\n", i2spiGetRtipBootloaderState());
            #endif
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
        SpiFlashOpResult rslt = SPI_FLASH_RESULT_OK;
        if (didEsp)
        {
          uint32_t headerUpdate[2];
          headerUpdate[0] = nextImageNumber; // Image number
          headerUpdate[1] = 0; // Evil
          rslt = spi_flash_write(fwWriteAddress + APP_IMAGE_HEADER_OFFSET + 4, headerUpdate, 8);
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
            #if DEBUG_OTA
            os_printf("Done programming, send command done\r\n");
            #endif
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
        i2spiBootloaderCommandDone();
        break;
      }
      case OTATR_Set_Evil_A:
      {
        uint32_t invalidNumber = 0;
        if (spi_flash_write(APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4, &invalidNumber, 4) == SPI_FLASH_RESULT_OK)
        {
          phase = OTATR_Set_Evil_B;
        }
        break;
      }
      case OTATR_Set_Evil_B:
      {
        uint32_t invalidNumber = 0;
        if (spi_flash_write(APPLICATION_B_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4, &invalidNumber, 4) == SPI_FLASH_RESULT_OK)
        {
          counter = system_get_time() + 1000000; // 1s second delay
          phase = OTATR_Delay;
        }
        break;
      }
      case OTATR_Delay:
      {
        if (system_get_time() > counter) phase = OTATR_Get_Size;
        break;
      }
      case OTATR_Get_Size:
      {
        if (spi_flash_read(FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE, &counter, 4) == SPI_FLASH_RESULT_OK)
        {
          if (counter == 0xFFFFffff || counter < sizeof(FirmwareBlock))
          {
            os_printf("Invalid recovery firmware for RTIP and Body, size %x reported at %x\r\n", counter, FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE);
            phase = OTAT_Ready;
          }
          else
          {
            os_printf("Have %d bytes of recovery firmware to send\r\n", counter);
            bufferSize = AnimationController::SuspendAndGetBuffer(&buffer);
            phase      = OTATR_Read;
            retries    = MAX_RETRIES;
          }
        }
        break;
      }
      case OTATR_Read:
      {
        const uint32 readPos = (FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE) + counter + 4 - sizeof(FirmwareBlock);
        const SpiFlashOpResult rslt = spi_flash_read(readPos, reinterpret_cast<uint32*>(buffer), sizeof(FirmwareBlock));
        if (rslt != SPI_FLASH_RESULT_OK)
        {
          os_printf("Trouble reading recovery firmware from %x, %d\r\n", readPos, rslt);
        }
        else
        {
          counter -= sizeof(FirmwareBlock);
          retries = MAX_RETRIES;
          phase = OTATR_Write;
        }
        break;
      }
      case OTATR_Write:
      {
        switch (i2spiGetRtipBootloaderState())
        {
          case STATE_NACK:
          case STATE_ACK:
          case STATE_IDLE:
          {
            FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);
            if (i2spiBootloaderPushChunk(fwb))
            {
              #if DEBUG_OTA
              os_printf("Write RTIP 0x%08x\t", fwb->blockAddress);
              #endif
              phase = OTATR_Wait;
            }
            break;
          }
          case STATE_BUSY:
          {
            break;
          }
          default:
          {
            #if DEBUG_OTA
            os_printf("OTATR WFR %x\r\n", i2spiGetRtipBootloaderState());
            #endif
            break;
          }
        }
        break;
      }
      case OTATR_Wait:
      {
        switch(i2spiGetRtipBootloaderState())
        {
          case STATE_NACK:
          {
            os_printf("RTIP NACK!\r\n");
            phase = OTATR_Write; // try again
            break;
          }
          case STATE_ACK:
          {
            #if DEBUG_OTA
            os_printf("Done\r\n");
            #endif
            if (counter > 0)
            { // Have more to write
              phase = OTATR_Read; // Finished operation
            }
            else
            { // Just wrote the last one
              #if DEBUG_OTA
              os_printf("Done programming, send command done\r\n");
              #endif
              phase = OTATR_Apply;
            }
            break;
          }
          case STATE_BUSY:
          case STATE_IDLE:
          {
            break;
          }
          default:
          {
            //os_printf("OW %d\r\n", i2spiGetRtipBootloaderState());
            break;
          }
        }
        break;
      }
      case OTATR_Apply:
      {
        switch (i2spiGetRtipBootloaderState())
        {
          case STATE_ACK:
          case STATE_IDLE:
            i2spiBootloaderCommandDone();
            break;
          case STATE_NACK:
            os_printf("NACK, attempting again\r\n");
            phase = OTATR_Get_Size;
            break;
          case STATE_BUSY:
            break;
          default:
            os_printf("%04x\r\n", i2spiGetRtipBootloaderState());
        }
        break;
      }
      default:
      {
        AnkiAssert(false, 465);
      }
    }
  }

  #if FACTORY_FIRMWARE
  extern "C" bool i2spiRecoveryCallback(uint32 param)
  {
    os_printf("I2SPI Recovery Synchronized\r\n");
    phase = OTATR_Set_Evil_A;
    return false;
  }
  #endif

  void OnDisconnect()
  {
    if (OTAT_Ready < phase && phase < OTATR_Set_Evil_A)
    {
      Reset();
    }
  }

}
}
}
