#include "../../include/anki/cozmo/robot/rec_protocol.h"

#include "MK02F12810.h"
#include "recovery.h"
#include "sha1.h"
#include "timer.h"

static const int FLASH_BLOCK_SIZE = 0x800;

static inline commandWord WaitForWord(void) {
  while(~SPI0_SR & SPI_SR_RFDF_MASK) ;  // Wait for a byte
  commandWord ret = SPI0_POPR;
 
  SPI0_SR = SPI0_SR;
  return ret;
}

extern int counterVolume;
int counterVolume = 0;

int SendCommand()
{
  const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

  // Wait for idle and clear errors
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;
  FTFA->FSTAT = FSTAT_ERROR;

  // Start flash command (and wait for completion)
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) counterVolume++;

  // Return if there was an error
  return FTFA->FSTAT & FSTAT_ERROR;
}

bool FlashSector(int target, const uint32_t* data)
{
  volatile const uint32_t* original = (uint32_t*)target;
  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;

  // Test for sector erase nessessary
  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++) {
    if (original[i] == 0xFFFFFFFF) continue ;
    
    // Block requires erasing
    if (original[i] != data[i])
    {
      FTFA->FCCOB0 = 0x09;
      FTFA->FCCOB1 = target >> 16;
      FTFA->FCCOB2 = target >> 8;
      FTFA->FCCOB3 = target;
      
      // Erase block
      if (SendCommand()) { 
        return false;
      }

      break ;
    }
  }

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Program word does not need to be written
    if (data[i] == original[i]) {
      continue ;
    }

    // Write program word
    flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
    flashcmd[1] = data[i];
    
    // Did command succeed
    if (SendCommand()) { 
      return false;
    }
  }

  return true;
}


static inline bool FlashBlock() {
  static const int length = sizeof(FirmwareBlock) / sizeof(commandWord);
  
  static FirmwareBlock packet;    
  static commandWord* raw = (commandWord*) &packet;

  // Load raw packet into memory
  for (int i = 0; i < length; i++) {
    raw[i] = WaitForWord();
  }

  MicroWait(5000);
  
  // Check the SHA-1 of the packet to verify that transmission actually worked
  SHA1_CTX ctx;
  sha1_init(&ctx);
  sha1_update(&ctx, (uint8_t*)packet.flashBlock, sizeof(packet.flashBlock));

  uint8_t sig[SHA1_BLOCK_SIZE];
  sha1_final(&ctx, sig);

  int writeAdddress = packet.blockAddress;

  // We will not override the boot loader
  if (writeAdddress < SECURE_SPACE) {
    return false;
  }
  
  // Verify block before writting to flash
  for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
    if (sig[i] != packet.checkSum[i])
    {
      return false;
    }
  }

  // Write sectors to flash
  for (int i = 0; i < TRANSMIT_BLOCK_SIZE; i+= FLASH_BLOCK_SIZE) {
    if (!FlashSector(writeAdddress + i,&packet.flashBlock[i / sizeof(uint32_t)])) {
      return false;
    }
  }
  
  return true;
}

void EnterRecovery(void) {
  SPI0_PUSHR_SLAVE = STATE_IDLE;
  for (;;) {
    while (WaitForWord() != COMMAND_HEADER);
    SPI0_PUSHR_SLAVE = STATE_BUSY;
    
    // Receive command packet
    switch (WaitForWord()) {
      case COMMAND_DONE:
        SPI0_PUSHR_SLAVE = STATE_IDLE;
        return ;
     
      case COMMAND_FLASH:
        SPI0_PUSHR_SLAVE = FlashBlock() ? STATE_IDLE : STATE_NACK;
        break ;
    }
  }
}
