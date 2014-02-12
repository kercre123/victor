#include "anki/cozmo/robot/hal.h"
#include "cameras.h"


/**
 * NOTE: This camera has reset active high, instead of low like normal
 */

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      static const u8 RESET_PIN = 94;

      static u32 I2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr,
          u32 regAddr);

      static u8 m_camWriteProto[] = I2C_PROTO_WRITE_8BA;

/*      static const unsigned short m_OV9653_VGA[][2] =
      {
        { 0x09, 0x10 },  // Set soft sleep mode
        { 0x0e, 0x80 },  // System clock options
        { 0x09, 0x01 },  // Output drive 2x
        { 0x15, 0x00 },  // Signals not negated
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
        { 0x11, 0x80 },  // Enable double clock option for PLL
        { 0x12, 0x40 },  // VGA selection

        { 0x17, 0x26 },  // HSTART == HREF start ~309
        { 0x18, 0xc6 },  // HSTOP == HREF stop ~1589  ... 1280
        { 0x32, 0xed },  // HREF control == {11,101,101}
        { 0x19, 0x01 },  // VSTRT ~8
        { 0x1a, 0x3d },  // VSTOP ~488
        { 0x03, 0x00 },  // VREF
        { 0x2a, 0x00 },  // EXHCH == Dummy Pixel Insert MSB
        { 0x2b, 0x80 },  // EXHCL == Dummy Pixel Insert LSB
        { 0x37, 0x91 },  // ADC
        { 0x38, 0x12 },  // ACOM
        { 0x39, 0x43 },  // OFON
        { 0x13, 0x00 },  // AGC/AEC options

        { 0x31, 0x00 },  // HSYNC Falling Edge Delay
        { 0x3a, 0x00 },  // YUYV output

        { 0x10, 0x20 },  // Set exposure to half of default (0x40)
      }; */

      static const u16 m_OV7725_VGA[] =
      {
        0x12, 0x80,
        0x3d, 0x03,
        0x12, 0x03,  // Bayer RAW
        0x17, 0x22,
        0x18, 0xa4,
        0x19, 0x07,
        0x1a, 0xf0,
        0x32, 0x00,
        0x29, 0xa0,
        0x2c, 0xf0,
        0x2a, 0x00,
        0x11, 0x02,  // pre-scaler

        0x15, 0x00,

        0x42, 0x7f,
        0x4d, 0x09,
        0x63, 0xe0,
        0x64, 0xff,
        0x65, 0x20,
        0x0c, 0x10, // flip Y with UV
        0x66, 0x00, // ... ?
        0x67, 0x4a,  // RAW8 Output Selection
        0x13, 0xf0,
        0x0d, 0x71, // PLL = 8x
        0x0f, 0xc5,
        0x14, 0x11,
        0x22, 0xff, // ff/7f/3f/1f for 60/30/15/7.5fps  -- banding filter
        0x23, 0x01, // 01/03/07/0f for 60/30/15/7.5fps
        0x24, 0x40,
        0x25, 0x30,
        0x26, 0xa1,
        0x2b, 0x00, // Dummy bytes LSB
        0x6b, 0xaa,
        0x13, 0xff,
        0x90, 0x05,
        0x91, 0x01,
        0x92, 0x03,
        0x93, 0x00,
        0x94, 0xb0,
        0x95, 0x9d,
        0x96, 0x13,
        0x97, 0x16,
        0x98, 0x7b,
        0x99, 0x91,
        0x9a, 0x1e,
        0x9b, 0x08,
        0x9c, 0x20,
        0x9e, 0x81,
        0xa6, 0x04,
        0x7e, 0x0c,
        0x7f, 0x16,
        0x80, 0x2a,
        0x81, 0x4e,
        0x82, 0x61,
        0x83, 0x6f,
        0x84, 0x7b,
        0x85, 0x86,
        0x86, 0x8e,
        0x87, 0x97,
        0x88, 0xa4,
        0x89, 0xaf,
        0x8a, 0xc5,
        0x8b, 0xd7,
        0x8c, 0xe8,
        0x8d, 0x20,
      };

/*      static CameraSpecification m_camSpecVGA = {
        BAYER_TO_Y,       // type
        FRAME_WIDTH,      // width
        FRAME_HEIGHT,     // height
        1,                // bytesPP
        24,               // referenceFrequency
        (0x42 >> 1),      // sensorI2CAddress
        sizeof(m_OV7725_VGA) / (sizeof(short) * 2),  // registerCount
        m_OV7725_VGA      // regValues
      }; */


