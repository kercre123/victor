#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "recovery.h"
#include "crc32.h"
#include "timer.h"
#include "battery.h"
#include "hal/hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "anki/cozmo/robot/rec_protocol.h"

static const int MAX_TIMEOUT = 1000000;

static const int target_pin = PIN_TX_HEAD;
static bool UartWritting;

void setTransmit(bool tx);

// Define charlie wiring here:
struct charliePlex_s {
  int anode;
  int cathodes[3];
};

static inline void setCathode(int pin, bool set) {
  if (set) {
    nrf_gpio_pin_clear(pin);
    nrf_gpio_cfg_output(pin);
  } else {
    nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
  }
}

void setLight(int channel) {
	int clr = channel >> 2;
	channel %= 4;
	
  static const charliePlex_s RGBLightPins[] =
  {
    // anode, cath_red, cath_gree, cath_blue
    {PIN_LED1, {PIN_LED2, PIN_LED3, PIN_LED4}},
    {PIN_LED2, {PIN_LED1, PIN_LED3, PIN_LED4}},
    {PIN_LED3, {PIN_LED1, PIN_LED2, PIN_LED4}},
    {PIN_LED4, {PIN_LED1, PIN_LED2, PIN_LED3}}
  };

  // Setup anode
  nrf_gpio_pin_set(RGBLightPins[channel].anode);
  nrf_gpio_cfg_output(RGBLightPins[channel].anode);
  
  // Set lights for current charlie channel
  setCathode(RGBLightPins[channel].cathodes[0], clr & 1);
  setCathode(RGBLightPins[channel].cathodes[1], clr & 2);
  setCathode(RGBLightPins[channel].cathodes[2], clr & 4);
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

void toggleTargetPin(void) {
  //target_pin = (target_pin == PIN_TX_HEAD) ? PIN_TX_VEXT : PIN_TX_HEAD;
}

void setTransmit(bool tx) {
  if (UartWritting == tx) return ;

  UartWritting = tx;
  
  if (tx) {
    NRF_UART0->PSELRXD = 0xFFFFFFFF;
    MicroWait(10);
    NRF_UART0->PSELTXD = target_pin;

    // Configure pin so it is open-drain
    nrf_gpio_cfg_output(target_pin);

    NRF_GPIO->PIN_CNF[target_pin] = 
      (NRF_GPIO->PIN_CNF[target_pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
      (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos);
  } else {
    nrf_gpio_cfg_input(target_pin, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD = 0xFFFFFFFF;
    MicroWait(10);
    NRF_UART0->PSELRXD = target_pin;
  }

  // Clear our UART interrupts
  NRF_UART0->EVENTS_RXDRDY = 0;
  uint8_t test = NRF_UART0->RXD;
  NRF_UART0->EVENTS_TXDRDY = 0;
}

static uint8_t ReadByte(void) {
  while (!NRF_UART0->EVENTS_RXDRDY) {
    Battery::manage();
  }
  NRF_UART0->EVENTS_RXDRDY = 0;
  return NRF_UART0->RXD;
}

static commandWord ReadWord(void) {
  commandWord word = 0;
  
  for (int i = 0; i < sizeof(commandWord); i++) {
    word = ReadByte() | (word << 8);
  }

  return word;
}

static bool WaitWord(commandWord target) {
  setTransmit(false);

  commandWord word = 0;
  
  while (target != word) {
    // Wait with timeout
    int waitTime = GetCounter() + MAX_TIMEOUT;
    while (!NRF_UART0->EVENTS_RXDRDY) {
      Battery::manage();

      signed int remaining = waitTime - GetCounter();
      if (remaining <= 0) return false;
    }

    // We received a word
    NRF_UART0->EVENTS_RXDRDY = 0;
    word = NRF_UART0->RXD | (word << 8);
  }

  return word == target;
}

static void WriteWord(commandWord word) {
  setTransmit(true);

  for (int i = sizeof(commandWord) - 1; i >= 0; i--) {
    NRF_UART0->TXD = word >> (i * 8);

    while (!NRF_UART0->EVENTS_TXDRDY) ;
    NRF_UART0->EVENTS_TXDRDY = 0;
  }
}

bool FlashSector(int target, const uint32_t* data)
{
  const int FLASH_BLOCK_SIZE = NRF_FICR->CODEPAGESIZE;
  
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
  static FirmwareBlock packet;
  uint8_t* raw = (uint8_t*) &packet;
  
  // Load raw packet into memory
  for (int index = 0; index < sizeof(FirmwareBlock); index++) {
    raw[index] = ReadByte();
  }
 
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

void BlinkALot(void) {
  for (int i = 0; i < 0x1F; i++) {
    setLight(i);
    MicroWait(10000);
  }
}

extern bool CheckSig(void);

void EnterRecovery(void) {
	BlinkALot();
	
  UARTInit();

  RECOVERY_STATE state = STATE_IDLE;

  for (;;) {
    do {
      toggleTargetPin();
      WriteWord(COMMAND_HEADER);
      WriteWord(state);
    } while (!WaitWord(COMMAND_HEADER));

    // Receive command packet
    switch (ReadWord()) {
			case COMMAND_DONE:
        state = STATE_IDLE;
        return ;
     
      case COMMAND_FLASH_BODY:
        state = FlashBlock() ? STATE_IDLE : STATE_NACK;
        break ;
      
      case COMMAND_SET_LED:
				setLight(ReadWord());
				break ;
			
			case COMMAND_CHECK_SIG:
				state = CheckSig() ? STATE_IDLE : STATE_NACK;
				break ;

			default:
				state = STATE_NACK;
        break ;
    }
  }
}
