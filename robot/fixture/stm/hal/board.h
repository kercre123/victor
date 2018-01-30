// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#ifndef __BOARD_H
#define __BOARD_H

#include <stdint.h>
#include "common.h"
#include "stm32f2xx.h"

//-----------------------------------------------------------
//        Board Config
//-----------------------------------------------------------
static const uint32_t SYSTEM_CLOCK = 120*1000000;
static const uint32_t COMMS_BAUDRATE = 1000000;

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

//Fixture Hardware revisions
typedef enum {
  BOARD_REV_INVALID   = -1,
  BOARD_REV_1_0       = 0,  //Victor Fixture v1.0 11/10/2017
  BOARD_REV_2_0       = 1,  //Victor Fixture v2.0 1/5/2018
  BOARD_REV_FUTURE,         //detected rev is newer than this firmware recognizes
} board_rev_t;

#ifdef __cplusplus

//channel args for Board::getAdcMv()
enum adc_chan_e {
  //ADC_MCUTX       = ADC_Channel_0,  //PA0
  //ADC_MCURX       = ADC_Channel_1,  //PA1
    ADC_DUT_RX      = ADC_Channel_2,  //PA2   XXX: Cozmo Fix 1.5 D1_BP4, GrabADC(), BP LED testing
    ADC_DUT_TX      = ADC_Channel_3,  //PA3   XXX: Cozmo Fix 1.5 D3_BP3, GrabADC(), BP LED testing
    ADC_DUT_CS      = ADC_Channel_4,  //PA4
    ADC_DUT_SCK     = ADC_Channel_5,  //PA5
    ADC_DUT_MISO    = ADC_Channel_6,  //PA6   XXX: Cozmo Fix 1.5 D2_BP2, GrabADC(), BP LED testing
    ADC_DUT_MOSI    = ADC_Channel_7,  //PA7   XXX: Cozmo Fix 1.5 D4_BP1, GrabADC(), BP LED testing
  //ADC_DUT_VDD     = ADC_Channel_8,  //PB0
    ADC_DUT_VDD_IN  = ADC_Channel_9,  //PB1
    ADC_GPIO_PC0    = ADC_Channel_10, //PC0
  //ADC_ENBAT_LC    = ADC_Channel_11, //PC1
  //ADC_NBAT        = ADC_Channel_12, //PC2
    ADC_DUT_ADC2    = ADC_Channel_13, //PC3   XXX: Cozmo Fix 1.5 ADC_ENCA, QuickADC(), ReadEncoder()
    ADC_DUT_ADC3    = ADC_Channel_14, //PC4   XXX: Cozmo Fix 1.5 ADC_ENCB, QuickADC(), ReadEncoder()
    ADC_DUT_RESET   = ADC_Channel_15, //PC5
};

//power nets on the board. parameterized selection used by multiple APIs
enum pwr_e {
  PWR_VEXT    = 0, //on/off use Contacts innterface
  PWR_VBAT    = 1,
  PWR_CUBEBAT = 2,
  PWR_DUTPROG = 3,
  PWR_DUTVDD  = 4,
  PWR_UAMP    = 5,
};

namespace Board
{
  enum led_e {
    LED_RED   = 0,
    LED_GREEN = 1,
    LED_YLW   = 2,
    LED_NUM     = 3, //number of leds
  };
  
  enum btn_e {
    BTN_1 = 1,
    BTN_2 = 2,
    BTN_3 = 3,
    BTN_4 = 4,
    BTN_NUM = 4, //number of buttons
  };
    
  void init();
  board_rev_t revision(); //get the board revision
  char* revString(void); //get descriptive revision string
  
  void ledOn(led_e led);
  void ledOff(led_e led);
  void ledToggle(led_e led);
  
  bool btnPressed(btn_e btn, int oversample = 6); //true if button is pressed. 2^oversample for average/glitch filter
  
  //poll for debounce and edge detect. Includes hysteresis, min edge time is sample period * sample limit
  //sample period/limit determine filtering heuristics. < 0 for either param resets the state machine.
  //@return -1 released (edge), 1 pressed (edge), 0 no change
  int btnEdgeDetect(btn_e btn, int sample_period_us = 5000, int sample_limit = 10);
  
