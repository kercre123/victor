#include <string.h>
#include <stdio.h>

#define FCC_VERSION "c23"   // This must match fixture version, to help the factory keep in sync

#include "fcc.h"
#include "oled.h"
#include "dac.h"
#include "anki/cozmo/robot/hal.h"

#include "anki/cozmo/robot/spineData.h"

#include "hal/portable.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

// Include command constants from the nordic API
extern "C" {
  #include "syscon/components/ble/ble_dtm/ble_dtm.h"
}

using namespace Anki::Cozmo::RobotInterface;

extern GlobalDataToHead g_dataToHead;
extern GlobalDataToBody g_dataToBody;

struct LightEnable {
  int index;
  uint16_t color;
};

static const LightEnable lights[] = {
  { 0, 0x7FFF },
  { 1, 0x001F },
  { 1, 0x03E0 },
  { 1, 0x7C00 },
  { 2, 0x001F },
  { 2, 0x03E0 },
  { 2, 0x7C00 },
  { 3, 0x001F },
  { 3, 0x03E0 },
  { 3, 0x7C00 },
  { 4, 0x7FFF }
};

static int NUM_LIGHTS = sizeof(lights) / sizeof(LightEnable);

enum SecondaryTest {
  TEST_NONE,
  TEST_MOTOR,
  TEST_LEDS,
  TEST_LOW_NOISE,
  TEST_HI_NOISE
};

struct DTM_Mode_Settings {
  const char* name;
  SecondaryTest side_test;
  int32_t command;
  int32_t freq;
  int32_t payload;
  int32_t length;
};

