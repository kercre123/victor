#include "../../include/anki/cozmo/robot/rec_protocol.h"

#include "MK02F12810.h"
#include "recovery.h"
#include "crc32.h"
#include "timer.h"
#include "uart.h"
#include "spi.h"
#include "power.h"

#include "portable.h"
#include "../../k02_hal/hal/hardware.h"

using namespace Anki::Cozmo::HAL;

extern bool CheckSig(void);

static union {
  FirmwareBlock packet;
  commandWord rawWords[1];
} flash;

static inline commandWord WaitForWord(void) {
  while(~SPI0_SR & SPI_SR_RFDF_MASK) ;  // Wait for a byte
  commandWord ret = SPI0_POPR;
 
  SPI0_SR = SPI0_SR;
  return ret;
}

int SendCommand()
{
  const int FSTAT_ERROR = FTFA_FSTAT_FPVIOL_MASK | FTFA_FSTAT_ACCERR_MASK | FTFA_FSTAT_FPVIOL_MASK;

  // Wait for idle and clear errors
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;
  FTFA->FSTAT = FSTAT_ERROR;

  // Start flash command (and wait for completion)
  FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
  while (FTFA_FSTAT_CCIF_MASK & ~FTFA->FSTAT) ;

  // Return if there was an error
  return FTFA->FSTAT & FSTAT_ERROR;
}

