#include "../../include/anki/cozmo/robot/rec_protocol.h"

#include "MK02F12810.h"
#include "recovery.h"
#include "crc32.h"
#include "timer.h"
#include "uart.h"

using namespace Anki::Cozmo::HAL;

extern bool CheckSig(void);

static const int FLASH_BLOCK_SIZE = 0x800;
static uint32_t recovery_word __attribute__((at(0x20001FFC)));
static const uint32_t recovery_value = 0xCAFEBABE;

static RECOVERY_STATE body_state = STATE_UNKNOWN;

static union {
	FirmwareBlock packet;
	commandWord rawWords[1];
};

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

static void UpdateBodyState() {
	static uint16_t header = 0;

	while (UART::rx_avail()) {	
		// Try to syncronize to the header
		if (header != COMMAND_HEADER) {
			header = (header << 8) | UART::readByte();
			continue ;
		}
		
		if (UART0_RCFIFO < 2) {
			return ;
		}
		
		// Read body state
		body_state = (RECOVERY_STATE) UART::readWord();
		header = 0;
	}
}

static void WaitForState(void) {
	body_state = STATE_BUSY;
	UART::receive();
	
	while (body_state == STATE_BUSY) {
		UpdateBodyState();
	}
}

static bool SendBodyCommand(uint16_t command, const void* body = NULL, int length = 0) {	
	SPI0_PUSHR_SLAVE = STATE_BUSY;
	
	WaitForState();
	UART::writeWord(COMMAND_HEADER);
	UART::writeWord(command);
	UART::write(body, length);
	WaitForState();
	
	SPI0_PUSHR_SLAVE = body_state;
	
	return (body_state == STATE_IDLE);
}

static bool CheckBodySig(void) {
	return SendBodyCommand(COMMAND_CHECK_SIG);
}

static bool CheckBodyEvilWord(void) {
	return SendBodyCommand(COMMAND_EVIL_WORD);
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

bool CheckEvilWord(void) {
	return IMAGE_HEADER->evil_word == 0;
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

static inline bool FlashBlock() {
  static const int length = sizeof(FirmwareBlock) / sizeof(commandWord);
  
  // Load raw packet into memory
  for (int i = 0; i < length; i++) {
    rawWords[i] = WaitForWord();
  }

	// Upper 2gB is body space
	if (packet.blockAddress >= 0x80000000) {
		packet.blockAddress &= ~0x80000000;
		return SendBodyCommand(COMMAND_FLASH, &packet, sizeof(packet));
	}
	
  // We will not override the boot loader
  if (packet.blockAddress < ROBOT_BOOTLOADER) {
    return false;
  }
  
  // Verify block before writting to flash
	uint32_t crc = calc_crc32((uint8_t*)packet.flashBlock, sizeof(packet.flashBlock));
	
	if (crc != packet.checkSum) {
		return false;
	}

  // Write sectors to flash
  for (int i = 0; i < TRANSMIT_BLOCK_SIZE; i+= FLASH_BLOCK_SIZE) {
    if (!FlashSector(packet.blockAddress + i, &packet.flashBlock[i / sizeof(uint32_t)])) {
      return false;
    }
  }
  
  return true;
}

static bool SetLight(int colors) {
	uint8_t command[] = {0, colors};
	
	return SendBodyCommand(COMMAND_SET_LED, &command, sizeof(command));
}

void EnterRecovery() {  
	// Let the espressif know we are still booting
	SPI0_PUSHR_SLAVE = STATE_BUSY;

	// These are the requirements to boot immediately into the application
	// if any test fails, the robot will not exit recovery mode
	bool recovery_escape = 
		(recovery_word != recovery_value) &&
		CheckSig() &&
		CheckEvilWord() &&
		CheckBodyEvilWord();

	// If the body says it's safe, feel free to exit
	if (recovery_escape && SendBodyCommand(COMMAND_DONE)) {
		return ;
	}

	// We are now ready to start receiving commands
	SPI0_PUSHR_SLAVE = STATE_IDLE;
	for (;;) {
		// Make sure that our 
    do {
			UpdateBodyState();
		} while (WaitForWord() != COMMAND_HEADER);
    
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

				recovery_word = 0;
				SPI0_PUSHR_SLAVE = STATE_IDLE;
        return ;
     
			case COMMAND_CHECK_SIG:
				SPI0_PUSHR_SLAVE = (CheckSig() && CheckBodySig()) ? STATE_IDLE : STATE_NACK;
				break ;

      case COMMAND_EVIL_WORD:
				SPI0_PUSHR_SLAVE = (CheckEvilWord() && CheckBodyEvilWord()) ? STATE_IDLE : STATE_NACK;
				break ;
				
			case COMMAND_SET_LED:
				;
				SPI0_PUSHR_SLAVE = SetLight(WaitForWord()) ? STATE_IDLE : STATE_NACK;
				break ;

      case COMMAND_FLASH:
        SPI0_PUSHR_SLAVE = FlashBlock() ? STATE_IDLE : STATE_NACK;
        break ;
    }
  }
}
