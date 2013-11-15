#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const u32 BAUDRATE = 1000000;
      const u8 TX_PIN = 69;
      const u32 TX_MODE = D_GPIO_MODE_1;
      const u8 RX_PIN = 70;
      const u32 RX_MODE = D_GPIO_MODE_1;

      void UARTInit()
      {
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_UART);

        u32 mask =  (1 << 31) |  // FIFO write enable
                    (1 << 12) |  // Output enable
                    (1 << 1)  |  // TX enable
                    (1 << 0);    // RX enable
        u32 scaler = (DrvCprGetSysClockKhz() * 1000) / (BAUDRATE << 3);
        SET_REG_WORD(UART_CTRL_ADR, mask);
        SET_REG_WORD(UART_SCALER_ADR, scaler);
        DrvGpioMode(TX_PIN, TX_MODE);
        DrvGpioMode(RX_PIN, RX_MODE);
      }

      int USBPutChar(int c)
      {
        // Wait for TX FIFO to not be full
        while (REG_WORD(UART_STATUS_ADR) & 0x200)
          ;

        SET_REG_WORD(UART_DATA_ADR, c);

        return c;
      }

      s32 USBGetChar(u32 timeout)
      {
        u32 end = GetMicroCounter() + timeout;
        do
        {
          // Check RCNT - Receiver FIFO count
          if (REG_WORD(UART_STATUS_ADR) >> 26)
          {
            return REG_WORD(UART_DATA_ADR);
          }
        } while (GetMicroCounter() < end);

        return -1;
      }

      void USBSendBuffer(u8* buffer, u32 size)
      {
        for (int i = 0; i < size; i++)
        {
          USBPutChar(buffer[i]);
        }
      }
    }
  }
}

