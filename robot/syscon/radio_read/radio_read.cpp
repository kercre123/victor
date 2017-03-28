#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
}

#include "hardware.h"
#include "radio.h"
#include "lights.h"
#include "timer.h"
#include "messages.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Backpack {
  void update() {}
}

enum FilterTypes {
  FILTER_DISABLED,
  FILTER_ADVERTISE,
  FILTER_ALL,
  FILTER_CUBE1 = Block_LIGHTCUBE1,
  FILTER_CUBE2 = Block_LIGHTCUBE2,
  FILTER_CUBE3 = Block_LIGHTCUBE3,
  FILTER_CHARGER = Charger_Basic,
};

static FilterTypes filter_type = FILTER_DISABLED;

// Fixture pin-out
static const int PIN_TX = 17;
static const int PIN_RX = 18;
// Test thresholds
static const int ONEG = 64;                 // Cube default settings define ONE G as "64"
static const int ACCEL_THRESH = ONEG*0.22;  // Rejected if at or above 0.22G

// This value is set based on a fixture the factory built that averages around RSSI=59
static const int MAX_RSSI = 59+3;           // Smaller is closer, higher is further - add 3dB margin
static const int FAR_THRESHOLD = 5;
static unsigned int rssi[MAX_ACCESSORIES];

static uint8_t print_read_index = 0;
static uint8_t print_write_index = 0;
static uint8_t print_count = 0;
static uint8_t print_queue[0x100];

static inline unsigned int abs(int x) { return x < 0 ? -x : x; }

static bool minRSSI(unsigned int measured, int& slot) {
  int maximum = 0;
  
  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    if (rssi[i] >= maximum) {
      slot = i;
      maximum = rssi[i];
    }
  }
  
  if (measured < maximum) {
    rssi[slot] = measured;
    return true;
  }
  
  return false;
}

void set_test_mode(FilterTypes type) {
  LightState colors[4];
  memset(&colors, 0, sizeof(colors));

  for (int i = 0; i < MAX_ACCESSORIES; i++) {
    Radio::assignProp(i, 0);
    Radio::setPropLights(i, colors);
  }

  filter_type = type;
}

void uart_printf(const char* format, ...) {
  char buffer[256];
  uint8_t bytes;
  va_list args;
  va_start (args, format);
  bytes = vsprintf (buffer, format, args);
  va_end (args);

  for (int i = 0; i < bytes; i++) {
    print_queue[print_write_index++] = buffer[i];
  }

  print_count += bytes;
}

bool Anki::Cozmo::HAL::RadioSendMessage(const void *buffer, const u16 size, const u8 msgID) {
  using namespace Anki::Cozmo::RobotInterface;

  switch (msgID) {
  case RobotToEngine::Tag_activeObjectDiscovered:
    {
      const ObjectDiscovered* discovered = (ObjectDiscovered*) buffer;
      int slot;

      switch (filter_type) {
        case FILTER_ALL:
          break ;
        case FILTER_CUBE1:
        case FILTER_CUBE2:
        case FILTER_CUBE3:
        case FILTER_CHARGER:
          if (filter_type != (FilterTypes) discovered->object_type) {
            return true;
          }
          break ;
        case FILTER_DISABLED:
        case FILTER_ADVERTISE:
          return true;
      }
      
      if (abs(discovered->rssi) >= MAX_RSSI) {
        break ;
      }
      
      if (minRSSI(abs(discovered->rssi), slot)) {
        uart_printf("C%c%c%c%c", discovered->factory_id, discovered->factory_id >> 8, discovered->factory_id >> 16, discovered->factory_id >> 24);
        Radio::assignProp(slot, discovered->factory_id);
      }

      break ;
    }
  case EngineToRobot::Tag_getPropState:
    {
      const PropState* state = (PropState*) buffer;
      static int farcount[MAX_ACCESSORIES];
      LightState colors[4];
      LightState allState;

      if (abs(state->rssi) > MAX_RSSI) {
        if(++farcount[state->slot] > FAR_THRESHOLD) {
          Radio::assignProp(state->slot, 0);
          return false;
        }
      } else {
        farcount[state->slot] = 0;
      }
      
      int r = state->x, g = state->y, b = state->z;
      r = abs(r); g = abs(g); b = abs(b); 
      uint16_t color;
      
      if (!r && !g && !b) {
        color = PACK_COLORS(0, 0xFF, 0xFF, 0xFF);
      }
      else if (abs(state->x) < ACCEL_THRESH 
            && abs(state->z) < ACCEL_THRESH 
            && abs(state->y-ONEG) < ACCEL_THRESH) 
      {
        color = PACK_COLORS(0, 0x00, 0xFF, 0x00);
      }
      else {
        color = PACK_COLORS(0, 0xFF, 0x00, 0x00);
      }

      allState.onColor = color;
      allState.offColor = color;
      
      for (int i = 0; i < 4; i++) {
        memcpy(&colors[i], &allState, sizeof(LightState));
      }

      Radio::setPropLights(state->slot, colors);
      break ;
    }
  }

  return true;
}

