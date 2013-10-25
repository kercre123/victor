#include "anki/cozmo/robot/hal.h"
#include "cameras.h"

#define RESET_PIN     94
#define FRAME_WIDTH   (640)
#define FRAME_HEIGHT  (480)
#define FRAME_SIZE    (FRAME_WIDTH * FRAME_HEIGHT * 2)
#define DDR_BUFFER    __attribute__((section(".ddr_direct.bss")))

/**
 * NOTE: This camera has reset active high, instead of low like normal
 */

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static u32 I2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr,
          u32 regAddr);

      static u8 m_camWriteProto[] = I2C_PROTO_WRITE_8BA;

      static const unsigned short m_OV9653_VGA[][2] = {
        { 0x09, 0x10 },  // Set soft sleep mode
        { 0x0e, 0x00 },  // System clock options
        { 0x09, 0x01 },  // Output drive, soft sleep mode
        { 0x15, 0x00 },  // Slave mode, HREF vs HSYNC, signals negate
        { 0x3f, 0xa6 },  // Edge enhancement treshhold and factor
        { 0x41, 0x02 },  // Color matrix coeff double option
        { 0x42, 0x08 },  // Single frame out, banding filter
        { 0x16, 0x06 },
        { 0x33, 0xc0 },  // Reserved
        { 0x34, 0xbf },
        { 0xa8, 0x80 },
        { 0x96, 0x04 },
        { 0x8e, 0x00 },
        { 0x3c, 0x77 },  // HREF option, UV average
        { 0x8b, 0x06 },
        { 0x35, 0x91 },
        { 0x94, 0x88 },
        { 0x95, 0x88 },

        //{ 0x40, 0xc1 },  // Output range, RGB 555/565

        { 0x29, 0x2f },  // Analog BLC & regulator
        { 0x0f, 0x43 },  // HREF & ADBLC options
        { 0x13, 0xe5 },  // AGC/AEC options
        { 0x3d, 0x90 },  // Gamma selection, colour matrix, UV delay
        { 0x69, 0x80 },  // Manual banding filter MSB
        { 0x5c, 0x96 },  // Reserved up to 0xa5
        { 0x5d, 0x96 },
        { 0x5e, 0x10 },
        { 0x59, 0xeb },
        { 0x5a, 0x9c },
        { 0x5b, 0x55 },
        { 0x43, 0xf0 },
        { 0x44, 0x10 },
        { 0x45, 0x55 },
        { 0x46, 0x86 },
        { 0x47, 0x64 },
        { 0x48, 0x86 },
        { 0x5f, 0xe0 },
        { 0x60, 0x8c },
        { 0x61, 0x20 },
        { 0xa5, 0xd9 },
        { 0xa4, 0x74 },  // reserved
        { 0x8d, 0x02 },  // Color gain analog/_digital_
        { 0x13, 0xe7 },  // Enable AEC, AWB, AEC
        { 0x8c, 0x23 },  // Edge enhancement, denoising
        { 0xa9, 0xb8 },
        { 0xaa, 0x92 },
        { 0xab, 0x0a },
        { 0x8f, 0xdf },  // Digital BLC 
        { 0x90, 0x00 },  // Digital BLC B chan offset 
        { 0x91, 0x00 },  // Digital BLC R chan offset 
        { 0x9f, 0x00 },  // Digital BLC GB chan offset
        { 0xa0, 0x00 },
        { 0x14, 0x3a },  // Gain ceiling 16x

        // Frame size
        { 0x04, 0x00 },
        { 0x0c, 0x04 },
        { 0x0d, 0x80 },
        { 0x11, 0x01 },
        { 0x12, 0x40 },  // VGA selection

        { 0x17, 0x26 },  // HSTART == HREF start
        { 0x18, 0xc6 },  // HSTOP == HREF stop
        { 0x32, 0xed },  // HREF control
        { 0x19, 0x01 },  // VSTRT
        { 0x1a, 0x3d },  // VSTOP
        { 0x03, 0x00 },  // VREF
        { 0x2a, 0x10 },  // EXHCH == Dummy Pixel Insert MSB
        { 0x2b, 0x40 },  // EXHCL == Dummy Pixel Insert LSB
        { 0x37, 0x91 },  // ADC
        { 0x38, 0x12 },  // ACOM
        { 0x39, 0x43 },  // OFON
      };

      static CameraSpecification m_camSpecVGA = {
        YUV422p,          // type
        FRAME_WIDTH * 2,  // width
        FRAME_HEIGHT,     // height
        FRAME_WIDTH,      // stride
        2,                // bytesPP
        24,               // referenceFrequency
        (0x60 >> 1),      // sensorI2CAddress
        sizeof(m_OV9653_VGA) / (sizeof(short) * 2),  // registerCount
        m_OV9653_VGA      // regValues
      };

      static const tyI2cConfig m_i2cConfig = {
        GPIO_BITBASH,
        74,           // SCL GPIO
        75,           // SDA GPIO
        100,          // Speed in kHz
        ADDR_7BIT,    // Address size
        I2CErrorHandler
      };

      static const drvGpioInitArrayType m_cameraGPIO =
      {
        {
          74, 75, ACTION_UPDATE_ALL,   // I2C2 SDA, SCL
          PIN_LEVEL_LOW,
          D_GPIO_MODE_7             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_8mA      |
          D_GPIO_PAD_VOLT_2V5       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_OFF    |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },
        
        {
          114, 114, ACTION_UPDATE_ALL,  // CAM1_MCLK
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_OUT            |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_8mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_OFF    |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          115, 115, ACTION_UPDATE_ALL,  // CAM1_PCLK
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_ON     |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          116, 116, ACTION_UPDATE_ALL,  // CAM1_VSYNC
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_ON        |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_ON     |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          117, 117, ACTION_UPDATE_ALL,  // CAM1_HSYNC
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_ON     |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          118, 125, ACTION_UPDATE_ALL,  // CAM1 Data[7:0]
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_ON     |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          RESET_PIN, RESET_PIN, ACTION_UPDATE_ALL,  // CAM1 Reset
          PIN_LEVEL_LOW,              // Don't hold the camera in reset
          D_GPIO_MODE_7             |
          D_GPIO_DIR_OUT            |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_OFF    |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          91, 91, ACTION_UPDATE_ALL,  // CAM1 Power Down
          PIN_LEVEL_LOW,              // Don't hold the camera in power down
          D_GPIO_MODE_7             |
          D_GPIO_DIR_OUT            |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_1V8       |
          D_GPIO_PAD_SLEW_SLOW      |
          D_GPIO_PAD_SCHMITT_OFF    |
          D_GPIO_PAD_RECEIVER_ON    |
          D_GPIO_PAD_BIAS_2V5       |
          D_GPIO_PAD_LOCALCTRL_OFF  |
          D_GPIO_PAD_LOCALDATA_LO   |
          D_GPIO_PAD_LOCAL_PIN_OUT
        },

        {
          0, 0, ACTION_TERMINATE_ARRAY,
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0,
          D_GPIO_PAD_DEFAULTS
        }
      };

      static FrameBuffer m_cameraBuffer;
      static CameraHandle m_handle;
      static I2CM_Device m_i2c;

      static DDR_BUFFER u8 m_buffer[FRAME_SIZE];
      static DDR_BUFFER CameraSpecification m_cameraSpec;

      static volatile bool m_isFrameReady;

      static void FrameReady(FrameBuffer* fb)
      {
        m_isFrameReady = true;
      }

      static u32 I2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr,
          u32 regAddr)
      {
        if (i2cCommsError != I2CM_STAT_OK)
        {
          printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",
                i2cCommsError, slaveAddr, regAddr);
        }

        return i2cCommsError;
      }

      void FrontCameraInit()
      {
        // Initialize the camera GPIO
        DrvGpioInitialiseRange(m_cameraGPIO);

        // Initialize the camera buffer
        m_isFrameReady = false;

        m_cameraBuffer.p1 = m_buffer;
        m_cameraBuffer.p2 = m_buffer + FRAME_WIDTH * FRAME_HEIGHT;
        m_cameraBuffer.p3 = m_cameraBuffer.p2 + FRAME_WIDTH * FRAME_HEIGHT;

        int i;
        for (i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++)
          m_buffer[i] = 0xFF;

        // Initialize I2C
        if (DrvI2cMInitFromConfig(&m_i2c, &m_i2cConfig) != I2CM_STAT_OK)
        {
          printf("\nI2C INIT ERROR\n");
          while (1)
            ;
        }

        // Set the I2C Error handler
        DrvI2cMSetErrorHandler(&m_i2c, m_i2cConfig.errorHandler);

        // Verify camera communication (sort of) TODO: actually verify
        //DrvI2cMReadByte(&m_i2c, m_camSpecVGA.sensorI2CAddress, 0x3103);

        CameraInit(&m_handle, CAMERA_1, &m_i2c, &m_cameraBuffer, &m_camSpecVGA,
            FrameReady);
        CameraStart(&m_handle, RESET_PIN, false, m_camWriteProto);
      }

      u8* FrontCameraGetFrame()
      {
//        if (m_isFrameReady)
        {
          m_isFrameReady = false;
          return m_cameraBuffer.p1; //m_buffer;
        }

        return NULL;
      }
    }
  }
}

