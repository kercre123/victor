#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "recovery.h"
#include "sha1.h"
#include "timer.h"
#include "../../hal/hardware.h"
#include "../../../include/anki/cozmo/robot/spineData.h"

#include "rec_protocol.h"

static const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
static const int SECURE_SPACE = 0x1000;
static const int MAX_TIMEOUT = 1000000;

static bool UartWritting = false;

void setTransmit(bool tx) {
  if (UartWritting == tx) return ;

  UartWritting = tx;
  
  if (tx) {
    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    NRF_UART0->PSELTXD = PIN_TX_HEAD;

    // Configure pin so it is open-drain
    nrf_gpio_cfg_output(PIN_TX_HEAD);

    NRF_GPIO->PIN_CNF[PIN_TX_HEAD] = 
      (NRF_GPIO->PIN_CNF[PIN_TX_HEAD] & ~GPIO_PIN_CNF_DRIVE_Msk) |
      (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos);
  } else {
    nrf_gpio_cfg_input(PIN_TX_HEAD, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD = 0xFFFFFFFF;
    MicroWait(10);
    NRF_UART0->PSELRXD = PIN_TX_HEAD;
  }

  // Clear our UART interrupts
  NRF_UART0->EVENTS_RXDRDY = 0;
  NRF_UART0->EVENTS_TXDRDY = 0;
}

static uint8_t ReadByte(void) {
  while (!NRF_UART0->EVENTS_RXDRDY) ;

  // We received a word
  NRF_UART0->EVENTS_RXDRDY = 0;
  return NRF_UART0->RXD;
}

static int ReadUart(void) {
  setTransmit(false);

  // Wait with timeout
  int waitTime = GetCounter() + MAX_TIMEOUT;
  while (!NRF_UART0->EVENTS_RXDRDY) {
    if (GetCounter() > waitTime) return -1;
  }

  // We received a word
  NRF_UART0->EVENTS_RXDRDY = 0;
  return NRF_UART0->RXD;
}

static bool WaitWord(uint32_t target) {
  uint32_t word = 0;
  
  for (int i = 0; i < sizeof(target); i++) {
    int read = ReadUart();

    if (read < 0) return false;
    
    word |= read << (i * 8);
  }

  return word == target;
}

static void WriteUart(uint8_t data) {
  setTransmit(true);

  NRF_UART0->EVENTS_TXDRDY = 0;    
  NRF_UART0->TXD = data;

  while (!NRF_UART0->EVENTS_TXDRDY) ;
}

static void WriteWord(uint32_t word) {
  for (int i = 0; i < sizeof(word); i++) {
    WriteUart((word >> (i * 8)) & 0xFF);
  }
}

void UARTInit(void) {
  // Power on the peripheral
  NRF_UART0->POWER = 1;

  // Disable parity and hardware flow-control
  NRF_UART0->CONFIG = 0;

  // Enable the peripheral and start the tasks
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTTX = 1;
  NRF_UART0->TASKS_STARTRX = 1;

  // Initialize the UART for the specified baudrate
  NRF_UART0->BAUDRATE = NRF_BAUD(spine_baud_rate);

  // We begin in receive mode (slave)
  setTransmit(false);
}

bool FlashSector(int target, const uint32_t* data)
{
  volatile uint32_t* original = (uint32_t*)target;

  // Test for sector erase nessessary
  for (int i = 0; i < FLASH_BLOCK_SIZE / sizeof(uint32_t); i++) {
    if (original[i] == 0xFFFFFFFF) continue ;
    
    // Block requires erasing
    if (original[i] != data[i])
    {
      // Erase the sector and continue
      NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
      NRF_NVMC->ERASEPAGE = target;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
      NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
      while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
      break ;
    }
  }

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


static inline bool FlashBlock() {
  struct Packet {
    uint32_t   flashBlock[TRANSMIT_BLOCK_SIZE / sizeof(uint32_t)];
    uint32_t   blockAddress;
    uint8_t    checkSum[SHA1_BLOCK_SIZE];
  };

  static union {
    Packet packet;    
    uint8_t raw[sizeof(Packet)];
  };

  // Load raw packet into memory
  for (int i = 0; i < sizeof(raw); i++) {
    raw[i] = ReadByte();
  }

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
    if (sig[i] != packet.checkSum[i]) return false;
  }

  // Write sectors to flash
  for (int i = 0; i < TRANSMIT_BLOCK_SIZE; i+= FLASH_BLOCK_SIZE) {
    if (!FlashSector(writeAdddress + i,&packet.flashBlock[i])) {
      return false;
    }
  }
  
  return true;
}

void EnterRecovery(void) {
  UARTInit();

  RECOVERY_STATE state = STATE_IDLE;

  for (;;) {
    do {
      WriteWord(RECOVER_SOURCE_BODY);
      WriteUart(state);
    } while (!WaitWord(RECOVER_SOURCE_HEAD));

    // Receive command packet
    switch (ReadByte()) {
      case COMMAND_DONE:
        state = STATE_IDLE;
        return ;
     
      case COMMAND_FLASH:
        state = FlashBlock() ? STATE_IDLE : STATE_NACK;
        break ;
    }
  }
}
