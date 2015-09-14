#include "anki/cozmo/robot/hal.h"

#include "anki/common/robot/trig_fast.h"
#include "hal/portable.h"

#include "board.h"
#include "fsl_debug_console.h"

#include "anki/cozmo/shared/cozmoConfig.h" // for calibration parameters
#include "anki/common/robot/config.h"
#include "anki/common/robot/benchmarking.h"

#include "hal/i2c.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // For headboard 4.1 - there is no D0
      GPIO_PIN_SOURCE(D1, PTC, 1);
      GPIO_PIN_SOURCE(D2, PTC, 2);
      GPIO_PIN_SOURCE(D3, PTC, 3);
      GPIO_PIN_SOURCE(D4, PTC, 4);
      GPIO_PIN_SOURCE(D5, PTC, 5);
      GPIO_PIN_SOURCE(D6, PTC, 6);
      GPIO_PIN_SOURCE(D7, PTC, 7);
      
      GPIO_PIN_SOURCE(HSYNC,   PTD, 5);
      GPIO_PIN_SOURCE(XCLK,    PTB, 0);
      
      GPIO_PIN_SOURCE(RESET_N, PTE, 25);
      GPIO_PIN_SOURCE(PWDN,    PTA, 1);

      // Configuration for GC0329 camera chip
      const u8 I2C_ADDR = 0x31; 
      const u8 CAM_SCRIPT[] =
      {
        #include "gc0329.h"
        0, 0
      };
      
      const int TOTAL_COLS = 640, TOTAL_ROWS = 480, SWIZZLE_ROWS = 8, BYTES_PER_PIX = 2;
      CAMRAM static u8 dmaBuff_[TOTAL_COLS * BYTES_PER_PIX * 2];
      CAMRAM static u8 swizzle_[TOTAL_COLS * SWIZZLE_ROWS];

      // Camera exposure value
      u32 exposure_;
      
      // For self-test purposes only.
      // XXX - restore this code for EP1
#if 0
      static void CamSetPulls(GPIO_TypeDef* GPIOx, u32 pin, int index)
      {
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_PuPd = index ? GPIO_PuPd_UP : GPIO_PuPd_DOWN;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_Pin =  pin;
        GPIO_Init(GPIOx, &GPIO_InitStructure);
      }
      static int CamGetPulls(GPIO_TypeDef* GPIOx, u32 pin, int index)
      {
        return (!!(GPIO_READ(GPIOx) & pin)) << index;
      }
      static int CamReadDB(int pullup)
      {
        CamSetPulls(GPIO_D0, PIN_D0, pullup & 1);
        CamSetPulls(GPIO_D1, PIN_D1, pullup & 2);
        CamSetPulls(GPIO_D2, PIN_D2, pullup & 4);
        CamSetPulls(GPIO_D3, PIN_D3, pullup & 8);
        CamSetPulls(GPIO_D4, PIN_D4, pullup & 16);
        CamSetPulls(GPIO_D5, PIN_D5, pullup & 32);
        CamSetPulls(GPIO_D6, PIN_D6, pullup & 64);
        CamSetPulls(GPIO_D7, PIN_D7, pullup & 128);
        
        MicroWait(25);
        
        return
          CamGetPulls(GPIO_D0, PIN_D0, 0) |
          CamGetPulls(GPIO_D1, PIN_D1, 1) |
          CamGetPulls(GPIO_D2, PIN_D2, 2) |
          CamGetPulls(GPIO_D3, PIN_D3, 3) |
          CamGetPulls(GPIO_D4, PIN_D4, 4) |
          CamGetPulls(GPIO_D5, PIN_D5, 5) |
          CamGetPulls(GPIO_D6, PIN_D6, 6) |
          CamGetPulls(GPIO_D7, PIN_D7, 7);
      }
