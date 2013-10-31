#include "anki/cozmo/robot/hal.h"
#include "movidius.h"

#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CONFIG  (L2CACHE_NORMAL_MODE)

u32 __cmx_config __attribute__((section(".cmx.ctrl"))) = CMX_CONFIG;
u32 __l2_config  __attribute__((section(".l2.mode")))  = L2CACHE_CONFIG;

using namespace Anki::Cozmo::HAL;

static const tyAuxClkDividerCfg m_auxClockConfig[] =
{
  {
    (AUX_CLK_MASK_DDR   |
    AUX_CLK_MASK_IO),
    GEN_CLK_DIVIDER(1, 1)
  },
  {
    AUX_CLK_MASK_CIF1,
    GEN_CLK_DIVIDER(24, 180)  // Reference clock of 24 MHz
  },
  {
    AUX_CLK_MASK_SAHB,      // Clock the Slow AHB Bus for SDHOST, SDIO, USB, NAND
    GEN_CLK_DIVIDER(1, 2)   // Slow AHB must run at less than 100 MHz, so let's run at 90
  },

  {0, 0}
};

static const tySocClockConfig m_clockConfig =
{
  12000,    // Default to 12 Mhz input oscillator
  180000,   // 180 MHz from the PLL

  {
    DEFAULT_CORE_BUS_CLOCKS |
    DEV_CIF1                |
    DEV_IIC1                |
    DEV_SVU0,

    GEN_CLK_DIVIDER(1, 1)
  },
  m_auxClockConfig
};

int main()
{
  // Initialize the Clock Power Reset module
  if (DrvCprInit(NULL, NULL))
    return 1;

  // Initialize the CMX RAM layout
  SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, CMX_CONFIG);

  // Initialize the clock configuration using the variables above
  if (DrvCprSetupClocks(&m_clockConfig))
    return 1;

  SET_REG_WORD(L2C_MODE_ADR, L2CACHE_CONFIG);

  // Initialize DDR memory
  DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR, NULL));

  // Turn off all GPIO-related IRQs
  DrvGpioIrqResetAll();

  // Force big-endian memory swap
  *(volatile u32*)LHB_CMX_CTRL_MISC |= 1 << 24;

  // Setup L2 cache
  DrvL2CacheSetupPartition(PART128KB);
  DrvL2CacheAllocateSetPartitions();
  swcLeonFlushCaches();

  UARTInit();
  printf("\nUART Initialized\n");

  FrontCameraInit();
  printf("\nCamera Initialized\n");

  //USBInit();

  EncodersInit();

//  SetWheelAngularVelocity(0.75f, 0.f);

/*  DrvGpioMode(98, 7 | D_GPIO_DIR_IN);

  GpioPadSet(98, GpioPadGet(98) | D_GPIO_PAD_PULL_DOWN);

  SleepMs(100);

  printf("pad == %d\n", DrvGpioGetPin(98));


  GpioPadSet(98, (GpioPadGet(98) &~ D_GPIO_PAD_PULL_DOWN) |
    D_GPIO_PAD_PULL_UP | D_GPIO_PAD_VOLT_2V5 | D_GPIO_PAD_BIAS_2V5);

  SleepMs(100);

  printf("pad == %d\n", DrvGpioGetPin(98)); */

  Init();

  while (true)
  {
    //USBUpdate();

    MainExecution();
    LongExecution();
  }

/*      const u32 pinA = 80;
      const u32 pinB = 83;
      SleepMs(1000);
      DrvGpioMode(pinA, 7 | D_GPIO_DIR_OUT);
      DrvGpioMode(pinB, 7 | D_GPIO_DIR_OUT);
      DrvGpioSetPinHi(pinA);
      DrvGpioSetPinLo(pinB);

      SleepMs(1000);
      DrvGpioSetPinLo(pinA);
      DrvGpioSetPinHi(pinB); */


//    printf("%d", DrvGpioGetPin(106));

/*    u8* image = 0;
    do
    {
      image = (u8*)FrontCameraGetFrame();
    }
    while (!image);

    UARTPutChar(0xBE);
    UARTPutChar(0xEF);
    UARTPutChar(0xF0);
    UARTPutChar(0xFF);
    UARTPutChar(0xBD);

    for (int y = 0; y < 480; y += 8)
    {
      for (int x = 0; x < 640; x += 8)
      {
        UARTPutChar(image[x * 2 + 0 + y * 1280]);
      }
    }
  }*/

  return 0;
}