/*      static u8 m_camWriteProto[] = I2C_PROTO_WRITE_16BA;

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
        {0x501f, 0x01},  // ISP CTRL1F == YUV422
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

//        {0x3804, 0x02},
//        {0x3805, 0x90},

        {0x4708, 0x01},  // DVP control

        {0x3008, 0x02},  // Enable output
      };

      static CameraSpecification m_camSpecVGA = {
        YUV422i,          // type
        FRAME_WIDTH * 2,  // width
        FRAME_HEIGHT,     // height
        FRAME_WIDTH,      // stride
        1,                // bytesPP
        24,               // referenceFrequency
        (0x78 >> 1),      // sensorI2CAddress
        sizeof(m_OV7739_VGA) / (sizeof(short) * 2),  // regNumber
        m_OV7739_VGA      // regValues
      }; */



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
          115, 115, ACTION_UPDATE_ALL,  // CAM1_PCLK
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0             |
          D_GPIO_DIR_IN             |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
          D_GPIO_PAD_VOLT_2V5       |
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
          D_GPIO_PAD_VOLT_2V5       |
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
          D_GPIO_PAD_VOLT_2V5       |
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
          D_GPIO_PAD_VOLT_2V5       |
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
          91, 91, ACTION_UPDATE_ALL,  // CAM1 Power Down
          PIN_LEVEL_LOW,              // Don't hold the camera in power down
          D_GPIO_MODE_7             |
          D_GPIO_DIR_OUT            |
          D_GPIO_DATA_INV_OFF       |
          D_GPIO_WAKEUP_OFF         |
          D_GPIO_IRQ_SRC_NONE,

          D_GPIO_PAD_NO_PULL        |
          D_GPIO_PAD_DRIVE_2mA      |
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
          0, 0, ACTION_TERMINATE_ARRAY,
          PIN_LEVEL_LOW,
          D_GPIO_MODE_0,
          D_GPIO_PAD_DEFAULTS
        }
      };

      static CameraHandle m_handle;
      static I2CM_Device m_i2c;

      static u32 I2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr,
          u32 regAddr)
      {
        if (i2cCommsError != I2CM_STAT_OK)
        {
		  // TODO: fix
          // printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",
          //     i2cCommsError, slaveAddr, regAddr);
        }

        return i2cCommsError;
      }

      void FrontCameraInit()
      {
        // Initialize the camera GPIO
        DrvGpioInitialiseRange(m_cameraGPIO);

        // Initialize I2C
        if (DrvI2cMInitFromConfig(&m_i2c, &m_i2cConfig) != I2CM_STAT_OK)
        {
		  // TODO: fix
          // printf("\nI2C INIT ERROR\n");
          while (1)
            ;
        }

        // Set the I2C Error handler
        DrvI2cMSetErrorHandler(&m_i2c, m_i2cConfig.errorHandler);

        // Verify camera communication (sort of) TODO: actually verify
        //DrvI2cMReadByte(&m_i2c, m_camSpecVGA.sensorI2CAddress, 0x3103);

        //CameraInit(&m_handle, CAMERA_HEAD, &m_i2c, &m_camSpecVGA, m_buffer,
        //    FrameReady);
        //CameraStart(&m_handle, RESET_PIN, true, m_camWriteProto);

        const u32 BYTES_PER_PIXEL = 1;
        const u32 REFERENCE_FREQUENCY_MHZ = 24;
        const u32 I2C_ADDRESS = (0x42 >> 1);
        const u32 VGA_REG_COUNT = sizeof(m_OV7725_VGA) / (sizeof(short) * 2);
        const u32 QVGA_REG_COUNT = 0;
        const u32 QQVGA_REG_COUNT = 0;
        const bool IS_ACTIVE_LOW = true;
        CameraInit(&m_handle, CAMERA_FRONT, BAYER_TO_Y, BYTES_PER_PIXEL,
            REFERENCE_FREQUENCY_MHZ, I2C_ADDRESS, &m_i2c, m_OV7725_VGA,
            VGA_REG_COUNT, NULL, QVGA_REG_COUNT, NULL, QQVGA_REG_COUNT,
            RESET_PIN, IS_ACTIVE_LOW, m_camWriteProto);
      }
    }
  }
}