#endif
      
      // Set up peripherals and GPIO for camera interface
      static void InitIO()
      {
        // Set up databus to all GPIO inputs
        SOURCE_SETUP(GPIO_D1, SOURCE_D1, SourceGPIO);
        SOURCE_SETUP(GPIO_D2, SOURCE_D2, SourceGPIO);
        SOURCE_SETUP(GPIO_D3, SOURCE_D3, SourceGPIO);
        SOURCE_SETUP(GPIO_D4, SOURCE_D4, SourceGPIO);
        SOURCE_SETUP(GPIO_D5, SOURCE_D5, SourceGPIO);
        SOURCE_SETUP(GPIO_D6, SOURCE_D6, SourceGPIO);
        SOURCE_SETUP(GPIO_D7, SOURCE_D7, SourceGPIO);

        // Drive PWDN and RESET to safe defaults
        GPIO_SET(GPIO_PWDN, PIN_PWDN);
        GPIO_OUT(GPIO_PWDN, PIN_PWDN);
        SOURCE_SETUP(GPIO_PWDN, SOURCE_PWDN, SourceGPIO);
        
        GPIO_RESET(GPIO_RESET_N, PIN_RESET_N);
        GPIO_OUT(GPIO_RESET_N, PIN_RESET_N);
        SOURCE_SETUP(GPIO_RESET_N, SOURCE_RESET_N, SourceGPIO);

        // Set up HSYNC to trigger DMA start on rising edge
        SOURCE_SETUP(GPIO_HSYNC, SOURCE_HSYNC, SourceGPIO | SourceDMARise);
        
        // Configure XCLK (on FTM1) for bus clock / 2 - fastest we can go (24 MHz)
        SIM_SCGC6 |= SIM_SCGC6_FTM1_MASK;   // Enable FTM1
        FTM1_SC = 0;              // Reset
        FTM1_MOD = 1;             // Minimum timer period
        FTM1_C0SC = FTM_CnSC_ELSB_MASK|FTM_CnSC_MSB_MASK;   // Edge-aligned PWM on CH0
        FTM1_C0V = 1;                                       // 50% duty cycle on CH0
        FTM1_SC = FTM_SC_CLKS(1); // Use bus clock with a /1 prescaler
        SOURCE_SETUP(GPIO_XCLK, SOURCE_XCLK, SourceAlt3);
      }
      
      volatile u8 eof_ = 0;
      
      void HALExec(u8* buf, int buflen, int eof);
      
      int JPEGStart(int quality);
      int JPEGCompress(u8* out, u8* in, int pitch);
      int JPEGEnd(u8* out);
      
      // Set up camera 
      static void InitCam()
      {
        JPEGStart(50);
        
        // Power-up/reset the camera
        MicroWait(50);
        GPIO_RESET(GPIO_PWDN, PIN_PWDN);
        MicroWait(50);
        GPIO_SET(GPIO_RESET_N, PIN_RESET_N);

        // Read ID regs to get I2C state machine into proper state
        I2CRead(I2C_ADDR, 0xf0);
        I2CRead(I2C_ADDR, 0xf1);
        
        // Write the configuration registers            
        const u8* p = CAM_SCRIPT;
        while (*p) {
          I2CWrite(I2C_ADDR, p[0], p[1]);
          p += 2;
        }
        
        // TODO: Check that the GPIOs are okay
        //for (u8 i = 1; i; i <<= 1)
        //  printf("\r\nCam dbus: set %x, got %x", i, CamReadDB(i));           
      }
      
      // Initialize DMA to row buffer, and fire an interrupt at end of each transfer
      static void InitDMA()
      {
        // Enable DMA clocks
        SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
        SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;
        
        // Enable interrupt
        NVIC_EnableIRQ(DMA0_IRQn);
        
        // Note:  Adjusting DMA crossbar priority doesn't help, since any peripheral I/O causes DMA-harming wait states
        // The only way that DMA works is to keep the CPU from touching registers or RAM block 0 (starting with 0x1fff)
        // MCM_PLACR = 0; // MCM_PLACR_ARB_MASK;
        
        // Set up DMA channel 0 to repeatedly move one line buffer worth of pixels
        DMA_CR = DMA_CR_CLM_MASK;   // Continuous loop mode? (Makes no difference?)  
        DMA_TCD0_CSR = DMA_CSR_INTMAJOR_MASK;     // Stop channel, set up interrupt on transfer complete
        DMA_TCD0_NBYTES_MLNO = sizeof(dmaBuff_);  // Number of transfers in minor loop
        DMA_TCD0_ATTR = DMA_ATTR_SSIZE(0)|DMA_ATTR_DSIZE(0);  // Source 8-bit, dest 8-bit
        DMA_TCD0_SOFF = 0;          // Source (register) doesn't increment
        DMA_TCD0_SADDR = (uint32_t)&GPIOC_PDIR;
        DMA_TCD0_DOFF = 1;          // Destination (buffer) increments
        DMA_TCD0_DADDR = (uint32_t)dmaBuff_;
        DMA_TCD0_CITER_ELINKNO = 1; // Current major loop iteration (1 per interrupt)
        DMA_TCD0_BITER_ELINKNO = 1; // Beginning major loop iteration (1 per interrupt)
        DMA_TCD0_DLASTSGA = -sizeof(dmaBuff_);    // Point back at start of buffer after each loop

        // Set up DMA for UART1 transmit (5)
        UART1_C2 = 0x88;  // DMA on transmit, transmit enabled
        UART1_C5 = 0x80;
        DMAMUX_CHCFG1 = DMAMUX_CHCFG_ENBL_MASK | 5;
        
        DMA_TCD1_NBYTES_MLNO = 1;        // Number of transfers in minor loop
        DMA_TCD1_ATTR = DMA_ATTR_SSIZE(0)|DMA_ATTR_DSIZE(0);  // Source 8-bit, dest 8-bit
        DMA_TCD1_SOFF = 1;          // Source (buffer) increments
        DMA_TCD1_DOFF = 0;          // Destination (register) doesn't increment
        DMA_TCD1_DADDR = (uint32_t)&UART1_D;

        // Hook DMA request 0 to HSYNC
        const int DMAMUX_PORTA = 49;    // This is not in the .h files for some reason
        DMAMUX_CHCFG0 = DMAMUX_CHCFG_ENBL_MASK | (DMAMUX_PORTA + PORT_INDEX(GPIO_HSYNC));     
        DMA_ERQ = DMA_ERQ_ERQ0_MASK;

        //DMA_TCD0_CSR = 0; // DMA_CSR_START_MASK;

      }

      // Initialize camera
      void CameraInit()
      {
        InitIO();
        InitDMA();
        InitCam();
      }
        
      void CameraSetParameters(f32 exposure, bool enableVignettingCorrection)
      {
        // TODO: vignetting correction? Why?
        const f32 maxExposure = 0xf00; // Determined empirically
        f32 correctedExposure = exposure;
        
        if(exposure < 0.0f)
        {
          correctedExposure = 0;
        } else if(exposure > 1.0f)
        {
          correctedExposure = 1.0f;
        } 
        
        const u32 exposureU32 = (u32) floorf(correctedExposure * maxExposure + 0.5f);
        
        // Set exposure - let it get picked up during next vblank
      }
    }
  }
}

