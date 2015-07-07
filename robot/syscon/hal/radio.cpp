#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf51_bitfields.h"

extern "C" {
  #include "micro_esb.h"
}
  
#include "uart.h"
#include "radio.h"
#include "timer.h"
#include "head.h"

// Blinky status lights!
#include "anki/cozmo/robot/spineData.h"

#define PACKET_SIZE 13
#define TIME_PER_CUBE (int)(COUNT_PER_MS * 33.0 / MAX_CUBES)
//#define NATHAN_WANTS_DEMO
//#define DEBUG_MESSAGES

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

const uint8_t     cubePipe[] = {2,1,3,4};

#define MAX_CUBES sizeof(cubePipe)

LEDPacket         cubeTx[MAX_CUBES];
AcceleratorPacket cubeRx[MAX_CUBES];

void Radio::init() {
  uesb_config_t uesb_config = {
    UESB_BITRATE_1MBPS,
    UESB_CRC_8BIT,
    UESB_TX_POWER_0DBM,
    2,
    PACKET_SIZE,
    5,
    {0xE7,0xE7,0xE7,0xE7},
    {0xC2,0xC2,0xC2,0xC2},
    {0xE7,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8},
    0x3F,
    3
  };

  uesb_init(&uesb_config);
  uesb_start();
  
  memset(cubeRx, 0, sizeof(cubeRx));
  #ifdef NATHAN_WANTS_DEMO
  for (int g = 0; g < MAX_CUBES; g++) {
    memset(cubeTx[g].ledStatus, 0xFF, sizeof(cubeTx[g].ledStatus));
    cubeTx[g].ledDark = 0;
  }
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

void Radio::manage() {
  if (Head::spokenTo) {
    // Transmit to the head our cube status
    if (++g_dataToHead.cubeToUpdate >= MAX_CUBES) {
      g_dataToHead.cubeToUpdate = 0;
    }

    memcpy(&g_dataToHead.cubeStatus, &cubeRx[g_dataToHead.cubeToUpdate], sizeof(AcceleratorPacket));
    
    // Transmit to cube our line status
    if (g_dataToBody.cubeToUpdate < MAX_CUBES) {
      #ifndef NATHAN_WANTS_DEMO
      memcpy(&cubeTx[g_dataToBody.cubeToUpdate], &g_dataToBody.cubeStatus, sizeof(LEDPacket));
      #endif
    }
  }

  uesb_event_handler();
  static uint8_t currentCube = 0;
  currentCube = (currentCube + 1) % 7;

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
