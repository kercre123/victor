#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "portable.h"
#include "hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "ota.h"
#include "crc32.h"
#include "lights.h"

#include "boot/recovery.h"

#include "anki/cozmo/robot/rec_protocol.h"

enum TRANSMIT_MODE {
  TRANSMIT_NONE,
  TRANSMIT_SEND,
  TRANSMIT_RECEIVE
};

#define Count() (NRF_RTC1->COUNTER << 8)
#define MS(f)   (int)((f) * 32768 * 256 / 1000.0f)

// 30 seconds worth of idle before we pull the plug
static int max_idle;
static TRANSMIT_MODE uart_mode;
static const int IDLE_COUNT = 10; // How many IDLE commands before we default the LED channel

// === Return to application ===
__asm void ResetApplication() {
VectorTbl EQU   0x1801C
          LDR   r0, =VectorTbl
          LDR   r1, [r0, #0]  ; Stack pointer
          MOV   sp, r1
          LDR   r0, [r0, #4]  ; Reset address
          BX    r0
          ALIGN
}

// === Time functions ===
static void MicroWait(u32 time)
{
  loop: __asm {
      SUBS time, time, #1
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      NOP
      BNE loop
  }
}

// === Watchdog functions ===
static void kickDog(void) {
  for (int channel = 0; channel < 8; channel++) {
    NRF_WDT->RR[channel] = WDT_RR_RR_Reload;
  }
}

// === Flash functions ===
static void EraseSector(int target) {
  // Erase the sector and continue
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->ERASEPAGE = target;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
}

static bool FlashSector(int target, const uint32_t* data)
{
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  
  volatile uint32_t* original = (uint32_t*)target;

  EraseSector(target);

  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++, target += sizeof(uint32_t)) {
    // Program word does not need to be written
    if (data[i] == original[i]) {
      continue ;
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    original[i] = data[i];
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  }

  return true;
}

static inline void ClearEvilWord() {
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  *(uint32_t*)&IMAGE_HEADER->evil_word = 0x00000000;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
}

// === UART Functions ===
static void setTransmitMode(TRANSMIT_MODE mode) {
  if (uart_mode == mode) {
    return ;
  }

  // Shut it down (because reasons)
  NRF_UART0->TASKS_STOPRX = 1;
  NRF_UART0->TASKS_STOPTX = 1;
    
  uart_mode = mode;

  switch (mode) {
    case TRANSMIT_SEND:
      // Prevent debug words from transmitting
      NRF_UART0->PSELRXD = 0xFFFFFFFF;
      NRF_UART0->PSELTXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      MicroWait(5);
      NRF_UART0->TASKS_STARTTX = 1;

      break ;
    case TRANSMIT_RECEIVE:
      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      NRF_UART0->EVENTS_RXDRDY = 0;
      NRF_UART0->TASKS_STARTRX = 1;
      break ;

    default:
      break ;
  }
}

static void writeByte(uint8_t data) {
  setTransmitMode(TRANSMIT_SEND);

  NRF_UART0->EVENTS_TXDRDY = 0;
  NRF_UART0->TXD = data;
  while (!NRF_UART0->EVENTS_TXDRDY) ;
}

static uint8_t readByte() {
  setTransmitMode(TRANSMIT_RECEIVE);

  // We timeout in a little more than 50ms
  int timeout = Count() + MS(100);
  
  while (!NRF_UART0->EVENTS_RXDRDY) {
    Lights::update();

    int ticks = timeout - Count();
    
    if (ticks < 0) {
      return 0xFF;
    }
  }

  NRF_UART0->EVENTS_RXDRDY = 0;

  return NRF_UART0->RXD;
}

static void readUart(void* p, int length) {
  uint8_t* data = (uint8_t*) p;

  while (length-- > 0) {
    *(data++) = readByte();
  }
}

static void writeUart(const void* p, int length) {
  const uint8_t* data = (const uint8_t*) p;

  while (length-- > 0) {
    writeByte(*(data++));
  }
}

// Various commands
static inline bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;
  
  // Compute signature length
  uint32_t crc = calc_crc32(IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);

  return crc == IMAGE_HEADER->checksum;
}

static inline bool CheckEvilWord(void) {
  return IMAGE_HEADER->evil_word == 0x00000000;
}

static inline bool FlashBlock() {
  FirmwareBlock packet;
  
  // Load raw packet into memory
  readUart(&packet, sizeof(packet));

  int writeAddress = packet.blockAddress;

  // We will not override the boot loader
  if (writeAddress < BODY_SECURE_SPACE || writeAddress >= BODY_BOOTLOADER) {
    return false;
  }
 
  // Unaligned block
  if (packet.blockAddress % TRANSMIT_BLOCK_SIZE) {
    return false;
  }

  // Verify that our header is valid
  if (packet.blockAddress == BOOT_HEADER_LOCATION) {
    BootLoaderSignature* header = (BootLoaderSignature*) packet.flashBlock;

    if (header->sig != HEADER_SIGNATURE || !header->evil_word) {
      return false;
    }
  }

  // Check the CRC of the packet to verify that transmission actually worked
  uint32_t crc = calc_crc32((uint8_t*)packet.flashBlock, sizeof(packet.flashBlock));

  // Verify block before writting to flash
  if (crc != packet.checkSum) return false;

  // Write sectors to flash
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  for (int i = 0; i < TRANSMIT_BLOCK_SIZE; i+= FLASH_BLOCK_SIZE) {
    if (!FlashSector(writeAddress + i,&packet.flashBlock[i / sizeof(uint32_t)])) {
      return false;
    }
  }
  
  return true;
}

static void Sync(void) {
  int timeout = 0;
  uint32_t word = 0;
  
  // Sync up to the head
  do {
    int ticks = timeout - Count();
    
    if (ticks <= 0) {
      writeByte(0);
      writeUart(&BODY_RECOVERY_NOTICE, sizeof(BODY_RECOVERY_NOTICE));
      timeout = Count() + MS(100);
    }

    word = (word << 8) | readByte();
  } while(word != __REV(HEAD_RECOVERY_NOTICE));
}

static void EnterRecovery(void) {
  Sync();

  int idle_count = IDLE_COUNT;
  
  for (;;) {
    // Receive command packet
    RECOVERY_STATE state = STATE_UNKNOWN;

    uint8_t cmd = readByte();

    switch (cmd) {
      case COMMAND_DONE:
        // Signature is invalid
        if (!CheckSig()) {
          state = STATE_NACK;
          break ;
        }

        writeByte((uint8_t) STATE_ACK);

        // This makes sure that we handle the reboot loop properly
        if (CheckEvilWord()) {
          kickDog();
          Lights::stop();
          ResetApplication();
        } else {
          ClearEvilWord();
          return ;
        }

        break ;

      case COMMAND_BOOT_READY:
        state = (CheckEvilWord() && CheckSig()) ? STATE_ACK : STATE_NACK;
        break ;

      case COMMAND_CHECK_SIG:
        state = CheckSig() ? STATE_ACK : STATE_NACK;
        break ;

      case COMMAND_FLASH:
        state = FlashBlock() ? STATE_ACK : STATE_NACK;
        break ;
      
      case COMMAND_SET_LED:
        idle_count = IDLE_COUNT;  
        Lights::setChannel(readByte());
        state = STATE_ACK;
        break ;
      
      case COMMAND_PAUSE:
        while(readByte() != COMMAND_RESUME) ;
        break ;
      
      case COMMAND_IDLE:
        // We need to eject after a maximum period of time
        if (--max_idle <= 0) NVIC_SystemReset();
        
        kickDog();

        // This is a no op and we don't want to send a reply (so it doesn't stall out the k02)
        if (idle_count-- < 0) {
          Lights::setChannel(1);
        }

        continue ;

      default:
        // This is not a command, possibly line noise, don't respond
        continue ;
    }

    writeByte((uint8_t) state);
  }
}

int main(void) {
  Lights::init();
  
  // Setup Globals
  max_idle = 30 * 50; // 30 seconds worth of idle before we pull the plug
  uart_mode = TRANSMIT_NONE;

  // Enable the peripheral
  NRF_UART0->POWER = 1;
  NRF_UART0->CONFIG = 0;
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->INTENCLR = ~0;

  // Make sure the watchdog knows not to fail
  kickDog();

  // This makes sure that if the OTA fails, the robot will revert to factory
  EraseSector(0x18000);

  for (;;) {
    EnterRecovery();
  }
}
