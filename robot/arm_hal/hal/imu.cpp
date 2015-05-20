
// XXX TODO: PORT THIS CODE TO I2C

/**
* File: imu.cpp
*
* Author: Bryan Hood
* Created: 4/8/2014
*
* Description:
*
*  Driver file for BMI055 IMU
*
*  First, initialize the IMU with
*        void IMUInit();
*  Next, read data with
*        void IMUReadData(IMU_DataStructure &IMUData);
*
*  IMU_DataStructure contains 3 axes each of acceleration and gyro data
*     struct IMU_DataStructure
*     {
*        f32 acc_x;      // mm/s/s    
*        f32 acc_y;
*        f32 acc_z;
*        f32 rate_x;     // rad/s
*        f32 rate_y;
*        f32 rate_z;
*     };
*
*  Implementation notes:
*     - Need 2us delays between consecutive writes (in normal power mode)
*     - Need to toggle CS after read/write
*     - For multiple read, no CS toggle is required. Address auto-increments
* Copyright: Anki, Inc. 2013
*
**/

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"

// SPI READ/WRITE bits
#define IMU_READ            0x80
#define IMU_WRITE           0x00

// IMU Chip IDs
#define ACC_CHIPID          0xFA
#define GYRO_CHIPID         0x0F

// Accelerometer Register Map
#define ACC_BGW_CHIPID      0x00
#define ACC_ACCD_X_LSB      0x02
#define ACC_ACCD_X_MSB      0x03
#define ACC_ACCD_Y_LSB      0x04
#define ACC_ACCD_Y_MSB      0x05
#define ACC_ACCD_Z_LSB      0x06
#define ACC_ACCD_Z_MSB      0x07
#define ACC_ACCD_TEMP       0x08
#define ACC_INT_STATUS_0    0x09
#define ACC_INT_STATUS_1    0x0A
#define ACC_INT_STATUS_2    0x0B
#define ACC_INT_STATUS_3    0x0C
#define ACC_FIFO_STATUS     0x0E
#define ACC_PMU_RANGE       0x0F
#define ACC_PMU_BW          0x10
#define ACC_PMU_LPW         0x11
#define ACC_PMU_LOW_POWER   0x12
#define ACC_ACCD_HBW        0x13
#define ACC_BGW_SOFTRESET   0x14
#define ACC_INT_EN_0        0x16
#define ACC_INT_EN_1        0x17
#define ACC_INT_EN_2        0x18
#define ACC_INT_MAP_0       0x19
#define ACC_INT_MAP_1       0x1A
#define ACC_INT_MAP_2       0x1B
#define ACC_INT_SRC         0x1E
#define ACC_INT_OUT_CTRL    0x20
#define ACC_INT_RST_LATCH   0x21
#define ACC_INT_0           0x22
#define ACC_INT_1           0x23
#define ACC_INT_2           0x24
#define ACC_INT_3           0x25
#define ACC_INT_4           0x26
#define ACC_INT_5           0x27
#define ACC_INT_6           0x28
#define ACC_INT_7           0x29
#define ACC_INT_8           0x2A
#define ACC_INT_9           0x2B
#define ACC_INT_A           0x2C
#define ACC_INT_B           0x2D
#define ACC_INT_C           0x2E
#define ACC_INT_D           0x2F
#define ACC_FIFO_CONFIG_0   0x30
#define ACC_PMU_SELF_TEST   0x32
#define ACC_TRIM_NVM_CTRL   0x33
#define ACC_BGW_SPI3_WDT    0x34
#define ACC_OFC_CTRL        0x36
#define ACC_OFC_SETTING     0x37
#define ACC_OFC_OFFSET_X    0x38
#define ACC_OFC_OFFSET_Y    0x39
#define ACC_OFC_OFFSET_Z    0x3A
#define ACC_TRIM_GP0        0x3B
#define ACC_TRIM_GP1        0x3C
#define ACC_FIFO_CONFIG_1   0x3E
#define ACC_FIFO_DATA       0x3F

