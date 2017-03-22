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
#include "driver/rtc.h"
#include "sha1.h"
#include "anki/cozmo/robot/espAppImageHeader.h"
#include "driver/crash.h"
}
#include "anki/cozmo/robot/flash_map.h"
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

#include "crc32.h"
#include "sha512.h"
#include "aes.h"
#include "bignum.h"
#include "rsa_pss.h"
#include "publickeys.h"

#define WRITE_DATA_SIZE (1024)

#define MAX_RETRIES 2
#define SHA_CHECK_READ_LENGTH 512

#define CRC32_POLYNOMIAL (0xedb88320L)

#define DEBUG_OTA 0

void gen_random(void* ptr, int length)
{
  os_get_random((unsigned char *)ptr, length);
}

namespace Anki {
namespace Cozmo {
namespace UpgradeController {

  const u32 VERSION_INFO_MAX_LENGTH = 1024;

  typedef enum
  {
    OTAT_Uninitalized = 0,
    OTAT_Ready,
    OTAT_Enter_Recovery,
    OTAT_Delay,
    OTAT_Flash_Erase,
    OTAT_Sync_Recovery,
    OTAT_Flash_Verify,
    OTAT_Flash_Decrypt,
    OTAT_Flash_Write,
    OTAT_Wait,
    OTAT_Sig_Check,
    OTAT_Abort,
    OTAT_Abort_Check,
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

  struct UCState
  {
    u32 fwWriteAddress; ///< Where we will write the next image
    u32 nextImageNumber; ///< Image number for the next upgrade
    u8* buffer; ///< Pointer to RX buffer
    s32 bufferSize; ///< Number of bytes of storage at buffer
    s32 bufferUsed; ///< Number of bytes of storage buffer currently filled
    s32 bytesProcessed; ///< The number of bytes of OTA data that have been processed from the buffer
    s32 counter; ///< A in self.phase counter
    u32 timer; ///< A timeout time
    s16 acceptedPacketNumber; ///< Highest packet number we have accepted
    u16 retries         : 3; //  3  How many times we've retried the operation
    u16 dsCommanded     : 1; //  4  Downstream processors
    u16 dsWritten       : 1; //  5  Downstream processors written (flash no longer valid unless completed)
    u16 didWiFi         : 1; //  6  Have new wifi firmware
    u16 haveTermination : 1; //  7  Have all OTA packets
    u16 haveValidCert   : 1; //  8  Have a valid certificate for the OTA
    u16 receivedIV      : 1; //  9
    u16 disconnectReset : 1; // 10  Resetting due to disconnect
    OTATaskPhase phase; ///< Keeps track of our current task state
  };

  extern "C"
  {
    extern int COZMO_VERSION_ID;
    extern unsigned int COZMO_BUILD_DATE;
  }

  static char COZMO_BUILD_TYPE;

  static sha512_state firmware_digest;
  static uint8_t aes_iv[AES_KEY_LENGTH];
  static UCState self;

  bool Init()
  {
    os_memset(&self, 0, sizeof(UCState));
    self.timer = system_get_time();
    self.acceptedPacketNumber = -1;
    self.retries = MAX_RETRIES;
    sha512_init(firmware_digest);
    
    // No matter which of the three images we're loading, we can get a header here, factory will get header A
    const AppImageHeader* const ourHeader = (const AppImageHeader* const)(FLASH_MEMORY_MAP + APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET);
    
    if (FACTORY_UPGRADE_CONTROLLER) {
      os_printf("Factory Firmware\r\n");
      self.fwWriteAddress = APPLICATION_A_SECTOR * SECTOR_SIZE; // Factory firmware always writes image A
      self.nextImageNumber = ourHeader->imageNumber + 2; // Force image to be higher even if B was higher
    }
    else
    {
      FWImageSelection selectedImage = GetImageSelection();
      self.nextImageNumber = ourHeader->imageNumber + 1;
      switch (selectedImage)
      {
        case FW_IMAGE_A:
        {
          os_printf("Am image A\r\n");
          self.fwWriteAddress = APPLICATION_B_SECTOR * SECTOR_SIZE;
          break;
        }
        case FW_IMAGE_B:
        {
          os_printf("Am image B\r\n");
          self.fwWriteAddress = APPLICATION_A_SECTOR * SECTOR_SIZE;
          break;
        }
        default:
        {
          os_printf("UPC: Unexpected selectedImage key %08x. Not enabling OTA\r\n", selectedImage);
          recordBootError((void*)GetImageSelection, selectedImage);
          self.phase = OTAT_Uninitalized;
          return false;
        }
      }
    }

    // Copy version metadata into RAM for unaligned access required for string search
    uint32_t versionMetaDataBuffer[(VERSION_INFO_MAX_LENGTH/sizeof(uint32_t))+1]; // allow extra room for null term
    os_memcpy(versionMetaDataBuffer, GetVersionInfo(), VERSION_INFO_MAX_LENGTH);
    char* json = reinterpret_cast<char*>(versionMetaDataBuffer);
    json[VERSION_INFO_MAX_LENGTH] = 0; // Force NULL termination
    static const char* VERSION_TAG = "\"version\": ";
    const char* versionStr = os_strstr(json, VERSION_TAG);
    if (versionStr) COZMO_VERSION_ID = atoi(versionStr + os_strlen(VERSION_TAG));
    else COZMO_VERSION_ID = -2;
    static const char* TIME_TAG = "\"time\": ";
    const char* timeStr = os_strstr(json, TIME_TAG);
    if (timeStr) COZMO_BUILD_DATE = atoi(timeStr + os_strlen(TIME_TAG));
    else COZMO_BUILD_DATE = 1;
    static const char* BUILD_TYPE_TAG = "\"build\": \"";
    const char* buildTypeStr = os_strstr(json, BUILD_TYPE_TAG);
    if (buildTypeStr) COZMO_BUILD_TYPE = *(buildTypeStr+os_strlen(BUILD_TYPE_TAG));
    else COZMO_BUILD_TYPE = 'U';

    self.phase = OTAT_Ready;
    return true;
  }
  
  LOCAL void Reset()
  {
    os_printf("UpgradeController Reset:\r\n");
    AnkiDebug( 170, "UpdateController", 466, "Reset()", 0);
    if (self.phase < OTAT_Enter_Recovery)
    {
      os_printf("\tShutdown\r\n");
      system_deep_sleep(0); // Shut down and wait for timeout
    }
    else if (self.dsWritten)
    {
      os_printf("\tSet evil A\r\n");
      self.phase = OTATR_Set_Evil_A;
    }
    else
    {
      os_printf("\tAbort\r\n");
      self.phase = OTAT_Abort;
    }
  }

  LOCAL bool VerifyFirmwareBlock(const FirmwareBlock* fwb)
  {
    return calc_crc32(reinterpret_cast<const uint8*>(fwb->flashBlock), TRANSMIT_BLOCK_SIZE) == fwb->checkSum;
  }

  LOCAL bool FlashWriteOkay(const u32 address, const u32 length)
  {
    FWImageSelection selectedImage = GetImageSelection();
    switch (selectedImage)
    {
      case FW_IMAGE_FACTORY:  //factory allows writing to A.
      case FW_IMAGE_A:
      { // Am image A so writing to B okay
        if ((address >= (APPLICATION_B_SECTOR * SECTOR_SIZE)) && ((address + length) <= (NV_STORAGE_SECTOR * SECTOR_SIZE))) return true;
        else if ((address >= (APPLICATION_A_SECTOR * SECTOR_SIZE + ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE)) && ((address + length) <= (FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE))) return true; // Allow setting firmware note
        else if ((address == (APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4)) && (length == 4)) return true; // Allow invalidating our own image
        else if (selectedImage != FW_IMAGE_FACTORY) {
          break; 
        }
        //FACTORY image falls through, allows writing to B also.
      }
      case FW_IMAGE_B:
      { // Am image B so writing to A okay
        if ((address >= (APPLICATION_A_SECTOR * SECTOR_SIZE)) && ((address + length) <= (FACTORY_WIFI_FW_SECTOR * SECTOR_SIZE))) return true;
        else if ((address >= APPLICATION_B_SECTOR * SECTOR_SIZE + ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE) && ((address + length) <= (NV_STORAGE_SECTOR * SECTOR_SIZE))) return true;
        else if ((address == (APPLICATION_B_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4)) && (length == 4)) return true; // Allow invalidating our own image
        else break;
      }
      default:
        break;
    }
    return false;
  }

  LOCAL SpiFlashOpResult FlashWrite(u32 address, u32* data, u32 length)
  {
    if (FlashWriteOkay(address, length))
    {
      return spi_flash_write(address, data, length);
    }
    else return SPI_FLASH_RESULT_ERR;
  }

  LOCAL SpiFlashOpResult FlashErase(u32 address)
  {
    if ((int)address & SECTOR_MASK) return SPI_FLASH_RESULT_ERR;
    else if (FlashWriteOkay(address, SECTOR_SIZE))
    {
      u16 sector = address / SECTOR_SIZE;
      return spi_flash_erase_sector(sector);
    }
    else return SPI_FLASH_RESULT_ERR;
  }

  void Write(RobotInterface::OTA::Write& msg)
  {
    using namespace RobotInterface::OTA;
    Ack ack;
    ack.bytesProcessed = self.bytesProcessed;
    ack.packetNumber = self.acceptedPacketNumber;
    #if DEBUG_OTA > 1
    os_printf("UpC: Write(%d, %x...)\r\n", msg.packetNumber, msg.data[0]);
    #endif
    switch (self.phase)
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
          //Face::FacePrintf("Starting FOTA\nupgrade...");
          #endif
          self.bufferSize = AnimationController::SuspendAndGetBuffer(&self.buffer);
          self.timer = system_get_time() + 20000; // 20 ms
          self.phase = OTAT_Enter_Recovery;
          RTIP::SendMessage(NULL, 0, RobotInterface::EngineToRobot::Tag_bodyEnterOTA);
          // Explicit fallthrough to next case
        }
      }
      case OTAT_Enter_Recovery:
      case OTAT_Delay:
      case OTAT_Flash_Erase:
      case OTAT_Sync_Recovery:
      case OTAT_Flash_Verify:
      case OTAT_Flash_Decrypt:
      case OTAT_Flash_Write:
      case OTAT_Wait:
      case OTAT_Sig_Check:
      {
        if ((self.bufferSize - self.bufferUsed) < WRITE_DATA_SIZE)
        {
          #if DEBUG_OTA
          os_printf("\tNo mem p=%d s=%d u=%d\r\n", self.bytesProcessed, self.bufferSize, self.bufferUsed);
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
          self.haveTermination = 1;
        }
        else if (msg.packetNumber != (self.acceptedPacketNumber + 1))
        {
          #if DEBUG_OTA
          os_printf("\tOOO\r\n");
          #endif
          ack.result = OUT_OF_ORDER;
          RobotInterface::SendMessage(ack);
        }
        else
        {
          os_memcpy(self.buffer + self.bufferUsed, msg.data, WRITE_DATA_SIZE);
          self.bufferUsed += WRITE_DATA_SIZE;
          self.acceptedPacketNumber = msg.packetNumber;
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
    ack.bytesProcessed = self.bytesProcessed;
    ack.packetNumber   = self.acceptedPacketNumber;

    if ((clientConnected() == false) && (OTAT_Ready < self.phase && self.phase < OTATR_Set_Evil_A) && (self.disconnectReset == 0))
    {
      #if DEBUG_OTA > 0
      os_printf("OTA Disconnected in self.phase %d\r\n", self.phase);
      #endif
      self.disconnectReset = 1;
      Reset();
    }

    #if DEBUG_OTA > 1
    static OTATaskPhase prevPhase;
    if (self.phase != prevPhase)
    {
      os_printf("P %d->%d\r\n", prevPhase, self.phase);
      prevPhase = self.phase;
    }
    #endif

    switch (self.phase)
    {
      case OTAT_Uninitalized:
      case OTAT_Ready:
      {
        break;
      }
      case OTAT_Enter_Recovery:
      {
        if ((system_get_time() > self.timer) && (Face::GetRemainingRects() == 0))
        {
          const int8_t mode = OTA_Mode;
          if (RTIP::SendMessage((const uint8_t*)&mode, 1, RobotInterface::EngineToRobot::Tag_enterRecoveryMode))
          {
            self.timer = system_get_time() + 30000;
            self.phase = OTAT_Delay;
          }
        }
        break;
      }
      case OTAT_Delay:
      {
        if ((system_get_time() > self.timer) && i2spiMessageQueueIsEmpty())
        {
          #if DEBUG_OTA
          os_printf("I2SPI_SYNC\r\n");
          #endif
          i2spiSwitchMode(I2SPI_SYNC);
          self.phase = OTAT_Flash_Erase;
          self.retries = MAX_RETRIES;
          self.counter = 0;
          self.timer = 0;
          AnkiDebug( 172, "UpgradeController.state", 468, "flash erase", 0);
        }
        break;
      }
      case OTAT_Flash_Erase:
      {
        if (self.counter < ESP_FW_MAX_SIZE)
        {
          #if DEBUG_OTA
          os_printf("Erase sector 0x%x\r\n", self.fwWriteAddress + self.counter);
          #endif
          const SpiFlashOpResult rslt = FlashErase(self.fwWriteAddress + self.counter);
          if (rslt != SPI_FLASH_RESULT_OK)
          {
            if (self.retries-- == 0)
            {
              ack.result = rslt;
              RobotInterface::SendMessage(ack);
              Reset();
            }
          }
          else
          {
            self.retries = MAX_RETRIES;
            self.counter += SECTOR_SIZE;
          }
        }
        else // Done with erase, advance state
        {
          AnkiDebug( 172, "UpgradeController.state", 469, "sync recovery", 0);
          self.phase = OTAT_Sync_Recovery;
          self.timer = system_get_time() + 5000000; // 5 second timeout
          self.retries = MAX_RETRIES;
        }
        break;
      }
      case OTAT_Sync_Recovery:
      {
        if (i2spiGetRtipBootloaderState() == STATE_IDLE)
        {
          AnkiDebug( 172, "UpgradeController.state", 470, "Flash Verify", 0);
          self.phase = OTAT_Flash_Verify;
          self.counter = 0;
          self.timer = 0;
          self.retries = MAX_RETRIES;
          ack.result = OKAY;
          RobotInterface::SendMessage(ack);
        }
        else if (system_get_time() > self.timer)
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
        if (self.bufferUsed < (int)sizeof(FirmwareBlock)) // We've reached the end of the buffer
        {
          if (self.haveTermination) // If there's no more, advance now
          {
            if (self.haveValidCert)
            {
              #if DEBUG_OTA
              os_printf("Valid certificate, applying\r\n");
              #endif
              AnkiDebug( 172, "UpgradeController.state", 457, "Apply Wifi", 0);
              self.counter = 0;
              self.timer = 0;
              self.retries = MAX_RETRIES;
              self.phase = OTAT_Apply_WiFi;
            }
            else
            {
              #if DEBUG_OTA
              os_printf("No valid certificate, resetting\r\n");
              #endif
              AnkiError( 188, "UpgradeController.termination", 489, "No valid certificate!", 0);
              Reset();
            }
          }
        }
        else
        {
          FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(self.buffer);

          if (VerifyFirmwareBlock(fwb) == false)
          {
            #if DEBUG_OTA
            os_printf("Bad block CRC at %d\r\n", self.bytesProcessed);
            #endif
            ack.result = ERR_BAD_DATA;
            RobotInterface::SendMessage(ack);
            Reset();
          }
          else if (fwb->blockAddress != CERTIFICATE_BLOCK)
          {
            static const int sha_per_tick = SHA512_BLOCK_SIZE;
            self.haveValidCert = 0;

            const int remaining = sizeof(FirmwareBlock) - self.counter;

            if (remaining > sha_per_tick)
            {
              sha512_process(firmware_digest, self.buffer + self.counter, sha_per_tick);
              self.counter += sha_per_tick;
            }
            else
            {
              bool encrypted = (fwb->blockAddress & SPECIAL_BLOCK) != SPECIAL_BLOCK;
              sha512_process(firmware_digest, self.buffer + self.counter, remaining);

              if (encrypted && !self.receivedIV) {
                // We did not receive out IV, so this image is invalid
                AnkiError( 199, "UpgradeController.encryption.noIV", 507, "no IV!", 0);
                Reset();
              } else {
                self.retries = MAX_RETRIES;
                self.counter = 0;
                self.timer = 0;
                if (encrypted)
                {
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 508, "Flash Decrypt", 0);
                  os_printf("Flash decrypt\r\n");
                  #endif
                  self.phase = OTAT_Flash_Decrypt;
                }
                else
                {
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
                  os_printf("Flash write\r\n");
                  #endif
                  self.phase = OTAT_Flash_Write;
                }
              }
            }
          }
          else
          {
            AnkiDebug( 172, "UpgradeController.state", 490, "Sig Check", 0);
            #if DEBUG_OTA
            os_printf("OTA Sig header\r\n");
            #endif
            self.retries = MAX_RETRIES;
            self.counter = 0;
            self.timer = 0;
            self.phase = OTAT_Sig_Check;
          }
        }
        break;
      }
      case OTAT_Flash_Decrypt:
      {
        static const int AES_CHUNK_SIZE = TRANSMIT_BLOCK_SIZE / 8;
        FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(self.buffer);

        if ((unsigned)self.counter < TRANSMIT_BLOCK_SIZE)
        {
          uint8_t* address = self.counter + (uint8_t*)fwb->flashBlock;
          aes_cfb_decode( AES_KEY, aes_iv, address, address, AES_CHUNK_SIZE, aes_iv);
          self.counter += AES_CHUNK_SIZE;
        }
        else
        {
          // Correct the checksum and send it down the wire
          fwb->checkSum = calc_crc32(reinterpret_cast<const uint8*>(fwb->flashBlock), sizeof(fwb->flashBlock));
          #if DEBUG_OTA > 1
          AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
          os_printf("Flash write\r\n");
          #endif
          self.phase = OTAT_Flash_Write;
          self.counter = 0;
          self.timer = 0;
        }

        break ;
      }
      case OTAT_Flash_Write:
      {
        FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(self.buffer);

        if ((fwb->blockAddress & SPECIAL_BLOCK) == SPECIAL_BLOCK)
        {
          if (fwb->blockAddress == COMMENT_BLOCK)
          {
            #if DEBUG_OTA
            os_printf("Comment block\r\n");
            #endif
            self.retries = MAX_RETRIES;
            self.bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
            self.bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = self.bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            self.phase = OTAT_Flash_Verify;
            self.counter = 0;
            self.timer = 0;
          }
          else if (fwb->blockAddress == HEADER_BLOCK)
          {
            FirmwareHeaderBlock* head = reinterpret_cast<FirmwareHeaderBlock*>(fwb->flashBlock);
            
            #if DEBUG_OTA
            os_printf("OTA header block: %s\r\n", head->c_time);
            #endif

            const int max_model = 
              (head->fileVersion >= CURRENT_FILE_VERSION) ? head->max_model : PRODUCTION_V1;

            if ((getModelNumber() & 0xFF) > max_model) {
              #if DEBUG_OTA
              os_printf("\tInvalid model version detected\r\n");
              #endif

              ack.bytesProcessed = self.bytesProcessed;
              ack.result = ERR_INCOMPATIBLE;
              RobotInterface::SendMessage(ack);
  
              self.phase = OTAT_Flash_Verify;
              self.counter = 0;
              self.timer = 0;

              return ;
            }

            self.receivedIV = 1;
            memcpy(aes_iv, head->aes_iv, AES_KEY_LENGTH);

            self.retries = MAX_RETRIES;
            self.bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
            self.bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = self.bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);

            self.phase = OTAT_Flash_Verify;
            self.counter = 0;
            self.timer = 0;
          }
          else
          {
            AnkiWarn( 171, "UpgradeController", 491, "Unhandled special block 0x%x", 1, fwb->blockAddress);
            self.bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
            self.bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = self.bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);

            self.phase = OTAT_Flash_Verify;
            self.counter = 0;
            self.timer = 0;
          }
        }
        else
        {
          if ((fwb->blockAddress & (TRANSMIT_BLOCK_SIZE-1)) != 0)
          {
            #if DEBUG_OTA
            os_printf("\tBlock write address not on proper boundary\r\n");
            #endif
            ack.result = ERR_BAD_ADDRESS;
            RobotInterface::SendMessage(ack);
            Reset();
          }
          else if (fwb->blockAddress & ESPRESSIF_BLOCK) // Destined for the Espressif flash
          {
            const uint32 fwOffset = fwb->blockAddress & ESP_FW_ADDR_MASK;
            const uint32 destAddr = self.fwWriteAddress + fwOffset;
            #if DEBUG_OTA
            os_printf("WF 0x%x\r\n", destAddr);
            #endif
            if (fwOffset == 0) // If this is the block that contains the app image header
            {
              // If the evil and image number words aren't 0xFFFFffff, someone is trying something nasty
              if ((fwb->flashBlock[(APP_IMAGE_HEADER_OFFSET/sizeof(uint32_t)) + 1] != 0xFFFFffff) ||
                  (fwb->flashBlock[(APP_IMAGE_HEADER_OFFSET/sizeof(uint32_t)) + 2] != 0xFFFFffff))
              {
                #if DEBUG_OTA
                os_printf("\tInvalid evil!\r\n");
                #endif
                ack.result = ERR_BAD_DATA;
                RobotInterface::SendMessage(ack);
                Reset();
                return;
              }
            }
            else if (self.didWiFi == 0)
            {
              #if DEBUG_OTA
              os_printf("\tRejecting WiFi firmware out of order!\r\n");
              #endif
              ack.result = OUT_OF_ORDER;
              RobotInterface::SendMessage(ack);
              Reset();
              return;
            }
            const SpiFlashOpResult rslt = FlashWrite(destAddr, fwb->flashBlock, TRANSMIT_BLOCK_SIZE);
            if (rslt != SPI_FLASH_RESULT_OK)
            {
              if (self.retries-- == 0)
              {
                #if DEBUG_OTA
                os_printf("\tRan out of retries writing to Espressif flash\r\n");
                #endif
                ack.result = rslt;
                RobotInterface::SendMessage(ack);
                Reset();
              }
            }
            else
            {
              self.didWiFi = 1;
              self.retries = MAX_RETRIES;
              self.bufferUsed -= sizeof(FirmwareBlock);
              os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
              self.bytesProcessed += sizeof(FirmwareBlock);
              ack.bytesProcessed = self.bytesProcessed;
              ack.result = OKAY;
              RobotInterface::SendMessage(ack);
              #if DEBUG_OTA > 1
              AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
              #endif
              self.phase = OTAT_Flash_Verify;
              self.counter = 0;
              self.timer = 0;
            }
          }
          else // This is bound for the RTIP or body
          {
            if ((self.dsCommanded == 0) && (fwb->blockAddress != FIRST_BODY_BLOCK))
            {
              #if DEBUG_OTA
              os_printf("\tRejecting RTIP/Body firmware out of order\r\n");
              #endif
              ack.result = OUT_OF_ORDER;
              RobotInterface::SendMessage(ack);
              Reset();
              return;
            }
            
            switch (i2spiGetRtipBootloaderState())
            {
              case STATE_NACK:
              case STATE_ACK:
              case STATE_IDLE:
              {
                if (i2spiBootloaderPushChunk(fwb))
                {
                  self.dsCommanded = 1;
                  #if DEBUG_OTA
                  os_printf("Write RTIP 0x%08x\t", fwb->blockAddress);
                  #endif
                  self.timer = system_get_time() + 20000; // 20ms
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 499, "Flash wait", 0);
                  #endif
                  self.phase = OTAT_Wait;
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
        break;
      }
      case OTAT_Wait:
      {
        switch(i2spiGetRtipBootloaderState())
        {
          case STATE_NACK:
          {
            os_printf("RTIP NACK!\r\n");
            if (self.retries-- == 0)
            {
              ack.result = ERR_I2SPI;
              RobotInterface::SendMessage(ack);
              Reset();
              break;
            }
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
            #endif
            self.phase = OTAT_Flash_Write; // try again
            break;
          }
          case STATE_ACK:
          {
            #if DEBUG_OTA
            os_printf("Done\r\n");
            #endif
            self.bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
            self.bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = self.bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
            #endif
            self.phase = OTAT_Flash_Verify; // Finished operation
            self.dsWritten = 1;
            self.counter = 0;
            self.timer = 0;
            break;
          }
          case STATE_IDLE:
          {
            if (system_get_time() > self.timer)
            {
              #if DEBUG_OTA
              os_printf("Timeout\r\n");
              #endif
              self.phase = OTAT_Flash_Write;
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
        cert_state_t* cert_state = (cert_state_t*)os_zalloc(sizeof(cert_state_t));
        if (cert_state == NULL)
        {
          if (self.retries-- == 0)
          {
            #if DEBUG_OTA
            os_printf("Couldn't allocate memory (%d bytes) for cert_state\r\n", sizeof(cert_state_t));
            #endif
            ack.result = ERR_NO_MEM;
            RobotInterface::SendMessage(ack);
            Reset();
          }
          break;
        }
        else
        {
          FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(self.buffer);
          CertificateData* cert = reinterpret_cast<CertificateData*>(fwb->flashBlock);
          
          os_memcpy(&(cert_state->mont), &RSA_CERT_MONT, sizeof(RSA_CERT_MONT));
          os_memcpy(&(cert_state->rsa),  &CERT_RSA,      sizeof(CERT_RSA));
          
          sha512_done(firmware_digest, cert_state->checksum);
          sha512_init(firmware_digest);
          if (verify_cert(cert_state, cert->data, cert->length) == false)
          {
            #if DEBUG_OTA
            os_printf("Invalid Cert\r\n");
            #endif
            Reset();
          }
          else
          {
            os_free(cert_state);
            self.haveValidCert = 1;
            self.retries = MAX_RETRIES*MAX_RETRIES; // More retries just after sig check
            self.bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(self.buffer, self.buffer + sizeof(FirmwareBlock), self.bufferUsed);
            self.bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = self.bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            self.counter = 0;
            self.phase = OTAT_Flash_Verify;
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
            os_printf("Flash verify\r\n");
            #endif
          }
          break;
        }
        break ;
      }
      case OTAT_Abort:
      {
        if (i2spiBootloaderCommandDone())
        {
          self.phase = OTAT_Abort_Check;
        }
        break;
      }
      case OTAT_Abort_Check:
      {
        s16 status = i2spiGetRtipBootloaderState();
        if (status == STATE_NACK)
        {
          os_printf("Abort done nacked, resetting to factory\r\n");
          self.phase = OTATR_Set_Evil_A;
        }
        break;
      }
      case OTAT_Apply_WiFi:
      {
        SpiFlashOpResult rslt = SPI_FLASH_RESULT_OK;
        if (self.didWiFi)
        {
          uint32_t headerUpdate[2];
          headerUpdate[0] = self.nextImageNumber; // Image number
          headerUpdate[1] = 0; // Evil
          rslt = FlashWrite(self.fwWriteAddress + APP_IMAGE_HEADER_OFFSET + 4, headerUpdate, 8);
        }
        if (rslt != SPI_FLASH_RESULT_OK)
        {
          if (self.retries-- == 0)
          {
            ack.result = rslt;
            RobotInterface::SendMessage(ack);
            Reset();
          }
        }
        else
        {
          AnkiDebug( 172, "UpgradeController.state", 463, "Reboot", 0);
          self.phase   = OTAT_Reboot;
          //self.phase   = OTAT_Apply_RTIP;
          //self.timer = system_get_time() + 10000000; // 10 second timeout
          self.retries = MAX_RETRIES;
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
            self.phase = OTAT_Wait_For_RTIP_Ack;
            self.timer = system_get_time() + 2000000; // 2 second timeout
          }
        }
        else if (system_get_time() > self.timer)
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
            if (system_get_time() > self.timer)
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
            self.phase = OTAT_Reboot;
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
        self.phase = OTAT_Wait_For_Reboot;
        #if DEBUG_OTA
        os_printf("OTAT Wait for Reboot\r\n");
        #endif
        self.timer = system_get_time() + 100000; // delay for messages to clear
        break;
      }
      case OTAT_Wait_For_Reboot:
      {
        if (i2spiGetRtipBootloaderState() == STATE_NACK)
        {
          self.phase = OTATR_Set_Evil_A;
        }
        else if (system_get_time() > self.timer)
        {
          system_deep_sleep(0);
        }
        else
        {
          i2spiBootloaderCommandDone();
        }
        break;
      }
      case OTATR_Set_Evil_A:
      {
        uint32_t invalidNumber = 0;
        if (FlashWrite(APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4, &invalidNumber, 4) == SPI_FLASH_RESULT_OK)
        {
          self.phase = OTATR_Set_Evil_B;
        }
        break;
      }
      case OTATR_Set_Evil_B:
      {
        uint32_t invalidNumber = 0;
        if (FlashWrite(APPLICATION_B_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET + 4, &invalidNumber, 4) == SPI_FLASH_RESULT_OK)
        {
          self.timer = system_get_time() + 1000000; // 1s second delay
          self.phase = OTATR_Delay;
        }
        break;
      }
      case OTATR_Delay:
      {
        if (system_get_time() > self.timer) self.phase = OTATR_Get_Size;
        break;
      }
      case OTATR_Get_Size:
      {
        self.counter = 0;
        if (spi_flash_read(FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE, reinterpret_cast<uint32_t*>(&self.counter), 4) == SPI_FLASH_RESULT_OK)
        {
          if ((self.counter < (int)sizeof(FirmwareBlock)) || (self.counter > 0x20000))
          {
            os_printf("Invalid recovery firmware for RTIP and Body, size %x reported at %x\r\n", self.counter, FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE);
            system_deep_sleep(0); // Just die, nothing we can do
          }
          else
          {
            os_printf("Have %d bytes of recovery firmware to send\r\n", self.counter);
            self.bufferSize = AnimationController::SuspendAndGetBuffer(&self.buffer);
            self.phase      = OTATR_Read;
            self.retries    = MAX_RETRIES;
          }
        }
        break;
      }
      case OTATR_Read:
      {
        const uint32 readPos = (FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE) + self.counter + 4 - sizeof(FirmwareBlock);
        const SpiFlashOpResult rslt = spi_flash_read(readPos, reinterpret_cast<uint32*>(self.buffer), sizeof(FirmwareBlock));
        if (rslt != SPI_FLASH_RESULT_OK)
        {
          os_printf("Trouble reading recovery firmware from %x, %d\r\n", readPos, rslt);
        }
        else
        {
          self.counter -= sizeof(FirmwareBlock);
          self.retries = MAX_RETRIES;
          self.phase = OTATR_Write;
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
            FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(self.buffer);
            if (i2spiBootloaderPushChunk(fwb))
            {
              #if DEBUG_OTA
              os_printf("Write RTIP 0x%08x\t", fwb->blockAddress);
              #endif
              self.phase = OTATR_Wait;
              self.timer = system_get_time() + 200000; // 200ms
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
            self.phase = OTATR_Write; // try again
            break;
          }
          case STATE_IDLE:
          {
            if (system_get_time() > self.timer)
            {
              #if DEBUG_OTA
              os_printf("Timeout\t");
              #endif
              self.phase = OTATR_Read;
              // Fall through to ack case
            }
            else
            {
              break;
            }
          }
          case STATE_ACK:
          {
            #if DEBUG_OTA
            os_printf("Done\r\n");
            #endif
            if (self.counter > 0)
            { // Have more to write
              self.phase = OTATR_Read; // Finished operation
            }
            else
            { // Just wrote the last one
              #if DEBUG_OTA
              os_printf("Done programming, send command done\r\n");
              #endif
              self.phase = OTATR_Apply;
            }
            break;
          }
          case STATE_BUSY:
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
            self.phase = OTATR_Get_Size;
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
  
  extern "C" bool i2spiRecoveryCallback(uint32 param)
  {
    if (self.phase < OTAT_Delay)
    {
      os_printf("I2SPI Recovery Synchronized\r\n");
      self.phase = OTATR_Set_Evil_A;
    }
    return false;
  }

  const u32* GetVersionInfo()
  {
    #if FACTORY_USE_STATIC_VERSION_DATA
    static const char FACTORY_STATIC_VERSION_DATA[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"build\": \"FACTORY\", \"version\": \"F1.5.2\", \"date\": \"Wed Mar 22 13:13:37 2017\", \"time\": 1490213637}\0\0\0\0\0\0\0\0"; //< Ensure at u32 null padding
    return reinterpret_cast<const u32*>(FACTORY_STATIC_VERSION_DATA);
    #else
    const uint32_t VERSION_INFO_ADDR = (APPLICATION_A_SECTOR * SECTOR_SIZE) + ESP_FW_MAX_SIZE - 0x800; // Memory offset of version info for both apps
    return FLASH_CACHE_POINTER + (VERSION_INFO_ADDR/4);  // Use A address because both images see it mapped in that place.
    #endif
  }

  char GetBuildType()
  {
    return COZMO_BUILD_TYPE;
  }

  /// Retrieve numerical firmware version
  extern "C" s32 GetFirmwareVersion() { return COZMO_VERSION_ID; }
  
  /// Retrieve the numerical (epoch) build timestamp
  extern "C" u32 GetBuildTime() { return COZMO_BUILD_DATE; }
  
  extern "C" bool SetFirmwareNote(const u32 offset, u32 note)
  {
    if (offset >= (ESP_FW_NOTE_SIZE/sizeof(u32)))
    {
      os_printf("SetFirmwareNote: Offset %x out of range\r\n", offset);
      return false;
    }
    else if (GetFirmwareNote(offset) != 0xFFFFFFFF)
    {
      //os_printf("SetFirmwareNote: Note already written\r\n");
      return false;
    }
    else
    {
      u32 noteAddr = ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE + (offset * sizeof(u32));
      switch (GetImageSelection())
      {
         case FW_IMAGE_A:
        {
          noteAddr += APPLICATION_A_SECTOR * SECTOR_SIZE;
          break;
        }
        case FW_IMAGE_B:
        {
          noteAddr += APPLICATION_B_SECTOR * SECTOR_SIZE;
          break;
        }
        default:
        {
          os_printf("SetFirmwareNote: Couldn't determine where to store note!\r\n");
          return false;
        }
      }
      
      return FlashWrite(noteAddr, &note, sizeof(u32)) == SPI_FLASH_RESULT_OK;
    }
  }
  
  u32 GetFirmwareNote(const u32 offset)
  {
    if (offset >= (ESP_FW_NOTE_SIZE/sizeof(u32)))
    {
      os_printf("GetFirmwareNote: Offset %x out of range\r\n", offset);
      return 0xFFFFFFFF;
    }
    else
    {
      const uint32_t NOTE_BASE_ATTR = (APPLICATION_A_SECTOR * SECTOR_SIZE) + ESP_FW_MAX_SIZE - ESP_FW_NOTE_SIZE; // Memory offset of notes info for both apps
      return *(FLASH_CACHE_POINTER + (NOTE_BASE_ATTR/4) + offset);
    }
  }

}
}
}
