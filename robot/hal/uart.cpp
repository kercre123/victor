#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const u32 BAUDRATE = 1000000;
      const u32 GPIO_PIN = 69; // 4;
      const u32 GPIO_MODE = D_GPIO_MODE_1; // D_GPIO_MODE_5;

      void UARTInit()
      {
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_UART);

        u32 mask =  (1 << 31) |  // FIFO write enable
                    (1 << 12) |  // Output enable
                    (1 << 1);    // TX enable
        u32 scaler = (DrvCprGetSysClockKhz() * 1000) / (BAUDRATE << 3);
        SET_REG_WORD(UART_CTRL_ADR, mask);
        SET_REG_WORD(UART_SCALER_ADR, scaler);
        DrvGpioMode(GPIO_PIN, GPIO_MODE);
      }

      int UARTPutChar(int c)
      {
        if (c == '\n')
        {
          UARTPutChar('\r');
        }

        // Wait for TX FIFO to not be full
        while (GET_REG_WORD_VAL(UART_STATUS_ADR) & 0x200)
          ;

        SET_REG_WORD(UART_DATA_ADR, c);

        return c;
      }
    }
  }
}

