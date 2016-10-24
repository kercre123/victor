#include <string.h>
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"

#include "hal/portable.h"

#include "hal/i2c.h"
#include "hal/hardware.h"
#include "hal/spi.h"
#include "hal/uart.h"

#include "anki/cozmo/robot/logging.h"

#include "anki/cozmo/robot/drop.h"
extern DropToWiFi* spi_write_buff;  // To save RAM, we write directly into spi_write_buff

#define ENABLE_JPEG     // Comment this out to troubleshoot timing problems caused by JPEG encoder
//#define TEST_VIDEO      // When JPEG encoder is disabled, uncomment this to send JPEG test video anyway
//#define SERIAL_IMAGE    // Uncomment this to dump camera data over UART for camera debugging with SerialImageViewer

// Camera reads lines at 7.44khz or 1 line in 0.1344ms
#define LINES_TO_MS(lines) ((u16)((lines) * (0.1344086022)))
#define MS_TO_LINES(ms) ((u16)((ms) * (7.44)))
// Convert to fixed point u8 with 6 places after the decimal (2^6 = 64)
// as defined by the camera specs for the format of gain
#define GAIN_TO_FIXED_POINT(gain) ((u8)((gain) * (64)))
// 1/64 = 0.015625
#define FIXED_POINT_TO_GAIN(fp) ((f32)((fp) * (0.015625)))

static u32 frameNumber = 0;
static u16 line = 0;

