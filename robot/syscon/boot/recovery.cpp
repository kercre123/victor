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

// These are all the magic numbers for the boot loader
static const int target_pin = PIN_TX_HEAD;
static bool UartWritting;
static const int UART_TIMEOUT = 0x4000 << 8;

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

void setLight(uint8_t clr) {
	int channel = (clr >> 3) & 3;
	
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
	NRF_UART0->EVENTS_TXDRDY = 0;
	NRF_UART0->EVENTS_RXDRDY = 0;
	
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
  while (NRF_UART0->EVENTS_RXDRDY) {	
		NRF_UART0->EVENTS_RXDRDY = 0;
		uint8_t temp = NRF_UART0->RXD;
	}
	
  NRF_UART0->EVENTS_TXDRDY = 0;
}

static void readUart(void* p, int length) {
	uint8_t* data = (uint8_t*) p;
	setTransmit(false);
	
	while (length-- > 0) {
		int waitTime = GetCounter() + UART_TIMEOUT;

		while (!NRF_UART0->EVENTS_RXDRDY) {
			int ticks = waitTime - GetCounter();
			if (ticks < 0) {
				NVIC_SystemReset();
			}

			Battery::manage();
		}

		NRF_UART0->EVENTS_RXDRDY = 0;
		*(data++) = NRF_UART0->RXD;
	}
}

static void writeUart(const void* p, int length) {
	uint8_t* data = (uint8_t*) p;
	setTransmit(true);
	
	while (length-- > 0) {
		NRF_UART0->TXD = *(data++);

		while (!NRF_UART0->EVENTS_TXDRDY) ;

		NRF_UART0->EVENTS_TXDRDY = 0;
	}
}

static uint8_t readByte() {
	uint8_t byte;
	readUart(&byte, sizeof(byte));
	
	return byte;
}

static uint8_t writeByte(const uint8_t byte) {
	writeUart(&byte, sizeof(byte));
}

static void SyncToHead(void) {
	uint8_t color;

	// Write our recovery sync signal
	writeUart(&BODY_RECOVERY_NOTICE, sizeof(BODY_RECOVERY_NOTICE));
	setTransmit(false);
	
	uint32_t recoveryWord = 0;

	// This will read a word, and attempt to sync to head
	readUart(&recoveryWord, sizeof(recoveryWord));

	// Simply restart when we receive bad data
	if (recoveryWord != HEAD_RECOVERY_NOTICE) {
		NVIC_SystemReset();
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
  uint8_t* raw = (uint8_t*) &packet;
  
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

void BlinkALot(void) {
  uint8_t colors[] = {
		0x02 + 16, 0x02 +  8, 0x04 + 24,
		0x04 + 24, 0x04 +  8, 0x02 + 16,
		0x06 + 16, 0x06 +  8, 0x06 + 24,
		0x07 + 24, 0x07 +  8, 0x07 + 16
	};
	
	for (int i = 0; i < sizeof(colors); i++) {
		setLight(colors[i]);
    MicroWait(50000);
  }
}

void EnterRecovery(void) {
  BlinkALot();
  UARTInit();
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