// Our passing standard is based on low duty cycle and low power:
// At most, can send 3 full-length BLE packets on same channel per 35ms (hopping means even less)
#define POWER(i) (i & 0xfc)
static const DTM_Mode_Settings DTM_MODE[] = {
  { "LED/RX",   TEST_LEDS,      LE_RECEIVER_TEST,      2,                   0xFF,         0xFF },   // 0 = listen/rx 02
  { "BLE 2402", TEST_NONE,      LE_TRANSMITTER_TEST,   2,           DTM_PKT_0X55|POWER(0),         37*3 },   // 1..3 = tx 02,42,81 at 0dB
  { "BLE 2442", TEST_NONE,      LE_TRANSMITTER_TEST,  42,           DTM_PKT_0X55|POWER(0),         37*3 },
  { "BLE 2481", TEST_NONE,      LE_TRANSMITTER_TEST,  81,           DTM_PKT_0X55|POWER(0),         37*3 },
  { "Tone 2402",TEST_NONE,      LE_TRANSMITTER_TEST,   2, DTM_PKT_VENDORSPECIFIC|POWER(0), CARRIER_TEST },   // 4..7 = carrier 02,42,81 at 0dB
  { "Tone 2442",TEST_NONE,      LE_TRANSMITTER_TEST,  42, DTM_PKT_VENDORSPECIFIC|POWER(0), CARRIER_TEST },
  { "Tone 2481",TEST_NONE,      LE_TRANSMITTER_TEST,  81, DTM_PKT_VENDORSPECIFIC|POWER(0), CARRIER_TEST },
  { "Motor",    TEST_MOTOR,     LE_RESET,              0,                   0xFF,         0xFF },   // 7 = motor test
  { "Noise",    TEST_LOW_NOISE, LE_RESET,              0,                   0xFF,         0xFF },   // 8 = low speaker noise
  { "MAX NOISE",TEST_HI_NOISE,  LE_RESET,              0,                   0xFF,         0xFF },   // 9 = high speaker noise
#define WIFI_MODE 10
  { "11b 2412", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },   // 10..18 = wifi modes
  { "11b 2437", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
  { "11b 2462", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
  { "11g 2412", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },   
  { "11g 2437", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
  { "11g 2462", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
  { "11n 2412", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF }, 
  { "11n 2437", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
  { "11n 2462", TEST_NONE,      LE_RESET,              0,                   0xFF,         0xFF },
};

static const int DTM_MODE_COUNT  = sizeof(DTM_MODE) / sizeof(DTM_Mode_Settings);

static int iabs(int x) {
  return (x < 0) ? -x : x;
}

// This make the head select the target mode for the FCC test
static int current_mode = 0;
static int target_mode = 0;
static int target_power = -4;

static void configureTest(int mode, int forreal) {
  SendDTMCommand msg;

  if (forreal)
    current_mode = mode;
  
  msg.command = DTM_MODE[mode].command;
  msg.freq = DTM_MODE[mode].freq;
  msg.length = DTM_MODE[mode].length;
  msg.payload = DTM_MODE[mode].payload | POWER(target_power);
    
  SendMessage(msg);
}

static void runMotorTest() {
  static const int DIRECTION_CHANGE_COUNT = 200;
  static const int DIRECTION_POWER = 0x4000;
  static int direction_counter = 0;
  static bool direction;
  
  if (++direction_counter >= DIRECTION_CHANGE_COUNT) {
    direction_counter = 0;
    direction = !direction;
  }

  for (int i = 0; i < 4; i++) {
    g_dataToBody.motorPWM[i] = direction ? -DIRECTION_POWER : DIRECTION_POWER;
  }
}

static void runLEDTest() {
  // Select a color
  int color = (g_dataToHead.positions[1] >> 10) % NUM_LIGHTS;
  
  // Tell the body to change it's colors
  Anki::Cozmo::RobotInterface::BackpackLights msg;

  memset(&msg, 0, sizeof(msg));

  int idx = lights[color].index;
  uint16_t clr = lights[color].color;
  
  msg.lights[idx].onColor = clr;
  msg.lights[idx].offColor = clr;

  SendMessage(msg);
}

static void whiteNoise(uint16_t scale) {
  using namespace Anki::Cozmo::HAL;
  __disable_irq();

  static volatile uint16_t* DAC_WRITE = (volatile uint16_t*) &DAC0_DAT0L;
  uint64_t lsfr = 0xFFFFFFFFFFFFFFFFL;
  int i = 0;
  
  DAC::EnableAudio(true);
  
  for(;;) {    
    lsfr = (lsfr >> 1) ^ (lsfr & 1 ? 0xC96C5795D7870F42 : 0);
    DAC_WRITE[i++ & 0xF] = (uint16_t) lsfr & scale;
  }
}

void Anki::Cozmo::HAL::FCC::start(void) { 
  // Enable body radio mode
  SetBodyRadioMode msg;
  msg.radioMode = BODY_DTM_OPERATING_MODE;  
  SendMessage(msg);

  configureTest(0, 1);
}

// See "ESP8266 Certification and Test Guide" for details on the command format
static const char* WIFI_TESTS[] = {
  /* 0 = No Test             */       "CmdStop\r",
  /* 10 = 2412MHz  -6dB b 1Mbps */    "WifiTxout 1 1 %d 0\r",
  /* 11 = 2437MHz  -6dB b 1Mbps */    "WifiTxout 6 1 %d 0\r",
  /* 12 = 2462MHz  -6dB b 1Mbps */    "WifiTxout 11 1 %d 0\r",
  /* 13 = 2412MHz  -6dB g 6Mbps */    "WifiTxout 1 5 %d 0\r",
  /* 14 = 2437MHz  -6dB g 6Mbps */    "WifiTxout 6 5 %d 0\r",
  /* 15 = 2462MHz  -6dB g 6Mbps */    "WifiTxout 11 5 %d 0\r",
  /* 16 = 2412MHz  -6dB n 6.5Mbps */  "WifiTxout 1 13 %d 0\r",
  /* 17 = 2437MHz  -6dB n 6.5Mbps */  "WifiTxout 6 13 %d 0\r",
  /* 18 = 2462MHz  -6dB n 6.5Mbps */  "WifiTxout 11 13 %d 0\r",
};

static const int WIFI_TEST_COUNT = sizeof(WIFI_TESTS) / sizeof(WIFI_TESTS[0]);

// Returns true if we sent a command
static bool sendWifiCommand(int test) {
  static int running = 0, startdelay = 0, power = 0;
  if (test >= WIFI_TEST_COUNT)
    test = 0;
  // Don't do anything if test is already running
  if (test == running && target_power == power)
    return false;
  // Stop any running test first - and just wait to be called 5ms later
  if (running && test)
    test = 0;
  // If we're going to start a new test, we need to wait 100ms first
  if (test && ++startdelay < 20)
    return false;
  startdelay = 0;
  running = test;
  power = target_power;
  
  // All we have is a bit-bang UART with no feedback.. so don't send commands too frequently!
  char text[80];
  sprintf(text, WIFI_TESTS[test], -(power*4)); // Adjustment is in -0.25dB steps - so -6dB = 24
  char *s = text;
  const u32 DIVISOR = (32768*2560)/(115200);   // K02 weird RC clock / ESP baud rate
  
  // Send some start bits before the string
  GPIO_SET(GPIO_MISO, PIN_MISO);
  GPIO_OUT(GPIO_MISO, PIN_MISO);
  SOURCE_SETUP(GPIO_MISO, SOURCE_MISO, SourceGPIO);
  Anki::Cozmo::HAL::MicroWait(50);  
  while (*s)
  {
    u16 now, last = SysTick->VAL;
    u16 c = *s++;
    c <<= 1;      // Start bit
    c |= (3<<9);  // Stop bits (always 2)
    for (int i = 0; i < 11; i++)
    {
      // This line relies on 100MHz SysTick->LOAD in timer.cpp (despite our 84MHz actual clock)
      while (((last-(now = SysTick->VAL))&65535) < DIVISOR) // And don't forget Systick counts DOWN
        ;
      last = now;
      if (c & (1<<i))
        GPIO_SET(GPIO_MISO, PIN_MISO);
      else
        GPIO_RESET(GPIO_MISO, PIN_MISO);      
    }
  }
  return true;
}

static int configuring_motor = 0;
void Anki::Cozmo::HAL::FCC::mainDTMExecution(void)
{  
  // Duty cycle correction - transmit one packet per 7
  static u8 toggle = 0;
  if (toggle == 6) {
    configureTest(current_mode, 1);
    toggle = 0;
  } else {
    configureTest(0, 0);    // Listen the rest of the time
    toggle++;
  }

  // Flash headlight - because we do that for compliance testing, too
  static Anki::Cozmo::RobotInterface::SetHeadlight headlight;
  headlight.enable = !headlight.enable;
  SendMessage(headlight);

  // If we sent a wifi command, no time for running tests - just return
  if (sendWifiCommand((current_mode < WIFI_MODE) ? 0 : current_mode-(WIFI_MODE-1)))  // Wifi test modes start at 10
    return;
  
  // Run side tests
  switch(DTM_MODE[current_mode].side_test) {
    case TEST_MOTOR:
      runMotorTest();
      break ;
    case TEST_LEDS:
      runLEDTest();
      break ;
    case TEST_LOW_NOISE:
      whiteNoise(0x7FF);
      break ;
    case TEST_HI_NOISE:
      whiteNoise(0xFFF);
      break ;
    default:
      break ;
  }
  
  // Display current mode and what we're going to test
  static int textidx = 0, textlen = 0, countdown = 0, wait = 0, x = 0, y = 0;
  static char text[80] = {0};
  
  // Target mode is set based on the position of the left tread
  int targ = (unsigned int)(g_dataToHead.positions[1] >> 9) % DTM_MODE_COUNT;
  static const char* COUNTDOWN[] = {"RUNNING", ".....", "....", "...", "..", "."};
  
  // Power is set based on the position of the right tread
  static int lastpower = 0;
  int righttread = g_dataToHead.positions[0] >> 10;
  if (lastpower != righttread)
  {
    lastpower = righttread;
    target_power += (target_mode < WIFI_MODE) ? -4 : -1;
    textlen = 0;
    wait = 0;
    countdown = 6;
  }
    
  // If motors aren't going crazy, update the target mode
  if (DTM_MODE[current_mode].side_test != TEST_MOTOR)
    if (target_mode != targ)
    {
      target_mode = targ;
      target_power = (target_mode < WIFI_MODE) ? -4 : -6;
      textlen = 0;
      wait = 0;
      countdown = 6;
    }

  // If nothing left to print, go print it again.. and again.. keep busy!
  if (0 == textlen)
  {
    if (wait--)
      return;                           // Just wait to refresh - can't safely continue
    else if (countdown)
      countdown--;                      // Continue countdown to start
    else
      current_mode = target_mode;       // Start the test
    wait = 50;            // About half a second including print time
    
    textidx = x = y = 0;  // Print start of message at top left of screen
    textlen=sprintf(text, "\n\n %02d %-9s  %3ddB\n\n    %7s   " FCC_VERSION, 
      target_mode, DTM_MODE[target_mode].name, target_power, COUNTDOWN[countdown]);
  }

  // Print one character from the message
  if (text[textidx] == '\n')
  {
    x = 0;
    y += 1;
  } else {
    OLED::DisplayDigit(x, y, text[textidx]);
    x += 6;
  }
  textidx++;
  textlen--;
}