// Accelerometer Register values
#define RANGE_2G            0x03
#define RANGE_4G            0x05
#define RANGE_8G            0x08
#define RANGE_16G           0x0B
#define BW_250              0x0D

#define ACC_INT_OPEN_DRAIN  0x0F  // Active high, open drain

// Accelerometer Masks
#define ACC_LSB_MASK        0xF0

// Accelerometer Constants
// 12 bits, +/- 2g   4g/2^12 = 4*9.81 m/s^2 / 4096 ~= 0.958 mm/s^2 / LSB
#define RANGE_CONST_2G      9.58f

// Gyro Register Map
#define GYRO_CHIP_ID        0x00
#define GYRO_RATE_X_LSB     0x02
#define GYRO_RATE_X_MSB     0x03
#define GYRO_RATE_Y_LSB     0x04
#define GYRO_RATE_Y_MSB     0x05
#define GYRO_RATE_Z_LSB     0x06
#define GYRO_RATE_Z_MSB     0x07
#define GYRO_INT_STATUS_0   0x09
#define GYRO_INT_STATUS_1   0x0A
#define GYRO_INT_STATUS_2   0x0B
#define GYRO_INT_STATUS_3   0x0C
#define GYRO_FIFO_STATUS    0x0E
#define GYRO_RANGE          0x0F
#define GYRO_BW             0x10
#define GYRO_LPM1           0x11
#define GYRO_LPM2           0x12
#define GYRO_RATE_HBW       0x13
#define GYRO_BGW_SOFTRESET  0x14
#define GYRO_INT_EN_0       0x15
#define GYRO_INT_EN_1       0x16
#define GYRO_INT_MAP_0      0x17
#define GYRO_INT_MAP_1      0x18
#define GYRO_INT_MAP_2      0x19
#define GYRO_REG_1A         0x1A
#define GYRO_REG_1B         0x1B
#define GYRO_REG_1C         0x1C
#define GYRO_REG_1E         0x1E
#define GYRO_INT_RST_LATCH  0x21
#define GYRO_HIGH_TH_X      0x22
#define GYRO_HIGH_DUR_X     0x23
#define GYRO_HIGH_TH_Y      0x24
#define GYRO_HIGH_DUR_Y     0x25
#define GYRO_HIGH_TH_Z      0x26
#define GYRO_HIGH_DUR_Z     0x27
#define GYRO_SOC            0x31
#define GYRO_A_FOC          0x32
#define GYRO_TRIM_NVM_CTRL  0x33
#define GYRO_BGW_SPI3_WDT   0x34
#define GYRO_OFC1           0x36
#define GYRO_OFC2           0x37
#define GYRO_OFC3           0x38
#define GYRO_OFC4           0x39
#define GYRO_TRIM_GP0       0x3A
#define GYRO_TRIM_GP1       0x3B
#define GYRO_BIST           0x3C
#define GYRO_FIFO_CONFIG_0  0x3D
#define GYRO_FIFO_CONFIG_1  0x3E
#define GYRO_FIFO_DATA      0x3F

// Gyro Register Values
#define RANGE_125DPS        0x04
#define RANGE_250DPS        0x03
#define RANGE_500DPS        0x02
#define RANGE_1000DPS       0x01
#define RANGE_2000DPS       0x00


// 16 bits, +/- 500 deg 1000/2^16 ~= 0.01526 deg / LSB = 2.663E-4 rad/LSB
#define RANGE_CONST_500D    2.663E-4f

#define POST_WRITE_DELAY_US 2

//-8.5
//10.9

#undef assert
#define assert(x) while(!(x)) ;