int main(void)
{
  memset(rssi, 0xFF, sizeof(rssi));
  
  // The synthesized LFCLK requires the 16MHz HFCLK to be running, since there's no
  // external crystal/oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  
  // Wait for the external oscillator to start
  while (!NRF_CLOCK->EVENTS_HFCLKSTARTED)
    ;
  
  // Change the source for LFCLK and start it
  NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Synth;
  NRF_CLOCK->TASKS_LFCLKSTART = 1;

  // Setup UART
  NRF_UART0->POWER = 1;
  NRF_UART0->CONFIG = 0;
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  nrf_gpio_pin_set(PIN_TX);

  NRF_UART0->BAUDRATE = NRF_BAUD(115200);
  NRF_UART0->PSELRXD = PIN_RX;
  NRF_UART0->PSELTXD = PIN_TX;
  NRF_UART0->TASKS_STARTRX = 1;
  NRF_UART0->TASKS_STARTTX = 1;
  NRF_UART0->EVENTS_TXDRDY = 1;

  // Initialize our scheduler
  RTOS::init();
  Timer::init();

  // Setup all tasks
  Radio::init();
  Radio::advertise();
  Radio::setLightGamma(0x80);

  Timer::start();

  // Tell the fixture we are awake
  uart_printf("!");

  __enable_irq();

  // Run forever, because we are awesome.
  static const int32_t periods[] = { 256, 32768 };
  static const int total_periods = sizeof(periods) / sizeof(int32_t);
  static int32_t target[total_periods];
  static bool write_ready = true;
  
  memset(&target, 0, sizeof(target));
 
  for (;;) {
    int32_t cur = NRF_RTC1->COUNTER;

    if (NRF_UART0->EVENTS_RXDRDY) {
      NRF_UART0->EVENTS_RXDRDY = 0;

      switch (NRF_UART0->RXD) {
        case 'A':
          set_test_mode(FILTER_ADVERTISE);
          break ;
        case 'C':
          set_test_mode(FILTER_ALL);
          break ;
        case '0':
          set_test_mode(FILTER_CHARGER);
          break ;
        case '1':
          set_test_mode(FILTER_CUBE1);
          break ;
        case '2':
          set_test_mode(FILTER_CUBE2);
          break ;
        case '3':
          set_test_mode(FILTER_CUBE3);
          break ;
        case 'X':
          set_test_mode(FILTER_DISABLED);
          break ;
      }
    }

    if (NRF_UART0->EVENTS_TXDRDY) {
      NRF_UART0->EVENTS_TXDRDY = 0;
      
      write_ready = true;
    }

    if (write_ready && print_count > 0) {
      NRF_UART0->TXD = print_queue[print_read_index++];
      print_count --;

      write_ready = false;
    }

    for (int i = 0; i < 8; i++) {
      NRF_WDT->RR[i] = WDT_RR_RR_Reload;
    }

    // Main loop for calculating counters
    for (int i = 0; i < total_periods; i++) {
      int ticks_left = target[i] - cur;
      
      if (ticks_left > 0) continue ;
      target[i] += periods[i];

      switch (i) {
        case 0:
          Lights::manage();
          break ;
        case 2:
          if (filter_type == FILTER_ADVERTISE) {
            Radio::sendTestPacket();
          }
          break ;
      }
    }
  }
}
