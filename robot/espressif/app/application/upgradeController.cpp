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

#include "crc32.h"
#include "sha512.h"
#include "aes.h"
#include "bignum.h"
#include "rsa_pss.h"
#include "publickeys.h"

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
  int32_t  counter; ///< A in phase counter
  uint32_t timer; ///< A timeout time
  int16_t  acceptedPacketNumber; ///< Highest packet number we have accepted
  int8_t   retries; ///< Flash operation retries
  OTATaskPhase phase; ///< Keeps track of our current task state
  bool didWiFi; ///< We have new firmware for the Espressif
  bool didRTIP; ///< We have written new firmare for the RTIP
  bool haveTermination; ///< Have received termination
  bool haveValidCert; ///< Have a valid certificate for the image
  bool receivedIV;

  static sha512_state firmware_digest;
  static uint8_t aes_iv[AES_KEY_LENGTH];

  bool Init()
  {
    buffer = NULL;
    bufferSize = 0;
    bufferUsed = 0;
    bytesProcessed = 0;
    counter = 0;
    timer = system_get_time();
    acceptedPacketNumber = -1;
    retries = MAX_RETRIES;
    didWiFi = false;
    didRTIP = false;
    haveTermination = false;
    haveValidCert = false;
    receivedIV = false;
    sha512_init(firmware_digest);
    
    // No matter which of the three images we're loading, we can get a header here
    const AppImageHeader* const ourHeader = (const AppImageHeader* const)(FLASH_MEMORY_MAP + APPLICATION_A_SECTOR * SECTOR_SIZE + APP_IMAGE_HEADER_OFFSET);
    

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
    }
    else if (didRTIP)
    {
      phase = OTATR_Set_Evil_A;
    }
    else
    {
      phase = OTAT_Abort;
    }
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
          //Face::FacePrintf("Starting FOTA\nupgrade...");
          #endif
          bufferSize = AnimationController::SuspendAndGetBuffer(&buffer);
          timer = system_get_time() + 20000; // 20 ms
          phase = OTAT_Enter_Recovery;
          RTIP::SendMessage(NULL, 0, RobotInterface::EngineToRobot::Tag_bodyRestart);
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
        if ((system_get_time() > timer) && (Face::GetRemainingRects() == 0))
        {
          const int8_t mode = OTA_Mode;
          if (RTIP::SendMessage((const uint8_t*)&mode, 1, RobotInterface::EngineToRobot::Tag_enterRecoveryMode))
          {
            timer = system_get_time() + 30000;
            phase = OTAT_Delay;
          }
        }
        break;
      }
      case OTAT_Delay:
      {
        if ((system_get_time() > timer) && i2spiMessageQueueIsEmpty())
        {
          i2spiSwitchMode(I2SPI_PAUSED);
          phase = OTAT_Flash_Erase;
          retries = MAX_RETRIES;
          counter = 0;
          timer = 0;
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
          timer = system_get_time() + 5000000; // 5 second timeout
          retries = MAX_RETRIES;
        }
        break;
      }
      case OTAT_Sync_Recovery:
      {
        if (i2spiGetRtipBootloaderState() == STATE_IDLE)
        {
          AnkiDebug( 172, "UpgradeController.state", 470, "Flash Verify", 0);
          phase = OTAT_Flash_Verify;
          counter = 0;
          timer = 0;
          retries = MAX_RETRIES;
          ack.result = OKAY;
          RobotInterface::SendMessage(ack);
        }
        else if (system_get_time() > timer)
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
            if (haveValidCert)
            {
              #if DEBUG_OTA
              os_printf("Valid certificate, applying\r\n");
              #endif
              AnkiDebug( 172, "UpgradeController.state", 457, "Apply Wifi", 0);
              counter = 0;
              timer = 0;
              retries = MAX_RETRIES;
              phase = OTAT_Apply_WiFi;
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
            static const int sha_per_tick = SHA512_BLOCK_SIZE;
            haveValidCert = false;

            const int remaining = sizeof(FirmwareBlock) - counter;

            if (remaining > sha_per_tick)
            {
              sha512_process(firmware_digest, buffer + counter, sha_per_tick);
              counter += sha_per_tick;
            }
            else
            {
              bool encrypted = (fwb->blockAddress & SPECIAL_BLOCK) != SPECIAL_BLOCK;
              sha512_process(firmware_digest, buffer + counter, remaining);

              if (encrypted && !receivedIV) {
                // We did not receive out IV, so this image is invalid
                AnkiError( 199, "UpgradeController.encryption.noIV", 507, "no IV!", 0);
                Reset();
              } else {
                retries = MAX_RETRIES;
                counter = 0;
                timer = 0;
                if (encrypted)
                {
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 508, "Flash Decrypt", 0);
                  os_printf("Flash decrypt\r\n");
                  #endif
                  phase = OTAT_Flash_Decrypt;
                }
                else
                {
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
                  os_printf("Flash write\r\n");
                  #endif
                  phase = OTAT_Flash_Write;
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

            counter = 0;
            timer = 0;
            phase = OTAT_Sig_Check;
          }
        }
        break;
      }
      case OTAT_Flash_Decrypt:
      {
        static const int AES_CHUNK_SIZE = TRANSMIT_BLOCK_SIZE / 8;
        FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);

        if ((unsigned)counter < TRANSMIT_BLOCK_SIZE)
        {
          uint8_t* address = counter + (uint8_t*)fwb->flashBlock;
          aes_cfb_decode( AES_KEY, aes_iv, address, address, AES_CHUNK_SIZE, aes_iv);
          
          counter += AES_CHUNK_SIZE;
        }
        else
        {
          // Correct the checksum and send it down the wire
          fwb->checkSum = calc_crc32(reinterpret_cast<const uint8*>(fwb->flashBlock), sizeof(fwb->flashBlock));
          #if DEBUG_OTA > 1
          AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
          os_printf("Flash write\r\n");
          #endif
          phase = OTAT_Flash_Write;
          counter = 0;
          timer = 0;
        }

        break ;
      }
      case OTAT_Flash_Write:
      {
        FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);

        if ((fwb->blockAddress & SPECIAL_BLOCK) == SPECIAL_BLOCK)
        {
          if (fwb->blockAddress == COMMENT_BLOCK)
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
            
            phase = OTAT_Flash_Verify;
            counter = 0;
            timer = 0;
          }
          else if (fwb->blockAddress == HEADER_BLOCK)
          {
            FirmwareHeaderBlock* head = reinterpret_cast<FirmwareHeaderBlock*>(fwb->flashBlock);
            
            #if DEBUG_OTA
            os_printf("OTA header block: %s\r\n", head->c_time);
            #endif

            receivedIV = true;
            memcpy(aes_iv, head->aes_iv, AES_KEY_LENGTH);

            retries = MAX_RETRIES;
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);

            phase = OTAT_Flash_Verify;
            counter = 0;
            timer = 0;
          }
          else
          {
            AnkiWarn( 171, "UpgradeController", 491, "Unhandled special block 0x%x", 1, fwb->blockAddress);
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);

            phase = OTAT_Flash_Verify;
            counter = 0;
            timer = 0;
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
            const uint32 destAddr = fwWriteAddress + fwOffset;
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
            else if (didWiFi == false)
            {
              #if DEBUG_OTA
              os_printf("\tRejecting WiFi firmware out of order!\r\n");
              #endif
              ack.result = OUT_OF_ORDER;
              RobotInterface::SendMessage(ack);
              Reset();
              return;
            }
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
              didWiFi = true;
              retries = MAX_RETRIES;
              bufferUsed -= sizeof(FirmwareBlock);
              os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
              bytesProcessed += sizeof(FirmwareBlock);
              ack.bytesProcessed = bytesProcessed;
              ack.result = OKAY;
              RobotInterface::SendMessage(ack);
              #if DEBUG_OTA > 1
              AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
              #endif
              phase = OTAT_Flash_Verify;
              counter = 0;
              timer = 0;
            }
          }
          else // This is bound for the RTIP or body
          {
            if ((didRTIP == false) && (fwb->blockAddress != FIRST_BODY_BLOCK))
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
                  didRTIP = true;
                  #if DEBUG_OTA
                  os_printf("Write RTIP 0x08%x\t", fwb->blockAddress);
                  #endif
                  timer = system_get_time() + 20000; // 20ms
                  #if DEBUG_OTA > 1
                  AnkiDebug( 172, "UpgradeController.state", 499, "Flash wait", 0);
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
              break;
            }
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 509, "Flash Write", 0);
            #endif
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
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
            #endif
            phase = OTAT_Flash_Verify; // Finished operation
            counter = 0;
            timer = 0;
            break;
          }
          case STATE_IDLE:
          {
            if (system_get_time() > timer)
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
        cert_state_t* cert_state = (cert_state_t*) Anki::Cozmo::Face::m_frame;
        #if DEBUG_OTA
        os_printf("\tSC %d\r\n", counter);
        #endif
        switch (counter++)
        {
        case 0: // Setup
          {
            if (cert_state == NULL) {
              Reset();
              break ;
            }

            FirmwareBlock* fwb = reinterpret_cast<FirmwareBlock*>(buffer);
            CertificateData* cert = reinterpret_cast<CertificateData*>(fwb->flashBlock);

            uint8_t digest[SHA512_DIGEST_SIZE];
            sha512_done(firmware_digest, digest);
            sha512_init(firmware_digest);

            verify_init(*cert_state, RSA_CERT_MONT, CERT_RSA, digest, cert->data, cert->length);
            
            i2spiSwitchMode(I2SPI_PAUSED);
            
            break ;
          }
        case 1: // Stage 1
          {
            verify_stage1(*cert_state);
            break ;
          }
        case 2: // Stage 2
          {
            verify_stage2(*cert_state);
            break ;
          }
        case 3: // Stage 3
          {
            verify_stage3(*cert_state);
            break ;
          }
        case 4: // Stage 4
          {
            if (!verify_stage4(*cert_state)) {
              Reset();
              break ;
            } else {
              haveValidCert = true;
            }
            retries = MAX_RETRIES*MAX_RETRIES; // More retries just after sig check
            bufferUsed -= sizeof(FirmwareBlock);
            os_memmove(buffer, buffer + sizeof(FirmwareBlock), bufferUsed);
            bytesProcessed += sizeof(FirmwareBlock);
            ack.bytesProcessed = bytesProcessed;
            ack.result = OKAY;
            RobotInterface::SendMessage(ack);
            counter = 0;
            phase = OTAT_Flash_Verify;
            #if DEBUG_OTA > 1
            AnkiDebug( 172, "UpgradeController.state", 461, "Flash verify", 0);
            os_printf("Flash verify\r\n");
            #endif
            i2spiSwitchMode(I2SPI_BOOTLOADER);
            break;
          }
        }
        break ;
      }
      case OTAT_Abort:
      {
        i2spiBootloaderCommandDone();
        break;
      }
      case OTAT_Apply_WiFi:
      {
        SpiFlashOpResult rslt = SPI_FLASH_RESULT_OK;
        if (didWiFi)
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
          AnkiDebug( 172, "UpgradeController.state", 463, "Reboot", 0);
          phase   = OTAT_Reboot;
          //phase   = OTAT_Apply_RTIP;
          //timer = system_get_time() + 10000000; // 10 second timeout
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
            timer = system_get_time() + 2000000; // 2 second timeout
          }
        }
        else if (system_get_time() > timer)
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
            if (system_get_time() > timer)
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
        if (i2spiGetRtipBootloaderState() == STATE_NACK)
        {
          phase = OTATR_Set_Evil_A;
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
          timer = system_get_time() + 1000000; // 1s second delay
          phase = OTATR_Delay;
        }
        break;
      }
      case OTATR_Delay:
      {
        if (system_get_time() > timer) phase = OTATR_Get_Size;
        break;
      }
      case OTATR_Get_Size:
      {
        if (spi_flash_read(FACTORY_RTIP_BODY_FW_SECTOR * SECTOR_SIZE, reinterpret_cast<uint32_t*>(&counter), 4) == SPI_FLASH_RESULT_OK)
        {
          if (counter < (int)sizeof(FirmwareBlock))
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
              timer = system_get_time() + 200000; // 200ms
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
          case STATE_IDLE:
          {
            if (system_get_time() > timer)
            {
              #if DEBUG_OTA
              os_printf("Timeout\t");
              #endif
              phase = OTATR_Read;
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