bool FlashSector(int target, const uint32_t* data)
{
  volatile const uint32_t* original = (uint32_t*)target;
  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;

  // Test for sector erase nessessary
  FTFA->FCCOB0 = 0x09;
  FTFA->FCCOB1 = target >> 16;
  FTFA->FCCOB2 = target >> 8;
  FTFA->FCCOB3 = target;
    
  // Erase block
  if (SendCommand()) { 
    return false;
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

static bool SendBodyCommand(uint8_t command, const void* body = NULL, int length = 0) {	
  UART::writeByte(command);
  UART::write(body, length);

  RECOVERY_STATE body_state = (RECOVERY_STATE) UART::readByte();

  if (body_state == STATE_ACK) {
    return true;
  } else if (body_state == STATE_NACK) {
    return false;
  }

  // Transmission error: Reboot
  NVIC_SystemReset();
  return false;
}

static bool SetLight(uint8_t colors) {
  return SendBodyCommand(COMMAND_SET_LED, &colors, sizeof(colors));
}

static bool CheckBodySig(void) {
  return SendBodyCommand(COMMAND_CHECK_SIG);
}

bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;

  unsigned int addr = (unsigned int) IMAGE_HEADER->rom_start;
  unsigned int length = IMAGE_HEADER->rom_length;
  
  if (addr + length > 0x10000) {
    return false;
  }

  // Compute signature length
  uint32_t crc = calc_crc32(IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);

  return crc == IMAGE_HEADER->checksum;
}

bool CheckBootReady(void) {
  if (IMAGE_HEADER->evil_word != 0) {
    return false;
  }

  return CheckSig();
}

void ClearEvilWord(void) {
  if (IMAGE_HEADER->evil_word != 0xFFFFFFFF) {
    return ;
  }

  volatile uint32_t* flashcmd = (uint32_t*)&FTFA->FCCOB3;
  int target = (uint32_t)&IMAGE_HEADER->evil_word;

  flashcmd[0] = (target & 0xFFFFFF) | 0x06000000;
  flashcmd[1] = 0;

  SendCommand();
}

static inline void SetChannelBlink(int channel) {
  // This is the color for the blink pattern
  static int color = 0;
  color = (color + 2) % 7;

  SetLight((channel << 3) | color);
}

static inline bool FlashBlock() {
  static const int length = sizeof(FirmwareBlock) / sizeof(commandWord);
  
  // Load raw packet into memory
  for (int i = 0; i < length; i++) {
    flash.rawWords[i] = WaitForWord();
  }

  // Verify block before writting to flash
  uint32_t crc = calc_crc32((uint8_t*)flash.packet.flashBlock, sizeof(flash.packet.flashBlock));

  if (crc != flash.packet.checkSum) {
    return false;
  }

  // Upper 2gB is body space
  if (flash.packet.blockAddress >= 0x80000000) {
    SetChannelBlink(2);
    flash.packet.blockAddress &= ~0x80000000;
    return SendBodyCommand(COMMAND_FLASH, &flash.packet, sizeof(flash.packet));
  } else {
    SetChannelBlink(3);
  }

  // We will not override the boot loader
  if (flash.packet.blockAddress < ROBOT_BOOTLOADER || flash.packet.blockAddress >= 0x10000) {
    return false;
  }

  MicroWait(1000);

  // Write sectors to flash
  for (int i = 0; i < TRANSMIT_BLOCK_SIZE; i+= FLASH_BLOCK_SIZE) {
    if (!FlashSector(flash.packet.blockAddress + i, &flash.packet.flashBlock[i / sizeof(uint32_t)])) {
      return false;
    }
  }

  return true;
}

static void SyncToBody(void) {
  uint32_t recovery_header = 0;

  while (recovery_header != BODY_RECOVERY_NOTICE) {
    UART0_S1 = UART0_S1;
    while (!UART0_RCFIFO) ;
    
    recovery_header = (UART0_D << 24) | (recovery_header >> 8);
  }

  MicroWait(10);

  UART::write(&HEAD_RECOVERY_NOTICE, sizeof(HEAD_RECOVERY_NOTICE));
}

void EnterRecovery(bool force) {  
  __disable_irq();
  
  // Pin to our body
  UART::receive();
  SyncToBody();

  // These are the requirements to boot immediately into the application
  // if any test fails, the robot will not exit recovery mode
  bool recovery_force = force || *recovery_word == recovery_value;
  bool remote_boot_ok = SendBodyCommand(COMMAND_BOOT_READY);
  bool boot_ok = !recovery_force && CheckBootReady() && remote_boot_ok;

  // If the body says it's safe, feel free to exit
  if (boot_ok && SendBodyCommand(COMMAND_DONE)) {
    return ;
  }

  // We know that booting the espressif will take awhile, so we should
  // just tell the body to pause until that finishes
  // NOTE: We don't start the espressif if it's already running
  
  if (~GPIO_POWEREN->PDDR & PIN_POWEREN) {
    UART::writeByte(COMMAND_PAUSE);
    Power::enableEspressif(false);
    SPI::init();
    UART::writeByte(COMMAND_RESUME);
  }

  // We are now ready to start receiving commands
  SPI0_PUSHR_SLAVE = STATE_IDLE;
  int next_idle = 0;
  for (;;) {
    // Keep the NRF happy by poking it occasionally
    int count = GetMicroCounter();
    if (next_idle - count <= 0) {
      UART::writeByte(COMMAND_IDLE);
      next_idle = count + 50000;
    }
    
    WDOG_REFRESH = 0xA602;
    WDOG_REFRESH = 0xB480;
    
    // Wait for a command from the host
    if (WaitForWord() != COMMAND_HEADER) {
      continue ;
    }
    
    SPI0_PUSHR_SLAVE = STATE_BUSY;
    
    // Receive command packet
    switch (WaitForWord()) {
      case COMMAND_DONE:
        // Cannot boot because the signature check failed
        if (!CheckSig()) {
          SPI0_PUSHR_SLAVE = STATE_NACK;
          break ;
        }
        
        // Body refused to reboot (probably back sig)
        if (!SendBodyCommand(COMMAND_DONE)) {
          SPI0_PUSHR_SLAVE = STATE_NACK;
          break ;
        }

        ClearEvilWord();

        *recovery_word = 0;
        SPI0_PUSHR_SLAVE = STATE_IDLE;

        NVIC_SystemReset();
        return ;

      case COMMAND_CHECK_SIG:
        SPI0_PUSHR_SLAVE = (CheckSig() && CheckBodySig()) ? STATE_IDLE : STATE_NACK;
        break ;

      case COMMAND_BOOT_READY:
        SPI0_PUSHR_SLAVE = (CheckBootReady() && SendBodyCommand(COMMAND_BOOT_READY)) ? STATE_IDLE : STATE_NACK;
        break ;
        
      case COMMAND_SET_LED:
        SPI0_PUSHR_SLAVE = SetLight(WaitForWord()) ? STATE_IDLE : STATE_NACK;
        break ;

      case COMMAND_FLASH:
        SPI0_PUSHR_SLAVE = FlashBlock() ? STATE_IDLE : STATE_NACK;
        break ;
    }
  }
}