extern volatile bool is_write_buff_final;   // See spi.cpp for details (this deserves a better fix)

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Configuration for GC0329 camera chip
      const u8 CAM_SCRIPT[] =
      {
        #include "gc0329.h"
        0, 0
      };

      const int TOTAL_COLS = 640, TOTAL_ROWS = 480, BYTES_PER_PIX = 4;

      // Camera exposure in lines
      u16 exposure_lines_ = 0;
      // Gain in fixed point format
      u8  gain_ = 0;
      const u16 MAX_EXPOSURE_LINES = 500;
      // Gain is a byte so max gain is fixed point 255 converted to float
      const f32 MAX_GAIN = FIXED_POINT_TO_GAIN(255);
      volatile bool timingSynced_ = false;

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
        SOURCE_SETUP(GPIO_CAM_D1, SOURCE_CAM_D1, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D2, SOURCE_CAM_D2, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D3, SOURCE_CAM_D3, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D4, SOURCE_CAM_D4, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D5, SOURCE_CAM_D5, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D6, SOURCE_CAM_D6, SourceGPIO);
        SOURCE_SETUP(GPIO_CAM_D7, SOURCE_CAM_D7, SourceGPIO);

        // Drive PWDN and RESET to safe defaults
        GPIO_SET(GPIO_CAM_PWDN, PIN_CAM_PWDN);
        GPIO_OUT(GPIO_CAM_PWDN, PIN_CAM_PWDN);
        SOURCE_SETUP(GPIO_CAM_PWDN, SOURCE_CAM_PWDN, SourceGPIO);

        GPIO_RESET(GPIO_CAM_RESET_N, PIN_CAM_RESET_N);
        GPIO_OUT(GPIO_CAM_RESET_N, PIN_CAM_RESET_N);
        SOURCE_SETUP(GPIO_CAM_RESET_N, SOURCE_CAM_RESET_N, SourceGPIO);

        // Set up HSYNC to trigger DMA start on rising edge
        SOURCE_SETUP(GPIO_CAM_HSYNC, SOURCE_CAM_HSYNC, SourceGPIO | SourceDMARise);

        // Configure XCLK (on FTM1) for bus clock / 2 - fastest we can go (24 MHz)
        SIM_SCGC6 |= SIM_SCGC6_FTM1_MASK;   // Enable FTM1
        FTM1_SC = 0;              // Reset
        FTM1_MOD = 1;             // Minimum timer period
        FTM1_C0SC = FTM_CnSC_ELSB_MASK|FTM_CnSC_MSB_MASK;   // Edge-aligned PWM on CH0
        FTM1_C0V = 1;                                       // 50% duty cycle on CH0
        FTM1_SC = FTM_SC_CLKS(1); // Use bus clock with a /1 prescaler
        SOURCE_SETUP(GPIO_CAM_XCLK, SOURCE_CAM_XCLK, SourceAlt3);
      }

      volatile u8 eof_ = 0;

      void HALExec(void);
      void HALSafe(void);
      void HALInit(void);

      u8* const dmaBuff_ = (u8*)0x20000000;       // Start of RAM buffer
      void JPEGCompress(int line, int height);
        
      static void InitDMA();

      // Set up camera
      static void InitCam()
      {
        // Power-up/reset the camera
        MicroWait(50);
        GPIO_RESET(GPIO_CAM_PWDN, PIN_CAM_PWDN);
        MicroWait(50);
        GPIO_SET(GPIO_CAM_RESET_N, PIN_CAM_RESET_N);

        I2C::ReadReg(CAMERA_ADDR, 0xF0);
        I2C::ReadReg(CAMERA_ADDR, 0xF1);
        uint8_t id = I2C::ReadReg(CAMERA_ADDR, 0xFB);

        // Send command array to camera
        uint8_t* initCode = (uint8_t*) CAM_SCRIPT;

        for(;;) {
          uint8_t p1 = *(initCode++), p2 = *(initCode++);

          if (!p1 && !p2) break ;

          I2C::WriteReg(CAMERA_ADDR, p1, p2);
        }

        // TODO: Check that the GPIOs are okay
        //for (u8 i = 1; i; i <<= 1)
        //  printf("\r\nCam dbus: set %x, got %x", i, CamReadDB(i));
      }
      
      const int DMAMUX_PORTA = 49;    // This is not in the K02 .h files for some reason

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
        DMA_TCD0_NBYTES_MLNO = TOTAL_COLS*BYTES_PER_PIX;  // Number of transfers in minor loop
        DMA_TCD0_ATTR = DMA_ATTR_SSIZE(0)|DMA_ATTR_DSIZE(0);      // Source 8-bit, dest 8-bit
        DMA_TCD0_SOFF = 0;          // Source (register) doesn't increment
        DMA_TCD0_SADDR = (uint32_t)&CAMERA_DATA_GPIO;
        DMA_TCD0_DOFF = 1;          // Destination (buffer) increments
        DMA_TCD0_DADDR = (uint32_t)dmaBuff_;
        DMA_TCD0_CITER_ELINKNO = 1; // Current major loop iteration (1 per interrupt)
        DMA_TCD0_BITER_ELINKNO = 1; // Beginning major loop iteration (1 per interrupt)
        DMA_TCD0_DLASTSGA = (uint32_t)-TOTAL_COLS*BYTES_PER_PIX;    // Point back at start of buffer after each loop

        // Hook DMA start request 0 to HSYNC
        DMAMUX_CHCFG0 = DMAMUX_CHCFG_ENBL_MASK | (DMAMUX_PORTA + PORT_INDEX(GPIO_CAM_HSYNC));
        DMA_ERQ = DMA_ERQ_ERQ0_MASK;

        // Set up FTM IRQ to match hsync - must match gc0329.h timing!
        static const uint16_t CLOCK_MOD = (168 * 8) * (BUS_CLOCK / I2SPI_CLOCK) - 1;
        static const uint16_t DISABLE_MOD = (uint16_t)(CLOCK_MOD * 0.5f);

        SIM_SCGC6 |= SIM_SCGC6_FTM2_MASK;
        FTM2_SC = 0;    // Make sure timer is disabled

        /* Not currently used
        FTM2_C0V = 8 * (BUS_CLOCK / I2SPI_CLOCK) / 2; // 50% time disable I2C interrupt
        FTM2_C0SC = FTM_CnSC_CHIE_MASK |
                    //FTM_CnSC_ELSA_MASK |
                    //FTM_CnSC_ELSB_MASK |
                    //FTM_CnSC_MSA_MASK |
                    FTM_CnSC_MSB_MASK ;
        */

        FTM2_MOD = (168 * 8) * (BUS_CLOCK / I2SPI_CLOCK) - 1;   // 168 bytes at I2S_CLOCK
        FTM2_CNT = FTM2_CNTIN = 0; //8 * (BUS_CLOCK / I2SPI_CLOCK); // Place toward center of transition
        FTM2_CNTIN = 0;

        FTM2_SYNCONF = FTM_SYNCONF_SWRSTCNT_MASK;
        FTM2_SYNC = FTM_SYNC_SWSYNC_MASK;   // Force all registers to be loaded
      }

      // Initialize camera
      void CameraInit()
      {
        timingSynced_ = false;

        InitIO();
        InitCam();
        
        // Exposure as defined in gc0329.h
        exposure_lines_ = (CAM_SCRIPT[29] << 8) | CAM_SCRIPT[31];
      }
      
      // Start streaming data from the camera - after this point, the main thread can't touch registers
      void CameraStart()
      {
        InitDMA();
        while (!timingSynced_)  ;
      }
      
      void CameraGetDefaultParameters(DefaultCameraParams& params)
      {
        params.minExposure_ms = LINES_TO_MS(((CAM_SCRIPT[511] << 8) | CAM_SCRIPT[513]) & 0xFFF);
        params.maxExposure_ms = LINES_TO_MS(MAX_EXPOSURE_LINES);
        params.maxGain = MAX_GAIN;
        params.gain = FIXED_POINT_TO_GAIN(CAM_SCRIPT[9]);
        
        for(u8 i = 0; i < sizeof(params.gammaCurve)/sizeof(params.gammaCurve[0]); ++i)
        {
          params.gammaCurve[i] = CAM_SCRIPT[143 + (i*2)];
        }
      }

      void CameraSetParameters(u16 exposure_ms, f32 gain)
      {
        const u16 exposure = MS_TO_LINES(exposure_ms);
        const u8 fixedPointGain = GAIN_TO_FIXED_POINT(gain);
        
        // In an attempt to minimize the amount of specialized code in I2C.cpp we can only write to exposure or gain one at a time
        if(exposure != exposure_lines_)
        {
          exposure_lines_ = exposure;
        
          if(exposure_lines_ > MAX_EXPOSURE_LINES)
          {
            AnkiWarn(143, "Camera.CameraSetParameters", 395, "Clipping exposure of %dms to %dms", 2, exposure_ms, LINES_TO_MS(MAX_EXPOSURE_LINES));
            exposure_lines_ = MAX_EXPOSURE_LINES;
          }
          
          // Write to exposure to camera
          I2C::SetCameraExposure(exposure_lines_);
        }
        else if(gain_ != fixedPointGain)
        {
          gain_ = fixedPointGain;
          
          // Write gain to camera
          I2C::SetCameraGain(gain_);
        }
      }
      
      const CameraInfo* GetHeadCamInfo(void)
      {
        const u16 HEAD_CAM_CALIB_WIDTH  = 400;
        const u16 HEAD_CAM_CALIB_HEIGHT = 296;

        // @TODO get stuff from the Espressif flash storage if nessisary
        static HAL::CameraInfo camCal = {
          280.0,  // focalLength_x
          280.0,  // focalLength_y
          HEAD_CAM_CALIB_WIDTH/2.0f,  // center_x
          HEAD_CAM_CALIB_HEIGHT/2.0f,  // center_y
          0.f,
          HEAD_CAM_CALIB_HEIGHT,
          HEAD_CAM_CALIB_WIDTH
        };

        return &camCal;
      }
			
      u16 CameraGetScanLine()
      {
        return line;
      }
      
      u32 CameraGetFrameNumber()
      {
        return frameNumber;
      }
      
      u16 CameraGetExposureDelay()
      {
        return exposure_lines_;
      }
    }
  }
}

