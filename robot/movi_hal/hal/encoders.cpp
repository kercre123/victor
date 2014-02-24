#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const u32 m_encoder1Pin = 106;
      static const u32 m_encoder2Pin = 32;
      static const u32 m_encoder3Pin = 35;

      static const u32 m_src = D_GPIO_IRQ_SRC_0;
      static const u32 m_irq = IRQ_GPIO_0;
      static const u32 m_priority = 5;
      static const u32 m_type = POS_EDGE_INT;

      static u64 m_encoder1LastTick = 0;
      static u64 m_encoder2LastTick = 0;
      static u64 m_encoder3LastTick = 0;
      static u64 m_encoder1Difference = 0;
      static u64 m_encoder2Difference = 0;
      static u64 m_encoder3Difference = 0;

      void EncodersIRQ(u32 source)
      {
        u8 state1 = DrvGpioGetPin(m_encoder1Pin);
        u8 state2 = DrvGpioGetPin(m_encoder2Pin);
        u8 state3 = DrvGpioGetPin(m_encoder3Pin);

        u32 mode1 = DrvGpioGetMode(m_encoder1Pin);
        u32 mode2 = DrvGpioGetMode(m_encoder2Pin);
        u32 mode3 = DrvGpioGetMode(m_encoder3Pin);

        if (state1)
        {
          // Only switch if it was a real logic-level rising edge
          if (!(mode1 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder1Difference = ticks - m_encoder1LastTick;
            m_encoder1LastTick = ticks;
          }

          DrvGpioMode(m_encoder1Pin, mode1 ^ D_GPIO_DATA_INV_ON);
        }
        if (state2)
        {
          if (!(mode2 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder2Difference = ticks - m_encoder2LastTick;
            m_encoder2LastTick = ticks;
          }

          DrvGpioMode(m_encoder2Pin, mode2 ^ D_GPIO_DATA_INV_ON);
        }
        if (state3)
        {
          if (!(mode3 & D_GPIO_DATA_INV_ON))
          {
            u64 ticks = GetMicroCounter();
            m_encoder3Difference = ticks - m_encoder3LastTick;
            m_encoder3LastTick = ticks;
          }

          DrvGpioMode(m_encoder3Pin, mode3 ^ D_GPIO_DATA_INV_ON);
        }

        DrvIcbIrqClear(m_irq);
      }

      void EncodersInit()
      {
        DrvGpioIrqConfig(m_encoder1Pin, m_src, m_type, m_priority, EncodersIRQ);
        DrvGpioIrqConfig(m_encoder2Pin, m_src, m_type, m_priority, EncodersIRQ);
        DrvGpioIrqConfig(m_encoder3Pin, m_src, m_type, m_priority, EncodersIRQ);

        //DrvGpioMode(m_encoder1Pin, D_GPIO_MODE_7 | D_GPIO_DIR_IN);
        //DrvGpioMode(m_encoder2Pin, D_GPIO_MODE_7 | D_GPIO_DIR_IN);
        //DrvGpioMode(m_encoder3Pin, D_GPIO_MODE_7 | D_GPIO_DIR_IN);

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

        GpioPadSet(m_encoder1Pin, pad);
        GpioPadSet(m_encoder2Pin, pad);
        GpioPadSet(m_encoder3Pin, pad);

        u64 ticks = GetMicroCounter();
        m_encoder1LastTick = m_encoder2LastTick = m_encoder3LastTick = ticks;
      }
    }
  }
}

