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
  BOARD_REV_UNKNOWN   = 0,
  BOARD_REV_1_0_REV1  = 1,  //Fixture release 1.0 rev1
  BOARD_REV_1_0_REV2  = 2,  //Fixture release 1.0 rev2
  BOARD_REV_1_0_REV3  = 3,  //Fixture release 1.0 rev3
  BOARD_REV_1_5_1     = 4,  //Fixture release 1.5.1
} board_rev_t;

#ifdef __cplusplus

//channel args for Board::getAdcMv()
enum adc_chan_e {
  //ADC_MCUTX       = ADC_Channel_0,  //PA0
  //ADC_MCURX       = ADC_Channel_1,  //PA1
    ADC_DUT_RX      = ADC_Channel_2,  //PA2   XXX: Fix 1.5 D1_BP4, GrabADC(), BP LED testing
    ADC_DUT_TX      = ADC_Channel_3,  //PA3   XXX: Fix 1.5 D3_BP3, GrabADC(), BP LED testing
    ADC_DUT_CS      = ADC_Channel_4,  //PA4
    ADC_DUT_SCK     = ADC_Channel_5,  //PA5
    ADC_DUT_MISO    = ADC_Channel_6,  //PA6   XXX: Fix 1.5 D2_BP2, GrabADC(), BP LED testing
    ADC_DUT_MOSI    = ADC_Channel_7,  //PA7   XXX: Fix 1.5 D4_BP1, GrabADC(), BP LED testing
  //ADC_DUT_VDD     = ADC_Channel_8,  //PB0
    ADC_DUT_VDD_IN  = ADC_Channel_9,  //PB1
  //ADC_OLED_CS     = ADC_Channel_10, //PC0
  //ADC_ENBAT_LC    = ADC_Channel_11, //PC1
  //ADC_NBAT        = ADC_Channel_12, //PC2
    ADC_DUT_ADC2    = ADC_Channel_13, //PC3   XXX: Fix 1.5 ADC_ENCA, QuickADC(), ReadEncoder()
    ADC_DUT_ADC3    = ADC_Channel_14, //PC4   XXX: Fix 1.5 ADC_ENCB, QuickADC(), ReadEncoder()
  //ADC_PC4_10K     = ADC_Channel_14,
    ADC_DUT_RESET   = ADC_Channel_15, //PC5
};

namespace Board
{
  enum led_e {
    LED_RED   = 0,
    LED_GREEN = 1
  };
  
  enum btn_e {
    BTN_1 = 1,
    BTN_2 = 2,
    BTN_NUM = 2, //number of buttons
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
  
  void enableVBAT();
  void disableVBAT();
  bool VBATisEnabled();
  
  void enableDUTPROG(int turn_on_delay_ms = 2);   //Wait 'turn_on_delay_ms' for voltage to stabilize (delay ignored if power already on)
  void disableDUTPROG(int turn_off_delay_ms = 50, bool force = 1); //Wait 'turn_off_delay_ms' for power to drain out (delay ignored if power already off). if force=1, drive DUT_PROG low to discharge quickly.
  bool DUTPROGisEnabled();
  
  void buzz(u8 f_kHz, u16 duration_ms); //piezo buzzer beep. f={1..20}kHz
  void buzzerOn();  //buzzer output static on
  void buzzerOff(); //buzzer output static off
  
  //read ADC[mV]. 2^oversample = # samples (averaged). pin_chg_us = charge delay pin.cfg->sample (-1 skips pin cfg/restore)
  uint32_t getAdcMv(adc_chan_e chan, int oversample = 4, int pin_chg_us = 20);
  
