#include <stdint.h>
#include <string.h>

#include "tests.h"
#include "nrf.h"
#include "nrf51_bitfields.h"

extern "C" {
  #include "micro_esb.h"
}
  
#include "hardware.h"
#include "debug.h"
#include "radio.h"
#include "timer.h"
#include "head.h"

#include "nrf_gpio.h"

// Blinky status lights!
#include "anki/cozmo/robot/spineData.h"

#define PACKET_SIZE 13
#define TIME_PER_CUBE (int)(COUNT_PER_MS * 33.0 / MAX_CUBES)
#define TICK_LOOP   7

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

#ifdef RADIO_TIMING_TEST
  const uint8_t     cubePipe[] = {1};
  #define RADIO_ADDRS {0xE7,0xC1,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8}
#elif defined(ROBOT41)
  // Robot #2
  //const uint8_t     cubePipe[] = {1,2,3,4};
  //const uint8_t     radioCannel = 83;
  //#define RADIO_ADDRS {0xE6,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8}
//#else
  const uint8_t     cubePipe[] = {1,2,3,4};
  const uint8_t     radioCannel = 82;
  #define RADIO_ADDRS {0xE7,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8}
#endif

#define MAX_CUBES sizeof(cubePipe)

LEDPacket         cubeTx[MAX_CUBES];
AcceleratorPacket cubeRx[MAX_CUBES];

const uesb_config_t uesb_config = {
  UESB_BITRATE_250KBPS,
  UESB_CRC_8BIT,
  UESB_TX_POWER_0DBM,
  radioCannel,
  PACKET_SIZE,
  5,
  {0xE7,0xE7,0xE7,0xE7},
  {0xC2,0xC2,0xC2,0xC2},
  RADIO_ADDRS,
  0x3F,
  3
};

void Radio::init() {
  uesb_init(&uesb_config);
  uesb_start();
  
  memset(cubeRx, 0, sizeof(cubeRx));
  memset(cubeTx, 0, sizeof(cubeTx));
}

extern "C" void uesb_event_handler(void)
{
  uint32_t rf_interrupts = uesb_get_clear_interrupts();

  if(rf_interrupts & UESB_INT_RX_DR_MSK)
  {
    uesb_payload_t rx_payload;
    uesb_read_rx_payload(&rx_payload);
    uint8_t addr = rx_payload.data[sizeof(AcceleratorPacket)] - 0xC2;
    
    if (addr < MAX_CUBES) {
      memcpy((uint8_t*)&cubeRx[addr], rx_payload.data, sizeof(AcceleratorPacket));
    }

    #ifdef DO_RADIO_TESTING
    UART::print("\r\nRx %x: ..", (uint8_t)addr, uesb_config.rx_address_prefix[addr+1]);
    UART::dump(rx_payload.length, (char*)rx_payload.data);
    #endif
  }
  #ifdef RADIO_TIMING_TEST
  else
  {
    nrf_gpio_pin_clear(PIN_LED1);
  }
  #endif
}

void Radio::manage() {
  if (Head::spokenTo) {
    // Transmit to the head our cube status
    if (++g_dataToHead.cubeToUpdate >= MAX_CUBES) {
      g_dataToHead.cubeToUpdate = 0;
    }

    memcpy(&g_dataToHead.cubeStatus, &cubeRx[g_dataToHead.cubeToUpdate], sizeof(AcceleratorPacket));
    
    // Transmit to cube our line status
    if (g_dataToBody.cubeToUpdate < MAX_CUBES) {
      memcpy(&cubeTx[g_dataToBody.cubeToUpdate], &g_dataToBody.cubeStatus, sizeof(LEDPacket));
    }
  }
  
  uesb_event_handler();
  static uint8_t currentCube = 0;
  currentCube = (currentCube + 1) % TICK_LOOP;
  
  // Transmit cubes round-robin
  if (currentCube < MAX_CUBES)
  {
    #ifdef DO_RADIO_TESTING
    UART::print("\r\nTx %i %2x", currentCube, uesb_config.rx_address_prefix[currentCube+1]);
    #endif

#ifdef RADIO_TIMING_TEST
    nrf_gpio_pin_set(PIN_LED1);
    nrf_gpio_cfg_output(PIN_LED1);
    memset(cubeTx[currentCube].ledStatus, 0xFF, 12);
#endif
    
    uesb_write_tx_payload(cubePipe[currentCube], (uint8_t*)&cubeTx[currentCube], sizeof(LEDPacket));
  }
}
