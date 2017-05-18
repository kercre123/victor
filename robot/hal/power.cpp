#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "MK02F12810.h"

#include "hardware.h"
#include "watchdog.h"
#include "power.h"
#include "uart.h"
#include "anki/cozmo/robot/buildTypes.h"

#include "uart.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // This powers up the camera, OLED, and ESP8266 while respecting power sequencing rules
      namespace Power
      {
        static void oldPowerSequencing(void)
        {
          // Pull-up MISO during ESP8266 boot
          GPIO_IN(MISO);
          SOURCE_SETUP(MISO, SourceGPIO | SourcePullUp);

          // Pull-down SCK during ESP8266 boot
          GPIO_RESET(SCK);
          GPIO_OUT(SCK);    // XXX: Driving SCK low here is bad for the ESP, why not pulldown?
          SOURCE_SETUP(SCK, SourceGPIO);

          // Weakly pull MOSI high to put ESP8266 into flash boot mode
          // We rely on the fixture to pull this strongly low and enter bootloader mode
          GPIO_IN(MOSI);
          SOURCE_SETUP(MOSI, SourceGPIO | SourcePullUp);

          // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
          GPIO_IN(WS);
          SOURCE_SETUP(WS, SourceGPIO | SourcePullUp);

          Anki::Cozmo::HAL::MicroWait(10000);

          // Turn on 2v8 and 3v3 rails
          GPIO_SET(POWEREN);
          GPIO_OUT(POWEREN);
          SOURCE_SETUP(POWEREN, SourceGPIO);

          #ifndef ENABLE_FCC_TEST
          // Wait for Espressif to toggle out 4 words of I2SPI
          for (int i = 0; i < 32 * 512; i++)
          {
            while (GPIO_READ(WS))     ;
            while (!GPIO_READ(WS))     ;
          }

          // Switch to 10MHz Espressif/external reference and 100MHz clock
          MCG_C1 &= ~MCG_C1_IREFS_MASK;
          // Wait for IREF to turn off
          while((MCG->S & MCG_S_IREFST_MASK))   ;
          // Wait for FLL to lock
          while((MCG->S & MCG_S_CLKST_MASK))    ;

          MicroWait(100);     // Because of erratum e7735: Wait 2 IRC cycles (or 2/32.768KHz)

          GPIO_IN(SCK);   // XXX: Shouldn't we turn around SCK sooner? Are we driving against each other?
          #endif

          SOURCE_SETUP(MISO, SourceGPIO);

          MicroWait(10000);

          // Power enable the OLED
          GPIO_OUT(OLED_RESET_N);
          PORTA_PCR19  = PORT_PCR_MUX(1);

          MicroWait(80);
          GPIO_RESET(OLED_RESET_N);
          MicroWait(80);
          GPIO_SET(OLED_RESET_N);

          MicroWait(10000);

          // Camera sequencing
          GPIO_SET(CAM_PWDN);
          GPIO_OUT(CAM_PWDN);
          SOURCE_SETUP(CAM_PWDN, SourceGPIO);

          GPIO_RESET(CAM_RESET_N);
          GPIO_OUT(CAM_RESET_N);
          SOURCE_SETUP(CAM_RESET_N, SourceGPIO);

          MicroWait(50);
          GPIO_RESET(CAM_PWDN);
          MicroWait(50);
          GPIO_SET(CAM_RESET_N);
        }

        void enableExternal(void)
        {
          if (*HW_VERSION != 0x01050000) {
            oldPowerSequencing();
            return ;
          }

          // Setup initial state for power on sequencing
          // Initial state of the machine (Powered down, disabled)
          GPIO_RESET(POWEREN);
          GPIO_SET(CAM_PWDN);
          GPIO_RESET(CAM_RESET_N);
          GPIO_RESET(OLED_RESET_N);
          
          // Configure pins as outputs
          GPIO_OUT(POWEREN);
          SOURCE_SETUP(POWEREN, SourceGPIO);
          GPIO_OUT(CAM_PWDN);
          SOURCE_SETUP(CAM_PWDN, SourceGPIO);
          GPIO_OUT(CAM_RESET_N);
          SOURCE_SETUP(CAM_RESET_N, SourceGPIO);
          GPIO_OUT(OLED_RESET_N);
          SOURCE_SETUP(OLED_RESET_N, SourceGPIO);
          MicroWait(100000);
          
         

          // Pull-up MISO during ESP8266 boot
          GPIO_IN(MISO);
          SOURCE_SETUP(MISO, SourceGPIO | SourcePullUp);

          // Pull-down SCK during ESP8266 boot
          GPIO_RESET(SCK);
          GPIO_OUT(SCK);    // XXX: Driving SCK low here is bad for the ESP, why not pulldown?
          SOURCE_SETUP(SCK, SourceGPIO);

          // Weakly pull MOSI high to put ESP8266 into flash boot mode
          // We rely on the fixture to pull this strongly low and enter bootloader mode
          GPIO_IN(MOSI);
          SOURCE_SETUP(MOSI, SourceGPIO | SourcePullUp);

          // Pull WS high to set correct boot mode on Espressif GPIO2 (flash or bootloader)
          GPIO_IN(WS);
          SOURCE_SETUP(WS, SourceGPIO | SourcePullUp);

          Anki::Cozmo::HAL::MicroWait(10000);

          // Power-up Sequence
          GPIO_SET(OLED_RESET_N);
          MicroWait(3000);

          GPIO_SET(POWEREN);
          MicroWait(1000);
          
          GPIO_RESET(CAM_PWDN);
          MicroWait(1000);

          GPIO_SET(CAM_RESET_N);
          MicroWait(1000);

          #ifndef FCC_TEST
          // Wait for Espressif to toggle out 4 words of I2SPI
          for (int i = 0; i < 32 * 512; i++)
          {
            while (GPIO_READ(WS))  ;
            while (!GPIO_READ(WS)) ;
          }

          // Switch to 10MHz Espressif/external reference and 100MHz clock
          MCG_C1 &= ~MCG_C1_IREFS_MASK;
          // Wait for IREF to turn off
          while((MCG->S & MCG_S_IREFST_MASK))   ;
          // Wait for FLL to lock
          while((MCG->S & MCG_S_CLKST_MASK))    ;

          MicroWait(100);     // Because of erratum e7735: Wait 2 IRC cycles (or 2/32.768KHz)

          GPIO_IN(SCK);   // XXX: Shouldn't we turn around SCK sooner? Are we driving against each other?
          #endif

          SOURCE_SETUP(MISO, SourceGPIO);
          
        }

        void enterSleepMode(void)
        {
          UART::DebugPutc('B');
          __disable_irq();
          Watchdog::suspend();

          // Don't go into low power mode for approximately 20ms, giving
          // the body time to acknowledge that the head went away
          MicroWait(20000);

          // TODO: DISABLE IMU
          // TODO: DISABLE OLED
          // TODO: DISABLE CAMERA

          // NOTE: WE SHOULDN'T BE RIPPING POWER AWAY
          GPIO_RESET(POWEREN);

          // Turn off all perfs except GPIO
          SIM_SCGC6 &= ~(SIM_SCGC6_FTM1_MASK | SIM_SCGC6_FTM2_MASK | SIM_SCGC6_DMAMUX_MASK | SIM_SCGC6_DAC0_MASK | SIM_SCGC6_PDB_MASK | SIM_SCGC6_SPI0_MASK);
          SIM_SCGC7 &= ~SIM_SCGC7_DMA_MASK;
          SIM_SCGC4 &= ~(SIM_SCGC4_UART1_MASK | SIM_SCGC4_UART0_MASK | SIM_SCGC4_I2C0_MASK);

          // Go into low frequency oscillator mode
          MCG_C2 &= ~MCG_C2_IRCS_MASK; // Internal reference clock is slow (32khz)
          MCG_C1 = (MCG_C1 & MCG_C1_CLKS_MASK) | MCG_C1_CLKS(1);

          // Treat the receive pin as a gpio (look for handshake)
          GPIO_RESET(BODY_UART_TX);
          GPIO_OUT(BODY_UART_TX);
          SOURCE_SETUP(BODY_UART_TX, SourceGPIO);

          GPIO_IN(BODY_UART_RX);
          SOURCE_SETUP(BODY_UART_RX, SourceGPIO);

          NVIC_SystemReset();
        }
      }
    }
  }
}