  //DEPRECIATED - USE 'Contacts' API
  void enableVEXT();
  void disableVEXT();
  bool VEXTisEnabled();
  void VEXTon(int turn_on_delay_ms = 0);
  void VEXToff(int turn_off_delay_ms = 0);
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
namespace NRF_SWD           GPIO_DEFINE(A, 11);
namespace NRF_SWC           GPIO_DEFINE(A, 12);
namespace SWD               GPIO_DEFINE(A, 13);
namespace SWC               GPIO_DEFINE(A, 14);
namespace ENCHG             GPIO_DEFINE(A, 15);

//Port B
namespace DUT_VDD           GPIO_DEFINE(B, 0);
namespace DUT_VDD_SENSE     GPIO_DEFINE(B, 1);
namespace BOOT1             GPIO_DEFINE(B, 2);
//namespace OLED_SCK        GPIO_DEFINE(B, 3);  //XXX: Fix 1.5 obsolete
//namespace OLED_RESN       GPIO_DEFINE(B, 4);  //XXX: Fix 1.5 obsolete
//namespace OLED_MOSI       GPIO_DEFINE(B, 5);  //XXX: Fix 1.5 obsolete
namespace BTN1              GPIO_DEFINE(B, 3);
namespace XXX_0_OLED_RESN   GPIO_DEFINE(B, 4);
namespace BTN2              GPIO_DEFINE(B, 5);
namespace TX                GPIO_DEFINE(B, 6);
//namespace OLED_DCN        GPIO_DEFINE(B, 7);  //XXX: Fix 1.5 obsolete
namespace SCL               GPIO_DEFINE(B, 8);
namespace SDA               GPIO_DEFINE(B, 9);
namespace DUT_SWD           GPIO_DEFINE(B, 10);
namespace DUT_SWC           GPIO_DEFINE(B, 11);
namespace PB12_IN1          GPIO_DEFINE(B, 12);
namespace PB13_EN1          GPIO_DEFINE(B, 13);
namespace PB14_IN2          GPIO_DEFINE(B, 14);
namespace PB15_EN2          GPIO_DEFINE(B, 15);

//Port C
//namespace OLED_CSN        GPIO_DEFINE(C, 0);  //XXX: Fix 1.5 obsolete
namespace XXX_1_OLED_CSN    GPIO_DEFINE(C, 0);
namespace ENBAT_LC          GPIO_DEFINE(C, 1);
namespace NBAT              GPIO_DEFINE(C, 2);
namespace DUT_ADC2          GPIO_DEFINE(C, 3);
namespace PC4_10K           GPIO_DEFINE(C, 4);
namespace DUT_ADC3          GPIO_DEFINE(C, 4); //alias
namespace DUT_RESET         GPIO_DEFINE(C, 5);
namespace CHGTX             GPIO_DEFINE(C, 6);
namespace CHGRX             GPIO_DEFINE(C, 7);
namespace LEDPIN_RED        GPIO_DEFINE(C, 8);
namespace LEDPIN_GRN        GPIO_DEFINE(C, 9);
namespace NRF_RX            GPIO_DEFINE(C, 10);
namespace NRF_TX            GPIO_DEFINE(C, 11);
namespace DUT_TRX           GPIO_DEFINE(C, 12);
namespace PC13_BRD_REV0     GPIO_DEFINE(C, 13);
namespace PC14_BRD_REV1     GPIO_DEFINE(C, 14);
namespace BZZ               GPIO_DEFINE(C, 15);

//Port D
namespace NBATSINK          GPIO_DEFINE(D, 2);

#endif /* __cplusplus */

//-----------------------------------------------------------
//        [LEGACY] pin defines -- DEPRECIATED
//-----------------------------------------------------------

#define PINC_CHGTX           6
#define PINC_CHGRX           7
#define PINB_SCL               8
#define PINB_SDA               9

#define GPIOC_CHGTX          (1 << PINC_CHGTX)
#define GPIOC_CHGRX          (1 << PINC_CHGRX)
#define GPIOB_SCL         (1 << PINB_SCL)
#define GPIOB_SDA         (1 << PINB_SDA)

#define PINB_VDD   0
#define ADC_VDD    8

#define PINC_RESET 5
#define ADC_RESET 15

#define PINC_TRX 12
#define GPIOC_TRX (1 << PINC_TRX)

#define PINA_ENCHG 15
#define GPIOA_ENCHG (1 << PINA_ENCHG)

#define PINB_SWD  10
#define GPIOB_SWD (1 << PINB_SWD)
#define PINB_SWC  11
#define GPIOB_SWC (1 << PINB_SWC)

#define PINB_MOTDRV_IN1   12
#define PINB_MOTDRV_EN1   13
#define PINB_MOTDRV_IN2   14
#define PINB_MOTDRV_EN2   15
#define GPIOB_MOTDRV_IN1  (1 << PINB_MOTDRV_IN1)
#define GPIOB_MOTDRV_EN1  (1 << PINB_MOTDRV_EN1)
#define GPIOB_MOTDRV_IN2  (1 << PINB_MOTDRV_IN2)
#define GPIOB_MOTDRV_EN2  (1 << PINB_MOTDRV_EN2)

#define PINC_BOARD_ID0    13
#define PINC_BOARD_ID1    14
#define GPIOC_BOARD_ID0   (1 << PINC_BOARD_ID0)
#define GPIOC_BOARD_ID1   (1 << PINC_BOARD_ID1)

#define PINC_BUZZER       15
#define GPIOC_BUZZER      (1 << PINC_BUZZER)

#define PINA_NRF_SWD  11
#define GPIOA_NRF_SWD (1 << PINA_NRF_SWD)
#define PINA_NRF_SWC  12
#define GPIOA_NRF_SWC (1 << PINA_NRF_SWC)

#define PINC_NRF_RX  10
#define GPIOC_NRF_RX (1 << PINC_NRF_RX)
#define PINC_NRF_TX  11
#define GPIOC_NRF_TX (1 << PINC_NRF_TX)

#define PINB_DEBUGTX 6
#define GPIOB_DEBUGTX (1 << PINB_DEBUGTX)

#define PINA_DUTCS 4
#define PINA_SCK 5
#define PINA_MISO 6
#define PINA_MOSI 7
#define PINA_PROGHV 9
#define PINC_RESET 5

// Backpack LEDs/ADC channels
#define PINA_BPLED0 2
#define PINA_BPLED1 3
#define PINA_BPLED2 6
#define PINA_BPLED3 7

// Quadrature encoder IOs/ADC channels
#define PINA_ENCLED 4
#define PINC_ENCA   3
#define ADC_ENCA    13
#define PINC_ENCB   4
#define ADC_ENCB    14


#endif //__BOARD_H

