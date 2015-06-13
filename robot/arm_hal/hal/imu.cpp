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
#include "hal/i2c.h"
#include <string.h>

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

// Accelerometer Constants
// 12 bits, +/- 2g   4g/2^12 = 4*9.81 m/s^2 / 4096 ~= 0.958 mm/s^2 / LSB
#define ACC_RANGE_CONST    9.58f

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
#define GYRO_RANGE_CONST    2.663E-4f

#undef assert
#define assert(x) while(!(x)) {}

// Addresses
#define ADDR_ACC 0x18  // 7-bit slave address of accelerometer
#define ADDR_GYRO 0x68 // 7-bit slave address of gyro

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Set an I2C register and immediately verify that it was written correctly
      static void SetAndVerify(u8 addr, u8 reg, u8 data)
      {
        I2CWrite(addr, reg, data);
        u8 result = I2CRead(addr, reg);
        assert(result == data);
      }

      // Initialize I2C, set up accelerometer and gyro
      void IMUInit()
      { 
        I2CInit();
        MicroWait(35000); // Datasheet says startup time is 30ms

        // Initialize accelerometer
        SetAndVerify(ADDR_ACC, ACC_PMU_RANGE, RANGE_2G);
        SetAndVerify(ADDR_ACC, ACC_PMU_BW, BW_250);
        SetAndVerify(ADDR_ACC, ACC_INT_OUT_CTRL, ACC_INT_OPEN_DRAIN);        
        
        // Set +/- 500 deg/sec range 
        SetAndVerify(ADDR_GYRO, GYRO_RANGE, RANGE_500DPS);
        
        // TODO: Add lowpass filter on gyro?
        
        // Start the pipeline by kicking off the first reading
        IMU_DataStructure imu;
        IMUReadData(imu);
      }
      
      // List of I2C operations and registers needed to read the IMU
      // This drives the state machine in the i2c.cpp interrupt handler
      typedef __packed struct
      {
        s16 astart[6];
        s16 axl, axm;
        s16 ayl, aym;
        s16 azl, azm;
        s16 astop;
        
        s16 gstart[6];
        s16 gxl, gxm;
        s16 gyl, gym;
        s16 gzl, gzm;
        s16 gstop;
        
        s16 end;
      } IMUOps;
      const IMUOps IMU_READ_OPS = 
      { 
        // Read accelerometer regs
        { StartBit, 
          ADDR_ACC << 1,        // Write starting register index
          ACC_ACCD_X_LSB,       // First of 6 regs
          StopBit,
          StartBit,
          (ADDR_ACC << 1) + 1   // Read all 6 registers
        },
        
        ReadByte,
        ReadByte, 
        ReadByte, 
        ReadByte, 
        ReadByte,          
        ReadByteEnd,          
        StopBit,
        
        // Read gyro regs
        { StartBit, 
          ADDR_GYRO << 1,        // Write starting register index
          GYRO_RATE_X_LSB,       // First of 6 regs
          StopBit,
          StartBit,
          (ADDR_GYRO << 1) + 1   // Read all 6 registers
        },
        
        ReadByte,
        ReadByte, 
        ReadByte, 
        ReadByte, 
        ReadByte,          
        ReadByteEnd,          
        StopBit,
        
        EndOfList
      };
      
      // Accelerometer uses 12 most significant bits - gyro uses all 16
      #define ACC_CONVERT(lsb, msb)  (ACC_RANGE_CONST * (((s16)((lsb) + ((msb) << 8))) >> 4))
      #define GYRO_CONVERT(lsb, msb) (GYRO_RANGE_CONST * ((s16)((lsb) + ((msb) << 8))))
      
      // Read data from IMU into IMUData
      // This is pipelined - the data you receive will be from the last call (approx 5ms ago)
      void IMUReadData(IMU_DataStructure &imuData)
      {
        // Wait for previous IMU sample to complete, then convert and copy the data
        static IMUOps ops;
        I2CWait();
        
        // The IMU chip is not oriented along the robot's natural axes
        // Positive X is robot forward, positive Y is robot left, and positive Z is robot up
        // Thus:  Robot X = -IMU Z, Robot Y = IMU Y, Robot Z = -IMU X
        imuData.acc_x = - ACC_CONVERT(ops.azl, ops.azm); 
        imuData.rate_x= -GYRO_CONVERT(ops.gzl, ops.gzm);   

        imuData.acc_y =   ACC_CONVERT(ops.ayl, ops.aym);  
        imuData.rate_y=  GYRO_CONVERT(ops.gyl, ops.gym);

        imuData.acc_z = - ACC_CONVERT(ops.axl, ops.axm);     
        imuData.rate_z= -GYRO_CONVERT(ops.gxl, ops.gxm);  
        
        // Set up the next sample and kick it off
        memcpy(&ops, &IMU_READ_OPS, sizeof(ops));
        I2CRun((s16*)&ops);
      }
    }         
  }
}
