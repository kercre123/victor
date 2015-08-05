#include <stdint.h>
#include <string.h>

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

// Blinky status lights!
#include "anki/cozmo/robot/spineData.h"

#define PACKET_SIZE 13
#define TIME_PER_CUBE (int)(COUNT_PER_MS * 33.0 / MAX_CUBES)
#define TICK_LOOP   7

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

#ifndef BACKPACK_DEMO
const uint8_t     cubePipe[] = {1,2,3,4};
#else
const uint8_t     cubePipe[] = {1};
#endif

#define MAX_CUBES sizeof(cubePipe)

LEDPacket         cubeTx[MAX_CUBES];
AcceleratorPacket cubeRx[MAX_CUBES];

void Radio::init() {
  uesb_config_t uesb_config = {
    UESB_BITRATE_250KBPS,
    UESB_CRC_8BIT,
    UESB_TX_POWER_0DBM,
    82,
    PACKET_SIZE,
    5,
    {0xE7,0xE7,0xE7,0xE7},
    {0xC2,0xC2,0xC2,0xC2},
#ifndef BACKPACK_DEMO
    {0xE7,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8},
#else
    {0xE7,0xC0,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8},
#endif
    0x3F,
    3
  };

  uesb_init(&uesb_config);
  uesb_start();
  
  memset(cubeRx, 0, sizeof(cubeRx));
  #if defined(NATHAN_WANTS_DEMO) || defined(BACKPACK_DEMO)
  for (int g = 0; g < MAX_CUBES; g++) {
    cubeTx[g].ledStatus[2] = 0xFF;
    //memset(cubeTx[g].ledStatus, 0xFF, 12);
    cubeTx[g].ledDark = 0;
  }
  #else
  memset(cubeTx, 0, sizeof(cubeTx));
  #endif
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

    #ifdef DEBUG_MESSAGES
    UART::put("\r\nRx");
    UART::hex((uint8_t)addr);
    UART::put(":");
    UART::dump((uint8_t*)rx_payload.data, rx_payload.length);
    #endif
  }
}

#include "nrf_gpio.h"
#include "hardware.h"

static int lastC = GetCounter();

void BenchMark() {
    static int lastC = GetCounter();
    int currentC = GetCounter();
    UART::print("%i\n\r", (int)((currentC - lastC) / 256.0f / 32768.0f * 1000));
    lastC = currentC;
}

extern "C" void BlinkPack(int toggle) {
  #if defined(BACKPACK_DEMO)
  nrf_gpio_pin_set(PIN_LED2);
  nrf_gpio_cfg_output(PIN_LED2);

  if (toggle) {
    nrf_gpio_cfg_input(PIN_LED1, NRF_GPIO_PIN_NOPULL);
  } else {
    nrf_gpio_pin_clear(PIN_LED1);
    nrf_gpio_cfg_output(PIN_LED1);
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
      #if !defined(BACKPACK_DEMO) && !defined(NATHAN_WANTS_DEMO)
      memcpy(&cubeTx[g_dataToBody.cubeToUpdate], &g_dataToBody.cubeStatus, sizeof(LEDPacket));
      #endif
    }
  }
  
  uesb_event_handler();
  static uint8_t currentCube = 0;
  currentCube = (currentCube + 1) % TICK_LOOP;
  
  // Transmit cubes round-robin
  if (currentCube < MAX_CUBES)
  {
    #ifdef DEBUG_MESSAGES
    UART::put("\r\nTx");
    UART::dec(currentCube);
    #endif
    
    uesb_write_tx_payload(cubePipe[currentCube], (uint8_t*)&cubeTx[currentCube], sizeof(LEDPacket));
  }
}
