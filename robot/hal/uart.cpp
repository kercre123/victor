#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

#ifndef USE_USB

#ifndef DEFAULT_BAUDRATE
#define DEFAULT_BAUDRATE 1500000
#endif

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const u32 BAUDRATE = DEFAULT_BAUDRATE;
      const u8 TX_PIN = 69;
      const u32 TX_MODE = D_GPIO_MODE_1;
      const u8 RX_PIN = 70;
      const u32 RX_MODE = D_GPIO_MODE_1;
      static const u32 IRQ = IRQ_UART;
      static const u32 IRQ_LEVEL = 5;
      
      const u8 ADC_CS = 39;
      const u8 FLASH_CS = 72;

      static void UARTIRQ(u32 source);

      static const u8 BUFFER_SIZE = 128;
      static const u8 BUFFER_MASK = 0x7f;
      static volatile u8 m_buffer[BUFFER_SIZE];
      static volatile u8 m_currentRead = 0;
      static volatile u8 m_currentWrite = 0;

      void UARTInit()
      {
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_UART);

        // Setup the interrupt trigger
        DrvIcbDisableIrq(IRQ);
        DrvIcbIrqClear(IRQ);
        swcLeonSetPIL(0);
        DrvIcbSetIrqHandler(IRQ, UARTIRQ);
        DrvIcbConfigureIrq(IRQ, IRQ_LEVEL, POS_EDGE_INT);
        DrvIcbEnableIrq(IRQ);

        // Calculate the mask to enable specific features
        u32 mask =  (1 << 31) |  // FIFO write enable
                    (1 << 12) |  // Output enable
                    (1 << 2)  |  // Receive interrupt enable
                    (1 << 1)  |  // TX enable
                    (1 << 0);    // RX enable
        // Calculate the baudrate prescaler
        u32 scaler = (DrvCprGetSysClockKhz() * 1000) / (BAUDRATE << 3) - 1;
        SET_REG_WORD(UART_CTRL_ADR, mask);
        SET_REG_WORD(UART_SCALER_ADR, scaler);
        
        u32 pad = D_GPIO_PAD_NO_PULL |
          D_GPIO_PAD_DRIVE_2mA |
          D_GPIO_PAD_VOLT_2V5 |
          D_GPIO_PAD_SLEW_SLOW |
          D_GPIO_PAD_SCHMITT_ON |
          D_GPIO_PAD_RECEIVER_ON |
          D_GPIO_PAD_BIAS_2V5 |
          D_GPIO_PAD_LOCALCTRL_OFF |
          D_GPIO_PAD_LOCALDATA_LO |
          D_GPIO_PAD_LOCAL_PIN_OUT;

        GpioPadSet(TX_PIN, pad);
        GpioPadSet(RX_PIN, pad);
        
        DrvGpioMode(TX_PIN, TX_MODE);
        DrvGpioMode(RX_PIN, RX_MODE | D_GPIO_DIR_IN);
        
        // XXX-NDM: Turn off SPI chip selects because the pin conflicts with receive
        GpioPadSet(FLASH_CS, pad);
        pad = D_GPIO_PAD_PULL_UP |
          D_GPIO_PAD_DRIVE_2mA |
          D_GPIO_PAD_VOLT_2V5 |
          D_GPIO_PAD_SLEW_SLOW |
          D_GPIO_PAD_SCHMITT_ON |
          D_GPIO_PAD_RECEIVER_ON |
          D_GPIO_PAD_BIAS_2V5 |
          D_GPIO_PAD_LOCALCTRL_OFF |
          D_GPIO_PAD_LOCALDATA_LO |
          D_GPIO_PAD_LOCAL_PIN_OUT;
        GpioPadSet(ADC_CS, pad);        
        DrvGpioMode(FLASH_CS, D_GPIO_MODE_7 | D_GPIO_DIR_IN);
        DrvGpioMode(ADC_CS, D_GPIO_MODE_7 | D_GPIO_DIR_IN);
        
        // Re-enable interrupts
        swcLeonEnableTraps();
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
          //if (REG_WORD(UART_STATUS_ADR) >> 26)
          //{
          //  return REG_WORD(UART_DATA_ADR);
          //}
          if (m_currentRead != m_currentWrite)
          {
            s32 c = m_buffer[m_currentRead] & 0xff;
            m_currentRead = (m_currentRead + 1) & BUFFER_MASK;
            return c;
          }
        } while (GetMicroCounter() < end);

        return -1;
      }
      
      s32 USBPeekChar(u32 offset)
      {
        // If trying to peek further than there are characters in the buffer
        // exit now.
        if (USBGetNumBytesToRead() <= offset) {
          return -1;
        }
        
        // Set the read index with offset
        u32 currReadWithOffset = (m_currentRead + offset) & BUFFER_MASK;

        s32 c = m_buffer[currReadWithOffset] & 0xff;
        return c;
      }
      
      u32 USBGetNumBytesToRead()
      {
        return (m_currentWrite - m_currentRead) & BUFFER_MASK;
      }
      
      void USBSendBuffer(const u8* buffer, const u32 size)
      {
        for (u32 i = 0; i < size; i++)
        {
          USBPutChar(buffer[i]);
        }
      }

      void UARTIRQ(u32 source)
      {
        // Check if there's any data in the Rx FIFO (RCNT)
        while (REG_WORD(UART_STATUS_ADR) >> 26)
        {
          m_buffer[m_currentWrite] = REG_WORD(UART_DATA_ADR);
          m_currentWrite = (m_currentWrite + 1) & BUFFER_MASK;
        }

        // Clear the interrupt
        DrvIcbIrqClear(IRQ);
      }
    }
  }
}

#endif  // ifndef USE_USB

