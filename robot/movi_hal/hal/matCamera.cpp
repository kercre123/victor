#include "anki/cozmo/robot/hal.h"
#include "cameras.h"

#define RESET_PIN     61
#define FRAME_WIDTH   (640)
#define FRAME_HEIGHT  (480)
#define FRAME_SIZE    (FRAME_WIDTH * FRAME_HEIGHT * 3 / 2)
#define DDR_BUFFER    __attribute__((section(".ddr_direct.bss")))

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
/*      static u32 I2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr,
          u32 regAddr);
      
      static u8 m_camWriteProto[] = I2C_PROTO_WRITE_16BA;

      static const unsigned short m_OV7739_VGA[][2] = {
        {0x3008, 0x82},  // Reset
        {0x3008, 0x42},  // Software sleep power down mode ??
        {0x3104, 0x03},
        {0x3017, 0x7f},  // PAD OEN01
        {0x3018, 0xfc},  // PAD OEN02

        {0x3602, 0x14},  // Analog control regs, according to movidius
        {0x3611, 0x44},
        {0x3631, 0x22},
        {0x3622, 0x00},  // ANA ARRAY02
        {0x3633, 0x25},
        {0x370d, 0x04},  // ARRAY CTRL02
        {0x3620, 0x32},
        {0x3714, 0x2c},
        {0x401c, 0x00},
        {0x401e, 0x11},
        {0x4702, 0x01},
        {0x5000, 0x0e},  // ISP CTRL00 == AWG gain enable | black pixel cancellation enable | white pixel cancellation enable | color interpolation disable
        {0x5001, 0x01},  // ISP CTRL01 == Auto white balance enable
        {0x3a00, 0x7a},  // AEC CTRL00 == less 1 line function able | band enable | less 1 band enable | night mode disable | freeze disable
        {0x3a18, 0x00},  // AEC GAIN CEILING [8]
        {0x3a19, 0x3f},  // AEC GAIN CEILING [7:0]
        {0x300f, 0x88},  // PLL1 CTRL00 == Div2 | 001
        {0x3011, 0x08},  // PLL1 CTRL02 == PLL1 bypass
        {0x4303, 0xff},  // YMAX VALUE [7:0]
        {0x4307, 0xff},  // UMAX VALUE [7:0]
        {0x430b, 0xff},  // VMAX VALUE [7:0]
        {0x4305, 0x00},  // YMIN VALUE [7:0]
        {0x4309, 0x00},  // UMIN VALUE [7:0]
        {0x430d, 0x00},  // VMIN VALUE [7:0]
        {0x5000, 0x4f},  // ISP CTRL00 == Gamma enable | black pixel cancellation enable | white pixel cancellation enable | color interpolation enable
        {0x5001, 0x47},  // ISP CTRL01 == UV average enable | color matrix enable | auto white balance enable
        {0x4300, 0x30},  // FORMAT CTRL00 == YUV422
        {0x4301, 0x80},
        {0x501f, 0x00},  // ISP CTRL1F ==  ISP RAW // YUV422
        {0x3008, 0x02},  // Debug mode ?
        {0x5180, 0x02},  // AWB CTRL00
        {0x5181, 0x02},  // AWB CTRL01
        {0x3a0f, 0x35},  // AEC CONTROL 0F
        {0x3a10, 0x2c},  // AEC CONTROL 10
        {0x3a1b, 0x36},  // AEC CONTROL 1B
        {0x3a1e, 0x2d},  // AEC CONTROL 1E
        {0x3a11, 0x90},  // AEC CONTROL 11
        {0x3a1f, 0x10},  // AEC CONTROL 1F
        {0x5000, 0xcf},  // ISP CTRL00 == LENC correction enable | gamma enable | black pixel cancellation enable | white pixel cancellation enable | color interpolation enable
        {0x5481, 0x0a},  // GAMMA YST1
        {0x5482, 0x13},  // GAMMA YST2
        {0x5483, 0x23},  // GAMMA YST3
        {0x5484, 0x40},  // GAMMA YST4
        {0x5485, 0x4d},  // GAMMA YST5
        {0x5486, 0x58},  // GAMMA YST6
        {0x5487, 0x64},  // GAMMA YST7
        {0x5488, 0x6e},  // GAMMA YST8
        {0x5489, 0x78},  // GAMMA YST9
        {0x548a, 0x81},  // GAMMA YST10
        {0x548b, 0x92},  // GAMMA YST11
        {0x548c, 0xa1},  // GAMMA YST12
        {0x548d, 0xbb},  // GAMMA YST13
        {0x548e, 0xcf},  // GAMMA YST14
        {0x548f, 0xe3},  // GAMMA YST15
        {0x5490, 0x26},  // GAMMA YSLP15
        {0x5380, 0x42},  // CMX COEFFICIENT11
        {0x5381, 0x33},  // CMX COEFFICIENT12
        {0x5382, 0x0f},  // CMX COEFFICIENT13
        {0x5383, 0x0b},  // CMX COEFFICIENT14
        {0x5384, 0x42},  // CMX COEFFICIENT15
        {0x5385, 0x4d},  // CMX COEFFICIENT16
        {0x5392, 0x1e},  // CMX SIGN
        {0x5801, 0x00},  // LENC CTRL1
        {0x5802, 0x06},  // LENC CTRL2
        {0x5803, 0x0a},  // LENC CTRL3
        {0x5804, 0x42},  // LENC CTRL4
        {0x5805, 0x2a},  // LENC CTRL5
        {0x5806, 0x25},  // LENC CTRL6
        {0x5001, 0xc7},  // ISP CTRL01
        {0x5580, 0x02},  // SDE CTRL0
        {0x5583, 0x40},  // SDE CTRL3
        {0x5584, 0x26},  // SDE CTRL4
        {0x5589, 0x10},  // SDE CTRL9
        {0x558a, 0x00},  // SDE CTRL10
        {0x558b, 0x3e},  // SDE CTRL11
        {0x5300, 0x0f},  // CIP CTRL0
        {0x5301, 0x30},  // CIP CTRL1
        {0x5302, 0x0d},  // CIP CTRL2
        {0x5303, 0x02},  // CIP CTRL3
        {0x5304, 0x0e},  // CIP CTRL4
        {0x5305, 0x30},  // CIP CTRL5
        {0x5306, 0x06},  // CIP CTRL6
        {0x5307, 0x40},  // CIP CTRL7
        {0x5680, 0x00},
        {0x5681, 0x50},
        {0x5682, 0x00},
        {0x5683, 0x3c},
        {0x5684, 0x11},
        {0x5685, 0xe0},
        {0x5686, 0x0d},
        {0x5687, 0x68},
        {0x5688, 0x03},  // AVERAGE CTRL8

        {0x4708, 0x00},  // DVP control

        {0x3008, 0x02},  // Enable output
      };

      static CameraSpecification m_camSpecVGA =
      {
        BAYER,            // type
        FRAME_WIDTH,      // width
        FRAME_HEIGHT,     // height
        2,                // bytesPP
        24,               // referenceFrequency
        (0x78 >> 1),      // sensorI2CAddress
        sizeof(m_OV7739_VGA) / (sizeof(short) * 2),  // regNumber
        m_OV7739_VGA      // regValues
      };

      static const tyI2cConfig m_i2cConfig =
      {
        GPIO_BITBASH,
        65,           // SCL GPIO
        66,           // SDA GPIO
        100,          // Speed in kHz
        ADDR_7BIT,    // Address size
        I2CErrorHandler
      };

      static const drvGpioInitArrayType m_cameraGPIO =
      {
        {
          65, 66, ACTION_UPDATE_ALL,   // I2C1 SDA, SCL
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
          49, 49, ACTION_UPDATE_ALL,    // CAM2_MCLK
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
          50, 50, ACTION_UPDATE_ALL,    // CAM2_PCLK
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
          51, 51, ACTION_UPDATE_ALL,    // CAM2_VSYNC
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
          52, 52, ACTION_UPDATE_ALL,    // CAM2_HSYNC
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
          53, 60, ACTION_UPDATE_ALL,    // CAM2 Data[7:0]
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
          RESET_PIN, RESET_PIN, ACTION_UPDATE_ALL,  // CAM1 Reset_n
          PIN_LEVEL_HIGH,             // Don't hold the camera in reset
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
          63, 63, ACTION_UPDATE_ALL,  // CAM2 Power Down
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

      static CameraHandle m_handle;
      static I2CM_Device m_i2c;

      static DDR_BUFFER u8 m_buffer[FRAME_SIZE];
      static DDR_BUFFER CameraSpecification m_cameraSpec;

      static volatile bool m_isFrameReady;

      static void FrameReady()
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

      void MatCameraInit()
      {
        // Initialize the camera GPIO
        DrvGpioInitialiseRange(m_cameraGPIO);

        // Initialize the camera buffer
        m_isFrameReady = false;

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

        CameraInit(&m_handle, CAMERA_MAT, &m_i2c, &m_camSpecVGA, m_buffer,
            FrameReady);
        CameraStart(&m_handle, RESET_PIN, true, m_camWriteProto);
      }

      const u8* MatCameraGetFrame()
      {
//        if (m_isFrameReady)
        {
          m_isFrameReady = false;
          return m_buffer;
        }

        return NULL;
      } */
    }
  }
}