  void powerOn(pwr_e net, int turn_on_delay_ms = 2); //Wait 'turn_on_delay_ms' for voltage to stabilize (delay ignored if power already on)
  void powerOff(pwr_e net, int turn_off_delay_ms = 50, bool force = 1); //Wait 'turn_off_delay_ms' for power to drain out (delay ignored if power already off). if force=1, drive available current sinks (if any) to discharge quickly.
  bool powerIsOn(pwr_e net);
  char* power2str(pwr_e net); //enum to string
  
  void buzz(u8 f_kHz, u16 duration_ms); //piezo buzzer beep. f={1..20}kHz
  void buzzerOn();  //buzzer output static on
  void buzzerOff(); //buzzer output static off
  
  //read ADC[mV]. 2^oversample = # samples (averaged). pin_chg_us = charge delay pin.cfg->sample (-1 skips pin cfg/restore)
  uint32_t getAdcMv(adc_chan_e chan, int oversample = 4, int pin_chg_us = 20);
}
#endif /* __cplusplus */

//-----------------------------------------------------------
//        pin defines
//-----------------------------------------------------------
#ifdef __cplusplus

//Port A
namespace MCUTX             GPIO_DEFINE(A, 0);
namespace MCURX             GPIO_DEFINE(A, 1);
namespace DUT_RX            GPIO_DEFINE(A, 2);
namespace DUT_TX            GPIO_DEFINE(A, 3);
namespace DUT_CS            GPIO_DEFINE(A, 4);
namespace DUT_SCK           GPIO_DEFINE(A, 5);
namespace DUT_MISO          GPIO_DEFINE(A, 6);
namespace DUT_MOSI          GPIO_DEFINE(A, 7);
namespace PROG_LV           GPIO_DEFINE(A, 8);
namespace PROG_HV           GPIO_DEFINE(A, 9);
namespace UAMP              GPIO_DEFINE(A, 10);
namespace PB4               GPIO_DEFINE(A, 11);
namespace PB3               GPIO_DEFINE(A, 12);
namespace SWD               GPIO_DEFINE(A, 13);
namespace SWC               GPIO_DEFINE(A, 14);
namespace ENCHG             GPIO_DEFINE(A, 15);

//Port B
namespace DUT_VDD           GPIO_DEFINE(B, 0);
namespace DUT_VDD_SENSE     GPIO_DEFINE(B, 1);
namespace BOOT1             GPIO_DEFINE(B, 2);
namespace PB2               GPIO_DEFINE(B, 3);
namespace PB1               GPIO_DEFINE(B, 4);
namespace BZZ               GPIO_DEFINE(B, 5);
namespace TX                GPIO_DEFINE(B, 6);
namespace ENCHG_CUBE        GPIO_DEFINE(B, 7);

namespace SCL               GPIO_DEFINE(B, 8);
namespace SDA               GPIO_DEFINE(B, 9);
namespace DUT_SWD           GPIO_DEFINE(B, 10);
namespace DUT_SWC           GPIO_DEFINE(B, 11);
namespace MOTDRV_IN1        GPIO_DEFINE(B, 12);
namespace MOTDRV_EN1        GPIO_DEFINE(B, 13);
namespace MOTDRV_IN2        GPIO_DEFINE(B, 14);
namespace MOTDRV_EN2        GPIO_DEFINE(B, 15);

//Port C
namespace CHGTX_EN          GPIO_DEFINE(C, 0);
namespace ENBAT_LC          GPIO_DEFINE(C, 1);
namespace NBAT              GPIO_DEFINE(C, 2);
namespace DUT_ADC2          GPIO_DEFINE(C, 3);
namespace DUT_ADC3          GPIO_DEFINE(C, 4);
namespace DUT_RESET         GPIO_DEFINE(C, 5);
namespace CHGTX             GPIO_DEFINE(C, 6);
namespace CHGRX             GPIO_DEFINE(C, 7);
namespace LEDPIN_RED        GPIO_DEFINE(C, 8);
namespace LEDPIN_GRN        GPIO_DEFINE(C, 9);
namespace LEDPIN_YLW        GPIO_DEFINE(C, 10);
namespace CHGPWR            GPIO_DEFINE(C, 11);
namespace DUT_TRX           GPIO_DEFINE(C, 12);
namespace PC13_BRD_REV0     GPIO_DEFINE(C, 13);
namespace PC14_BRD_REV1     GPIO_DEFINE(C, 14);
namespace PC15_BRD_REV2     GPIO_DEFINE(C, 15);

//Port D
namespace NBATSINK          GPIO_DEFINE(D, 2);

#endif /* __cplusplus */

#endif //__BOARD_H

