#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "movidius.h"

#define DDR_BUFFER    __attribute__((section(".ddr.text")))

#define CMX_CONFIG      (0x66666666)
#define L2CACHE_CONFIG  (L2CACHE_NORMAL_MODE)

      static const u32 FRAME_WIDTH = 640;
      static const u32 FRAME_HEIGHT = 480;
      static const u32 FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT;

      static DDR_BUFFER u8 frame[FRAME_SIZE];


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

      // Forward declarations
      void UARTInit();
      void MotorInit();
      void USBInit();
      void USBUpdate();
      void SendHeader(const u8 packetType);
      void SendFooter(const u8 packetType);
      
#ifdef SERIAL_IMAGING
      namespace USBprintBuffer
      {
        // This is a (ring) buffer to store messages created using printf in main
        // execution, until they can be picked up and actually sent over the USB
        // UART by long execution when we are also using the UART to send image
        // data and don't want to step on that data with frequent main execution
        // messages.
        static const u32  BUFFER_LENGTH = 512;
        
        static char  buffer[BUFFER_LENGTH];
        static u32   readIndex = 0;
        static u32   writeIndex = 0;
        
        void IncrementIndex(u32& index) {
          ++index;
          if(index == BUFFER_LENGTH) {
            index = 0;
          }
        }
        
        void SendMessage()
        {

          // Send all the characters in the buffer as of right now
          const u32 endIndex = writeIndex; // make a copy of where we should stop
          
          // Nothing to send
          if (endIndex == readIndex) {
            return;
          }
          
          SendHeader(0xDD);
          
          while(readIndex != endIndex)
          {
            // Send the character and circularly increment the read index:
            HAL::USBPutChar(buffer[readIndex]);
            IncrementIndex(readIndex);
          }
          
          SendFooter(0xDD);
        }
        
      } // namespace USBprintBuffer
      
      // Add a character to the ring buffer
      int USBBufferChar(int c)
      {
        using namespace USBprintBuffer;
        buffer[writeIndex] = (char) c;
        IncrementIndex(writeIndex);
      }
      
#endif // SERIAL_IMAGING
      
      


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

      static u8 frameResolution = CAMERA_MODE_QQQVGA;
      
      
      void SendHeader(const u8 packetType)
      {
        USBPutChar(USB_PACKET_HEADER[0]);
        USBPutChar(USB_PACKET_HEADER[1]);
        USBPutChar(USB_PACKET_HEADER[2]);
        USBPutChar(USB_PACKET_HEADER[3]);
        USBPutChar(packetType);
      }
      
      void SendFooter(const u8 packetType)
      {
        USBPutChar(USB_PACKET_FOOTER[0]);
        USBPutChar(USB_PACKET_FOOTER[1]);
        USBPutChar(USB_PACKET_FOOTER[2]);
        USBPutChar(USB_PACKET_FOOTER[3]);
        USBPutChar(packetType);
      }
      
      static void SendFrame()
      {
        const u8* image = frame;

        // Set window size for averaging when downsampling and send
        // a corresponding header
        u32 inc = 1;
        u8  frameResolution = 0;
        switch(frameResolution)
        {
          case CAMERA_MODE_QVGA:
            inc = 2;
            frameResolution = CAMERA_MODE_QVGA_HEADER;
            break;
            
          case CAMERA_MODE_QQVGA:
            inc = 4;
            frameResolution = CAMERA_MODE_QQVGA_HEADER;
            break;
            
          case CAMERA_MODE_QQQVGA:
            inc = 8;
            frameResolution = CAMERA_MODE_QQQVGA_HEADER;
            break;
            
          case CAMERA_MODE_QQQQVGA:
            inc = 16;
            frameResolution = CAMERA_MODE_QQQQVGA_HEADER;
            break;

          case CAMERA_MODE_VGA:
          default:
            inc = 1;
            frameResolution = CAMERA_MODE_VGA_HEADER;
        }
        
        SendHeader(frameResolution);

        if(inc==1)
        {
          // No averaging
          for(int i=0; i < 640*480; i++)
          {
            USBPutChar(image[i]);
          }
          
        } else {
          // Average inc x inc windows
          for (int y = 0; y < 480; y += inc)
          {
            for (int x = 0; x < 640; x += inc)
            {
              int sum = 0;
              for (int y1 = y; y1 < y + inc; y1++)
              {
                for (int x1 = x; x1 < x + inc; x1++)
                {
                  sum += image[(x1 + y1 * 640) ^ 3];
                }
              }
              USBPutChar(sum / (inc * inc));
            }
          }
        } // IF / ELSE inc==1
        
        SendFooter(frameResolution);
        
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
        const int PRIORITY = 1;

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

        // Acknowledge all interrupts
        REG_WORD(ICB_CLEAR_0_ADR) = 0xFFFFffff;
        REG_WORD(ICB_CLEAR_1_ADR) = 0xFFFFffff;

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

      void IRQDisable()
      {
        swcLeonSetPIL(0);
      }

      void IRQEnable()
      {
        swcLeonEnableTraps();
      }
      
      ///////////////////////////////////////
      // Function stubs
      //
      // Flesh out and move to appropriate place!
      
      // Gripper control
      bool IsGripperEngaged() {return false;}
      
      // Cameras
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


  u32 t = HAL::GetMicroCounter();

  while (true)
  {
    //Console::Update();

    Robot::step_LongExecution();

    CameraStartFrame(HAL::CAMERA_FRONT, frame, HAL::CAMERA_MODE_VGA,
        HAL::CAMERA_UPDATE_SINGLE, 0, false);

    while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
    {
    }

#ifdef SERIAL_IMAGING
    HAL::SendFrame();
    
    HAL::USBprintBuffer::SendMessage();
#endif
    
    
    u32 t2 = HAL::GetMicroCounter();
    //printf("%i\n", (t2 - t));
    t = t2;
 
    switch(HAL::USBGetChar())
    {
      case HAL::CAMERA_MODE_VGA_HEADER:
        HAL::frameResolution = HAL::CAMERA_MODE_VGA;
        break;
        
      case HAL::CAMERA_MODE_QVGA_HEADER:
        HAL::frameResolution = HAL::CAMERA_MODE_QVGA;
        break;
        
      case HAL::CAMERA_MODE_QQVGA_HEADER:
        HAL::frameResolution = HAL::CAMERA_MODE_QQVGA;
        break;
        
      case HAL::CAMERA_MODE_QQQQVGA_HEADER:
        HAL::frameResolution = HAL::CAMERA_MODE_QQQQVGA;
        break;

      case HAL::CAMERA_MODE_QQQVGA_HEADER:
      default:
        HAL::frameResolution = HAL::CAMERA_MODE_QQQVGA;
    }

    

    //HAL::USBUpdate();

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