enum IMU_DEVICE {
  IMU_GYRO = 0,
  IMU_ACC = 1
};

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Define SPI pins with macros
      GPIO_PIN_SOURCE(IMU_SCL, GPIOE, 3);
      GPIO_PIN_SOURCE(IMU_SDA, GPIOE, 5);
      GPIO_PIN_SOURCE(IMU_INT, GPIOE, 12);
      
      // Initialize spi
      static void InitI2C()
      {
        // Enable peripheral clock
        
        // Enable GPIO clocks
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

        // Peripherals alternate function

        // Initalize Pins
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;


        // Set Interupt input pin
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Pin = PIN_IMU_INT; 
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Init(GPIO_IMU_INT, &GPIO_InitStructure); 
              
        // Enable I2C peripheral
      }


      // Initialize accelerometer
      static void InitAcc()
      {
        uint8_t data = 0;
        // Do a communication test
        // Select accelerometer
        while (data != ACC_CHIPID)
        {
        // Get chip id
        //IMUWriteReadPipelined(IMU_READ | ACC_BGW_CHIPID);  // read data lags two IMUWriteReadPipelined calls
        //data = IMUWriteReadPipelined();
        }
        // Assert that chipid is correct
        assert(data == ACC_CHIPID);

        // Configure accelerometer
        // Select accelerometer
        //IMUWriteReadPipelined(IMU_WRITE | ACC_PMU_RANGE);
        //IMUWriteReadPipelined(RANGE_2G);
        MicroWait(POST_WRITE_DELAY_US);
        //IMUWriteReadPipelined(IMU_WRITE | ACC_PMU_BW); 
        //IMUWriteReadPipelined(BW_250);
        MicroWait(POST_WRITE_DELAY_US);
        //IMUWriteReadPipelined(IMU_WRITE | ACC_INT_OUT_CTRL);  // Set all I/O to open drain
        //IMUWriteReadPipelined(ACC_INT_OPEN_DRAIN);
        MicroWait(POST_WRITE_DELAY_US);

        // Verify everything that was just written
        //IMUWriteReadPipelined(IMU_READ | ACC_PMU_RANGE);
        //data = IMUWriteReadPipelined();
        assert(data == RANGE_2G);
        
        //IMUWriteReadPipelined(IMU_READ | ACC_PMU_BW);
        //data = IMUWriteReadPipelined();
        assert(data == BW_250);
      }
      
      
      // Initialize gyro
      static void InitGyro()
      {
        uint8_t data;
        // Do a communication test
        //IMUWriteReadPipelined(IMU_READ | GYRO_CHIP_ID);
        //IMUWriteReadPipelined(0x00);
        //data = IMUWriteReadPipelined();
        assert(data == GYRO_CHIPID);
        
        // Configure gyro
        // Set +/- 500 deg/sec range 
        //IMUWriteReadPipelined(IMU_WRITE | GYRO_RANGE);
        //IMUWriteReadPipelined(RANGE_500DPS);
        MicroWait(2);  // 2 us delay required after write
        
        // TODO: Add lowpass filter on gyro? Pete isn't certain this is actually useful. Good. I'm a big dope, says pete.
        
        // Verify everything that was just written
        //IMUWriteReadPipelined(IMU_READ | GYRO_RANGE);
        //data = IMUWriteReadPipelined();
        assert(data == RANGE_500DPS);
      }
      

      // Initialize I2C, set up accelerometer and gyro
      void IMUInit()
      {  
        
        return;
        
        InitI2C();
        InitAcc();
        InitGyro();
      }

      // Read data from IMU. Takes IMU_DataStructure
      // Assuming ACC +/- 2g range
      // Assuming GYRO +/- 500 deg range
      void IMUReadData(IMU_DataStructure &IMUData)
      {
        static uint8_t temp_data_msb, temp_data_lsb;
        static s16 temp_data;

        // Accelerometer x values
        // Get data
        //IMUWriteReadPipelined(IMU_READ | ACC_ACCD_X_LSB);  // ACC_ACCD_X_LSB
        //temp_data_lsb = IMUWriteReadPipelined(0x00); // ACC_ACCD_Y_LSB
        //temp_data_msb = IMUWriteReadPipelined(0x00);  //ACC_ACCD_Y_MSB

        // Combine and convert to signed
        temp_data = (temp_data_msb << 8) | (temp_data_lsb & ACC_LSB_MASK);
        temp_data = temp_data >> 4;  // signed extension shift to 12 bits
				
        // Put values into IMU Data Struct
#ifdef COZMO2          
        // With head facing forward, x-axis points along robot y-axis. Putting x value into y.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of acc x is preserved.      
        IMUData.acc_y  = RANGE_CONST_2G * temp_data;  // m/s^2    
#else
        IMUData.acc_y  = RANGE_CONST_2G * temp_data;  // m/s^2    
#endif
        
        //temp_data_lsb = IMUWriteReadPipelined(0x00);  // ACC_ACCD_Z_LSB
        //temp_data_msb = IMUWriteReadPipelined(0x00);  // ACC_ACCD_Z_MSB
        
        // Combine and convert to signed
        temp_data = (temp_data_msb << 8) | (temp_data_lsb & ACC_LSB_MASK);
        temp_data = temp_data >> 4; // Signed extension shift to 12 bits
				
        // Put values into IMU Data Struct
#ifdef COZMO2          
        // With head facing forward, y-axis points along robot z-axis. Putting y value into z.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of acc y is flipped.      
        IMUData.acc_z  = -RANGE_CONST_2G * temp_data;      // m/s^2    
#else
        IMUData.acc_z  = RANGE_CONST_2G * temp_data;      // m/s^2   
#endif

        //temp_data_lsb = IMUWriteReadPipelined(0x00);    
        //temp_data_msb = IMUWriteReadPipelined();
        
        // Combine and convert to signed
        temp_data = (temp_data_msb << 8) | (temp_data_lsb & ACC_LSB_MASK);
        temp_data = temp_data >> 4;  // Signed extension shift to 12 bits
				
        // Put values into IMU Data Struct
#ifdef COZMO2                
        // With head facing forward, z-axis points along robot x-axis. Putting z value into x.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of acc z is flipped.
        IMUData.acc_x  = -RANGE_CONST_2G * temp_data;  // m/s^2    
#else
        IMUData.acc_x  = RANGE_CONST_2G * temp_data;  // m/s^2    
#endif
         
        // Gyro x values
        // Get data
        //IMUWriteReadPipelined(IMU_READ | GYRO_RATE_X_LSB);  // GYRO_RATE_X_LSB
        //IMUWriteReadPipelined(0x00);  // GYRO_RATE_X_MSB
        //temp_data_lsb = IMUWriteReadPipelined(0x00);  // GYRO_RATE_Y_LSB
        //temp_data_msb = IMUWriteReadPipelined(0x00);  // GYRO_RATE_Y_MSB
        
        // Combine
        temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				
        // Put values into IMU Data Struct
#ifdef COZMO2                
        // With head facing forward, x-axis points along robot y-axis. Putting x value into y.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of gyro x is preserved.
        IMUData.rate_y  = RANGE_CONST_500D * temp_data;  // rad/s    
#else
        IMUData.rate_y  = RANGE_CONST_500D * temp_data;  // rad/s    
#endif
        
        //temp_data_lsb = IMUWriteReadPipelined(0x00);  // GYRO_RATE_Z_LSB
        //temp_data_msb = IMUWriteReadPipelined(0x00);  // GYRO_RATE_Z_MSB

        // Combine
        temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				
        // Put values into IMU Data Struct
#ifdef COZMO2                
        // With head facing forward, y-axis points along robot z-axis. Putting y value into z.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of gyro y is flipped.
        IMUData.rate_z  = -RANGE_CONST_500D * temp_data;  // rad/s    
#else
        IMUData.rate_z  = RANGE_CONST_500D * temp_data;  // rad/s    
#endif
  
        //temp_data_lsb = IMUWriteReadPipelined(0x00);    
        //temp_data_msb = IMUWriteReadPipelined();

        // Combine
        temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				
        // Put values into IMU Data Struct
#ifdef COZMO2                
        // With head facing forward, z-axis points along robot x-axis. Putting z value into x.
        // IMU rotated 180 around Z axis and then 180 around Y axis in 2.1, so sign of gyro z is flipped.
        IMUData.rate_x  = -RANGE_CONST_500D * temp_data;  // rad/s    
#else
        IMUData.rate_x  = RANGE_CONST_500D * temp_data;  // rad/s    
#endif
      }
    }         
  }
}