extern "C"
void DMA0_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  
  static u16 line = 0;
  static int last = 0;
  
  CAMRAM static u8 buf[2][128];
  static u8 whichbuf = 0;
  static u8 buflen = 0;
  static u8 whichpitch = 0;    // Swizzle pitch (80 or 640)
  static u8 eof = 0;
  
  // Kludgey look for start of frame
  int now = SysTick->VAL;
  int diff = last-now;
  last = now;
  if (diff < 0)
    diff += 6291456;
  if (diff > 100000) {
    line = 232;       // Swizzle buffer delays us by 8 lines
  }
  
  HALExec(&buf[whichbuf][4], buflen, eof);
  
  // Fill next buffer
  whichbuf ^= 1;
  u8* p = &buf[whichbuf][4];  // Offset 4 chars to leave room for a UART header
  buflen = 0;
  
  // Compute swizzle buffer address - this rolling buffer holds exactly 8 lines of video, the minimum for JPEG
  // Addressing the rolling buffer is complicated since we write linearly (640x1) but read macroblocks (80x8)
  if (0 == (line & 7))    // Switch pitch every 8 lines
    whichpitch ^= 1;
  int pitch = whichpitch ? 80 : 640;
  u8* swizz = swizzle_ + (line & 7) * (whichpitch ? 640 : 80);
  
  // Encode 10 macroblocks (one strip)
  buflen += JPEGCompress(p + buflen, swizz, pitch);
  if (line == 239) {
    buflen += JPEGEnd(p + buflen);
    eof = 1;
  } else {
    eof = 0;
  }

  // Copy YUYV data from DMA buffer into swizzle buffer
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 80; x++)
      swizz[x + y*pitch] = dmaBuff_[(y * 80 + x) * 4 + 3];
    
  // Advance through image a line at a time
  line++;
  if (line >= 240) {
    line = 0;
  }

  DMA_CDNE = 0;   // Clear done channel 0
  DMA_CINT = 0;   // Clear interrupt channel 0
}
