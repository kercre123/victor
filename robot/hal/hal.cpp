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
      static const u32 D_TIMER_CFG_ENABLE       = (1 << 0);
      static const u32 D_TIMER_CFG_RESTART      = (1 << 1);
      static const u32 D_TIMER_CFG_EN_INT       = (1 << 2);
      static const u32 D_TIMER_CFG_CHAIN        = (1 << 3);
      static const u32 D_TIMER_CFG_IRQ_PENDING  = (1 << 4);
      static const u32 D_TIMER_CFG_FORCE_RELOAD = (1 << 5);

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

      // Forward declarations
      void UARTInit();
      void MotorInit();
      void USBInit();
      void USBUpdate();

      static void SendFrame()
      {
        const u8* image = HAL::FrontCameraGetFrame();

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
      }

      static u32 MainExecutionIRQ(u32, u32)
      {
        // Check neck potentiometer here, until radio implements it
        // ...

        Robot::step_MainExecution();

        // Pet the watchdog
        // Other HAL tasks?

        return 0;
      }

      static void SetupMainExecution()
      {
        const int PRIORITY = 10;

        // TODO: Fix this and use instead of movi timer code
        /*const u32 timerConfig = (1 << 0) |  // D_TIMER_CFG_ENABLE
          (1 << 1) |  // D_TIMER_CFG_RESTART
          (1 << 2) |  // D_TIMER_CFG_EN_INT
          (1 << 5);  // D_TIMER_CFG_FORCE_RELOAD

        // Get the number of cycles for a 2ms interrupt
        const u32 cyclesPerIRQ = 2000 * (u64)DrvCprGetSysClocksPerUs();
        const u32 D_TIMER_CFG_IRQ_PENDING = (1 << 4);
        // Ensure there's not a pending IRQ
        CLR_REG_BITS_MASK(TIM1_CONFIG_ADR, D_TIMER_CFG_IRQ_PENDING);
        // Configure the interrupt
        DrvIcbSetupIrq(IRQ_TIMER_1, PRIORITY, POS_LEVEL_INT, MainIRQ);
        // Reload value
        SET_REG_WORD(TIM1_RELOAD_VAL_ADR, cyclesPerIRQ);
        // Configuration
        SET_REG_WORD(TIM1_CONFIG_ADR, timerConfig);*/

        // Call MainExecutionIRQ every 2ms
        DrvTimerCallAfterMicro(2000, MainExecutionIRQ, 0, PRIORITY);
      }

      static void InitMemory()
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
        REG_WORD(LHB_CMX_CTRL_MISC) |= 1 << 24;

        // Setup L2 cache
        DrvL2CacheSetupPartition(PART128KB);
        DrvL2CacheAllocateSetPartitions();
        swcLeonFlushCaches();

        UARTInit();

        FrontCameraInit();

        //USBInit();

        MotorInit();
      }

      // Called from Robot::Init(), but not actually used here.
      ReturnCode Init()
      {
        return 0;
      }
      
      
      ///////////////////////////////////////
      // Function stubs
      //
      // Flesh out and move to appropriate place!
      
      // Gripper control
      bool IsGripperEngaged() {return false;}
      
      // Cameras
      //const u8* MatCameraGetFrame() {return NULL;}
      //const u8* FrontCameraGetFrame() {return NULL;}
      
      const FrameGrabber GetHeadFrameGrabber() {FrameGrabber junk; return junk;}
      const FrameGrabber GetMatFrameGrabber() {FrameGrabber junk; return junk;}
      
      const CameraInfo* GetHeadCamInfo() {CameraInfo* junk; return junk;}
      const CameraInfo* GetMatCamInfo() {CameraInfo* junk; return junk;}
      
      // Communications
      //bool IsConnected() {return false;}
      u32 RadioFromBase(u8 buffer[RADIO_BUFFER_SIZE]) {return 0;}
      bool RadioToBase(u8* buffer, u32 size) {return true;}
      
      
      // Misc
      //bool IsInitialized();
      void UpdateDisplay() {};
      
      // Get the number of microseconds since boot
      //u32 GetMicroCounter(void);
      //void MicroWait(u32 microseconds);
      
      s32 GetRobotID(void) {return 0;}
      
      // Take a step (needed for webots, possibly a no-op for real robot?)
      ReturnCode Step(void) {return EXIT_SUCCESS;}
      
      // Ground truth (no-op if not in simulation?)
      void GetGroundTruthPose(f32 &x, f32 &y, f32& rad) {};
      
      
    }
  }
}

int main()
{
  HAL::InitMemory();

  Robot::Init();

  HAL::SetupMainExecution();

  while (true)
  {
    //HAL::SendFrame();

    //HAL::USBUpdate();

    Robot::step_LongExecution();

    //printf("%X\n", *(volatile u32*)TIM1_CNT_VAL_ADR);

    //HAL::MotorSetPower(HAL::MOTOR_LIFT, 1.0);
    //SleepMs(1000);
    //HAL::MotorSetPower(HAL::MOTOR_LIFT, -1.0);
    //SleepMs(1000);

//    HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 1.00f);
//    HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 1.00f);
//    HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 1.00f);
//    SleepMs(1000);
//    HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, -1.00f);
//    SleepMs(1000);

/*    printf("%d %d %d\n",
        DrvGpioGetPin(106),
        DrvGpioGetPin(32),
        DrvGpioGetPin(35));*/

/*    printf("%d %d %d\n",
        DrvGpioGetPin(106),
        DrvGpioGetPin(32),
        DrvGpioGetPin(35)); */

/*      if ((REG_WORD(GPIO_PWM_DEC0_VLD_ADR) & 0x3F) == 0x3F)
      {
        u32 dec = REG_WORD(GPIO_PWM_DEC_0_N1_ADR);
        printf("%d %d\n",
          dec >> 16,
          dec & 0xFFFF);

        SET_REG_WORD(GPIO_PWM_CLR_ADR, 0xFF);
      } */

//    HAL::MotorSetPower(HAL::MOTOR_GRIP, -1.0f);
//    SleepMs(1000);
  }

  return 0;
}

