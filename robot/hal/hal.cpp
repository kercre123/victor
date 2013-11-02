#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "movidius.h"

#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CONFIG  (L2CACHE_NORMAL_MODE)

u32 __cmx_config __attribute__((section(".cmx.ctrl"))) = CMX_CONFIG;
u32 __l2_config  __attribute__((section(".l2.mode")))  = L2CACHE_CONFIG;

using namespace Anki::Cozmo;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Called from Robot::Init(), but not actually used here.
      ReturnCode Init()
      {
        return 0;
      }
    }
  }
}

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

static void SendFrame()
{
  const u8* image = HAL::FrontCameraGetFrame();

  HAL::UARTPutChar(0xBE);
  HAL::UARTPutChar(0xEF);
  HAL::UARTPutChar(0xF0);
  HAL::UARTPutChar(0xFF);
  HAL::UARTPutChar(0xBD);

  for (int y = 0; y < 480; y += 1)
  {
    for (int x = 0; x < 640; x += 1)
    {
      HAL::UARTPutChar(image[x * 2 + 0 + y * 1280]);
    }
  }
}

static u32 MainExecutionIRQ(u32, u32)
{
  Robot::step_MainExecution();

  // Clear the interrupt
//  const u32 D_TIMER_CFG_IRQ_PENDING = (1 << 4);
//  CLR_REG_BITS_MASK(TIM1_CONFIG_ADR, D_TIMER_CFG_IRQ_PENDING);
//  DrvIcbIrqClear(IRQ_TIMER_1);

  return 0;
}

static void SetupMainExecution()
{
  const int PRIORITY = 1;

  // TODO: Fix this and use instead of movi timer code
/*  const u32 timerConfig = (1 << 0) |  // D_TIMER_CFG_ENABLE
    (1 << 1) |  // D_TIMER_CFG_RESTART
    (1 << 2) |  // D_TIMER_CFG_EN_INT
    (1 << 5);  // D_TIMER_CFG_FORCE_RELOAD

  // Get the number of cycles for a 2ms interrupt
  const u32 cyclesPerIRQ = 2000 * (u64)DrvCprGetSysClocksPerUs();
  const u32 D_TIMER_CFG_IRQ_PENDING = (1 << 4);
  // Ensure there's not a pending IRQ
  CLR_REG_BITS_MASK(TIM1_CONFIG_ADR, D_TIMER_CFG_IRQ_PENDING);
  // Configure the interrupt
  DrvIcbSetupIrq(IRQ_TIMER_1, PRIORITY, POS_LEVEL_INT, MainExecutionIRQ);
  // Reload value
  SET_REG_WORD(TIM1_RELOAD_VAL_ADR, cyclesPerIRQ);
  // Configuration
  SET_REG_WORD(TIM1_CONFIG_ADR, timerConfig); */

  DrvTimerCallAfterMicro(2000, MainExecutionIRQ, 0, PRIORITY);
}

void InitMemory()
{
  // Initialize the Clock Power Reset module
  if (DrvCprInit(NULL, NULL))
  {
    while (true)
      ;
  }

  // Initialize the CMX RAM layout
  SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, CMX_CONFIG);

  // Initialize the clock configuration using the variables above
  if (DrvCprSetupClocks(&m_clockConfig))
  {
    while (true)
      ;
  }

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

  HAL::UARTInit();
  printf("\nUART Initialized\n");

  HAL::FrontCameraInit();
  printf("\nFront Camera Initialized\n");

  //HAL::USBInit();

  HAL::EncodersInit();
}

int main()
{
  InitMemory();

  Robot::Init();

  SetupMainExecution();

  while (true)
  {
    Robot::step_LongExecution();
  }

  return 0;
}

