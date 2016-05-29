#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "lights.h"
#include "recovery.h"
#include "crc32.h"
#include "timer.h"
#include "battery.h"
#include "hal/hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "anki/cozmo/robot/rec_protocol.h"

enum TRANSMIT_MODE {
  TRANSMIT_NONE,
  TRANSMIT_SEND,
  TRANSMIT_RECEIVE,
  TRANSMIT_CHARGER_RX
};

// These are all the magic numbers for the boot loader
static TRANSMIT_MODE uart_mode = TRANSMIT_NONE;
static const int UART_TIMEOUT = 0x2000 << 8;

void setLight(uint8_t clr) {
  int channel = (clr >> 3) & 3;
  Lights::set(channel, clr & 7, 0x80);
}

void UARTInit(void) {
  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;

  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->INTENCLR = ~0;
  
  nrf_gpio_pin_set(PIN_TX_HEAD);
}

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
      // Configure pin so it is open-drain
      NRF_GPIO->PIN_CNF[PIN_TX_HEAD] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                    | (GPIO_PIN_CNF_DRIVE_H0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                                    | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                                    | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                    | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);

      // Prevent debug words from transmitting
      NRF_UART0->PSELRXD = 0xFFFFFFFF;
      NRF_UART0->PSELTXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      MicroWait(5);
      NRF_UART0->TASKS_STARTTX = 1;

      break ;
    case TRANSMIT_RECEIVE:
      nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);

      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_HEAD;
      NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

      MicroWait(5);
      NRF_UART0->TASKS_STARTRX = 1;
      break ;
    case TRANSMIT_CHARGER_RX:
      nrf_gpio_cfg_input(PIN_TX_VEXT, NRF_GPIO_PIN_PULLUP);

      NRF_UART0->PSELTXD = 0xFFFFFFFF;
      NRF_UART0->PSELRXD = PIN_TX_VEXT;
      NRF_UART0->BAUDRATE = NRF_BAUD(charger_baud_rate);

      MicroWait(5);
      NRF_UART0->TASKS_STARTRX = 1;
      break ;

    default:
      break ;
  }

  for (int i = 0; i < 6; i++) {
    uint8_t temp = NRF_UART0->RXD;
  }
  
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;
}

static void readUart(void* p, int length, bool timeout = true) {
  uint8_t* data = (uint8_t*) p;
  setTransmitMode(TRANSMIT_RECEIVE);

  while (length-- > 0) {
    int waitTime = GetCounter() + UART_TIMEOUT;

    while (!NRF_UART0->EVENTS_RXDRDY) {
      int ticks = waitTime - GetCounter();
      if (ticks < 0) {
        if (timeout) {
          NVIC_SystemReset();
        } else {
          static uint8_t color = 0x10;
          color = ((color + 1) & 7) | 0x10;
          setLight(color);
        }
        
        waitTime = GetCounter() + UART_TIMEOUT;
      }

      Battery::manage();
    }

    NRF_UART0->EVENTS_RXDRDY = 0;
    *(data++) = NRF_UART0->RXD;
  }
}

static void writeUart(const void* p, int length) {
  uint8_t* data = (uint8_t*) p;
  setTransmitMode(TRANSMIT_SEND);

  while (length-- > 0) {
    NRF_UART0->TXD = *(data++);

    while (!NRF_UART0->EVENTS_TXDRDY) ;

    NRF_UART0->EVENTS_TXDRDY = 0;
  }
  
}

static uint8_t readByte(bool timeout = true) {
  uint8_t byte;
  readUart(&byte, sizeof(byte), timeout);

  return byte;
}

static void writeByte(const uint8_t byte) {
  writeUart(&byte, sizeof(byte));
}

static bool EscapeFixture(void) {
  nrf_gpio_pin_set(PIN_TX_VEXT);
  nrf_gpio_cfg_output(PIN_TX_VEXT);

  MicroWait(15);

  setTransmitMode(TRANSMIT_CHARGER_RX);
  
  int waitTime = GetCounter() + (int)(0.0002f * 0x8000);
  
  do {
    while (!NRF_UART0->EVENTS_RXDRDY) {
      if (GetCounter() >= waitTime) {
        return false;
      }
    }
  } while (NRF_UART0->RXD != 0xA5);

  return true;
}


static void SyncToHead(void) {
  uint32_t recoveryWord = 0;

  // Write our recovery sync signal
  writeUart(&BODY_RECOVERY_NOTICE, sizeof(BODY_RECOVERY_NOTICE));
  readUart(&recoveryWord, sizeof(recoveryWord));

  if (recoveryWord != HEAD_RECOVERY_NOTICE) {
    NVIC_SystemReset();
  }
}

bool FlashSector(int target, const uint32_t* data)
{
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  
  volatile uint32_t* original = (uint32_t*)target;

  // Erase the sector and continue
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->ERASEPAGE = target;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;

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

// Boot loader info
bool CheckSig(void) {
  if (IMAGE_HEADER->sig != HEADER_SIGNATURE) return false;
  
  // Compute signature length
  uint32_t crc = calc_crc32(IMAGE_HEADER->rom_start, IMAGE_HEADER->rom_length);

  return crc == IMAGE_HEADER->checksum;
}

bool CheckEvilWord(void) {
  return IMAGE_HEADER->evil_word == 0x00000000;
}

static inline void ClearEvilWord() {
  if (IMAGE_HEADER->evil_word != 0xFFFFFFFF) {
    return ;
  }

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  *(uint32_t*)&IMAGE_HEADER->evil_word = 0x00000000;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
}

static inline bool FlashBlock() {
  static FirmwareBlock packet;
  
  // Load raw packet into memory
  readUart(&packet, sizeof(packet));
 
  int writeAddress = packet.blockAddress;

  // We will not override the boot loader
  if (writeAddress < BODY_SECURE_SPACE || writeAddress >= BODY_BOOTLOADER) {
    return false;
  }
 
  // Check the SHA-1 of the packet to verify that transmission actually worked
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


void EnterRecovery(void) {
  UARTInit();
  
  *FIXTURE_HOOK = EscapeFixture() ? 0xDEADFACE : 0;

  // Disconnect input so we don't dump current into the charge pin
  NRF_GPIO->PIN_CNF[PIN_TX_VEXT] = GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos;

  if (*FIXTURE_HOOK) {
    return ;
  }
  
  SyncToHead();

  for (;;) {
    // Receive command packet
    RECOVERY_STATE state;
    
    switch (readByte()) {
      case COMMAND_DONE:
        // Signature is invalid
        if (!CheckSig()) {
          state = STATE_NACK;
          break ;
        }

        ClearEvilWord();
        writeByte((uint8_t) STATE_ACK);
        return ;

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
        setLight(readByte());
        state = STATE_ACK;
        break ;
      
      case COMMAND_PAUSE:
        while(readByte(false) != COMMAND_RESUME) ;
        break ;
      
      case COMMAND_IDLE:
        // This is a no op and we don't want to send a reply (so it doesn't stall out the k02)
        static int color = 0x0E;
        setLight(color ^= 0x07);
        continue ;
              
      default:
        state = STATE_NACK;
        break ;
    }
    
    writeByte((uint8_t) state);
  }
}