// This is triggered on camera DMA complete - but does not trigger during vblank
// So, we setup an FTM to trigger repeatedly at just the right time
extern "C"
void DMA0_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;

  DMA_CDNE = DMA_CDNE_CDNE(0); // Clear done channel 0
  DMA_CINT = 0;   // Clear interrupt channel 0

  // The camera will send one entire frame (around 480 lines) at the wrong rate
  // So let that frame pass before we attempt to synchronize
  static u16 lineskip = TOTAL_ROWS;
  if (lineskip--)
    return;

  // Shut off DMA IRQ - we'll use FTM IRQ from now on
  DMA_TCD0_CSR = 0;

  // Sync to falling edge of I2SPI word select
  while(~GPIOD_PDIR & (1 << 4)) ;
  while( GPIOD_PDIR & (1 << 4)) ;

  // Turn on FTM right after sync
  FTM2_SC = FTM_SC_TOF_MASK |
          FTM_SC_TOIE_MASK |
          FTM_SC_CLKS(1) | // Select BUS_CLOCK - this enables counting
          FTM_SC_PS(0);

  timingSynced_ = true;
  NVIC_EnableIRQ(FTM2_IRQn);
  NVIC_SetPriority(FTM2_IRQn, 1);
  NVIC_SetPriority(PendSV_IRQn, 0xFF);  // Lowest possible priority
  HALInit();
}

extern "C"
void FTM2_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;

  // THIS HAPPENS IN SPI.CPP FOR NOW UNTIL WE FIGURE OUT WHY THIS ISN'T FIRING
  /*
  if (FTM2_C0SC & FTM_CnSC_CHF_MASK) {
    FTM2_C0SC &= ~FTM_CnSC_CHF_MASK;
    I2C::Disable();
  }

  // Enable SPI DMA, Clear flag
  if (~FTM2_SC & FTM_SC_TOF_MASK) return ;
  */

  DMA_CR = DMA_CR_CLM_MASK | DMA_CR_CX_MASK;  // Cancel camera DMA (in case it's out of sync).

  // QVGA subsample - TODO: Make dynamic
  if (line & 1) {
    // After receiving odd line, need to turn DMA back on
    DMA_TCD0_DOFF = 1;
    // Per erratum e8011: Repeat writes to SADDR, DADDR, or NBYTES until they stick
    do
      DMA_TCD0_DADDR = (uint32_t)dmaBuff_;
    while (DMA_TCD0_DADDR != (uint32_t)dmaBuff_);
  } else {        
    DMA_TCD0_DOFF = 0;
  }
  
  // Do this as early as possible - but after DMA is set up
  SPI::StartDMA();

  // Acknowledge timer interrupt now (we won't get time to later)
  FTM2_SC &= ~FTM_SC_TOF_MASK;

  // Cheesy way to check for start of frame
  // If we're past end of frame, but we saw a pixel change, reset line to 0
  // TODO:  The correct way is to just ask the DMA controller if it got triggered
  if (line >= TOTAL_ROWS && 1 != dmaBuff_[BYTES_PER_PIX*(TOTAL_COLS-1)])
  {
    line = 0;
    frameNumber++;
  }
  dmaBuff_[BYTES_PER_PIX*(TOTAL_COLS-1)] = 1;

  // Run all the register-hitting stuff
  HALExec();

  // Don't touch registers or dmabuff_ after this point!
  HALSafe();
  
  // Kick-off the JPEG encoder at a lower priority
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

// Run JPEG encoder at a lower priority
// If the JPEG encoder runs long, an entire line will be dropped from the image
extern "C"
void PendSV_Handler(void)
{
  using namespace Anki::Cozmo::HAL;

  SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;  // Acknowledge the interrupt

  // If write buffer is already full, there's no room for JPEG data, so just skip it
  if (is_write_buff_final)  return;
  
  // Run the JPEG encoder for all of the remaining time
  int eof = 0, buflen;   
#if defined(ENABLE_JPEG)
  if (line < 498 && IsVideoEnabled())   // XXX: This is apparently compensating for a JPEGCompress bug
    JPEGCompress(line, TOTAL_ROWS);
  else
    Anki::Cozmo::HAL::SPI::FinalizeDrop(0, 0, 0);
#else
  // If JPEG encoder is disabled, try various test modes
  #ifdef TEST_VIDEO
    static u8 frame = 0;
    if (0==(line&7) && line < TOTAL_ROWS) {
      for (int i = 0; i < 20; i++)
        spi_write_buff->payload[i] = ((frame + i) % 20) > (line > TOTAL_ROWS/2 ? 10 : 8) ? 0x4a : 0x5a;
      Anki::Cozmo::HAL::SPI::FinalizeDrop(20, false, frameNumber);
    } else if (line == TOTAL_ROWS) {
      *((int*)spi_write_buff->payload) = 0xffffff4a;
      Anki::Cozmo::HAL::SPI::FinalizeDrop(4, true, frameNumber);
      frame++;
      if (frame >= 20)
        frame = 0;
    } else
      Anki::Cozmo::HAL::SPI::FinalizeDrop(0, 0, frameNumber);
  #else
    
  // Video streaming disabled - stream nothing at all
  Anki::Cozmo::HAL::SPI::FinalizeDrop(0, 0, frameNumber);

  #ifdef SERIAL_IMAGE
    static int pclkoffset = 0;
    // At 3mbaud, during 60% time, can send about 20 bytes per line, or 160x60
    if (line < TOTAL_ROWS)
      for (int i = 0; i < 20; i++)
        UART::DebugPutc(dmaBuff_[((line & 7) * 20 + i) * 16 + 3 + (pclkoffset >> 4)]);

    // Write header for start of next frame
    if (hline == TOTAL_ROWS)
    {
      UART::DebugPutc(0xBE);
      UART::DebugPutc(0xEF);
      UART::DebugPutc(0xF0);
      UART::DebugPutc(0xFF);
      UART::DebugPutc(0xBD);
      // pclkoffset++;
    }
  #endif  
  #endif
#endif

  // Advance line pointer
  line++;
}
