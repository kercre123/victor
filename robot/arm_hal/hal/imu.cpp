/**
 * File: imu.cpp
 *
 * Author: Bryan Hood
 * Created: 10/10/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description:
 *
 *		Driver file for BMI055 IMU
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"

// SPI READ/WRITE bits
#define IMU_READ						0x80
#define IMU_WRITE						0x00

// IMU Chip ID
#define CHIPID							0xFA

// Accelerometer Register Map
#define	ACC_BGW_CHIPID			0x00
#define ACC_ACCD_X_LSB			0x02
#define ACC_ACCD_X_MSB			0x03
#define	ACC_ACCD_Y_LSB			0x04
#define	ACC_ACCD_Y_MSB			0x05
#define ACC_ACCD_Z_LSB			0x06
#define ACC_ACCD_Z_MSB			0x07
#define ACC_ACCD_TEMP				0x08
#define	ACC_INT_STATUS_0		0x09
#define ACC_INT_STATUS_1		0x0A
#define	ACC_INT_STATUS_2		0x0B
#define ACC_INT_STATUS_3		0x0C
#define ACC_FIFO_STATUS			0x0E
#define	ACC_PMU_RANGE				0x0F
#define	ACC_PMU_BW					0x10
#define ACC_PMU_LPW					0x11
#define ACC_PMU_LOW_POWER		0x12
#define	ACC_ACCD_HBW				0x13
#define	ACC_BGW_SOFTRESET		0x14
#define	ACC_INT_EN_0				0x16
#define	ACC_INT_EN_1				0x17
#define	ACC_INT_EN_2				0x18
#define	ACC_INT_MAP_0				0x19
#define	ACC_INT_MAP_1				0x1A
#define	ACC_INT_MAP_2				0x1B
#define	ACC_INT_SRC					0x1E
#define	ACC_INT_OUT_CTRL		0x20
#define	ACC_INT_RST_LATCH		0x21
#define	ACC_INT_0						0x22
#define	ACC_INT_1						0x23
#define	ACC_INT_2						0x24
#define	ACC_INT_3						0x25
#define	ACC_INT_4						0x26
#define	ACC_INT_5						0x27
#define	ACC_INT_6						0x28
#define	ACC_INT_7						0x29
#define	ACC_INT_8						0x2A
#define	ACC_INT_9						0x2B
#define	ACC_INT_A						0x2C
#define	ACC_INT_B						0x2D
#define	ACC_INT_C						0x2E
#define	ACC_INT_D						0x2F
#define	ACC_FIFO_CONFIG_0		0x30
#define ACC_PMU_SELF_TEST		0x32
#define ACC_TRIM_NVM_CTRL		0x33
#define	ACC_BGW_SPI3_WDT		0x34
#define	ACC_OFC_CTRL				0x36
#define ACC_OFC_SETTING			0x37
#define	ACC_OFC_OFFSET_X		0x38
#define	ACC_OFC_OFFSET_Y		0x39
#define	ACC_OFC_OFFSET_Z		0x3A
#define ACC_TRIM_GP0				0x3B
#define	ACC_TRIM_GP1				0x3C
#define	ACC_FIFO_CONFIG_1		0x3E
#define	ACC_FIFO_DATA				0x3F

//
#define RANGE_2G						0x03
#define	RANGE_4G						0x05
#define	RANGE_8G						0x08
#define RANGE_16G						0x0B

#define BW_250							0x0C

// Accelerometer Register Map
#define	GYRO_CHIP_ID				0x00
#define	GYRO_RATE_X_LSB			0x02
#define	GYRO_RATE_X_MSB			0x03
#define	GYRO_RATE_Y_LSB			0x04
#define	GYRO_RATE_Y_MSB			0x05
#define	GYRO_RATE_Z_LSB			0x06
#define	GYRO_RATE_Z_MSB			0x07
#define	GYRO_INT_STATUS_0		0x09
#define GYRO_INT_STATUS_1		0x0A
#define	GYRO_INT_STATUS_2		0x0B
#define GYRO_INT_STATUS_3		0x0C
#define GYRO_FIFO_STATUS		0x0E
#define	GYRO_RANGE					0x0F
#define	GYRO_BW							0x10
#define	GYRO_LPM1						0x11
#define	GYRO_LPM2						0x12
#define	GYRO_RATE_HBW				0x13
#define	GYRO_BGW_SOFTRESET	0x14
#define	GYRO_INT_EN_0				0x15
#define	GYRO_INT_EN_1				0x16
#define	GYRO_INT_MAP_0			0x17
#define	GYRO_INT_MAP_1			0x18
#define	GYRO_INT_MAP_2			0x19
#define	GYRO_REG_1A					0x1A
#define	GYRO_REG_1B					0x1B
#define	GYRO_REG_1C					0x1C
#define	GYRO_REG_1E					0x1E
#define	GYRO_INT_RST_LATCH	0x21
#define	GYRO_HIGH_TH_X			0x22
#define	GYRO_HIGH_DUR_X			0x23
#define	GYRO_HIGH_TH_Y			0x24
#define	GYRO_HIGH_DUR_Y			0x25
#define	GYRO_HIGH_TH_Z			0x26
#define	GYRO_HIGH_DUR_Z			0x27
#define GYRO_SOC						0x31
#define	GYRO_A_FOC					0x32
#define	GYRO_TRIM_NVM_CTRL	0x33
#define	GYRO_BGW_SPI3_WDT		0x34
#define	GYRO_OFC1						0x36
#define	GYRO_OFC2						0x37
#define	GYRO_OFC3						0x38
#define	GYRO_OFC4						0x39
#define	GYRO_TRIM_GP0				0x3A
#define	GYRO_TRIM_GP1				0x3B
#define	GYRO_BIST						0x3C
#define	GYRO_FIFO_CONFIG_0	0x3D
#define	GYRO_FIFO_CONFIG_1	0x3E
#define	GYRO_FIFO_DATA			0x3F

// Constants
// 12 bits, +/- 2g   4g/2^12 = 4*9.81 m/s^2 / 4096 ~= 9.58E-3 m/s^2 / LSB
#define RANGE_CONST_2G			0.00958
// 16 bits, +/- 500 deg 1000/2^16 ~= 0.01526 deg / LSB
#define RANGE_CONST_500D		0.01526


namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
			
			// IMU Data Structure
			IMU_DataStructure IMU_Data;
			
			// Define SPI pins with macros
			GPIO_PIN_SOURCE(IMU_SCK, GPIOI, 1);
      GPIO_PIN_SOURCE(IMU_MISO, GPIOI, 3);
      GPIO_PIN_SOURCE(IMU_MOSI, GPIOI, 2);
			GPIO_PIN_SOURCE(IMU_CS_ACC, GPIOG, 7);
			GPIO_PIN_SOURCE(IMU_CS_GYRO, GPIOG, 2);
			GPIO_PIN_SOURCE(IMU_INT, GPIOG, 3);
			
			// Initialize SPI2, set up accelerometer and gyro
			void IMUInit()
			{	

				// Enable peripheral clock
				RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
				
				// Enable SCK, MOSI, MISO and NSS GPIO clocks
				RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
				
				//  Peripherals alternate function
				GPIO_PinAFConfig(GPIO_IMU_SCK, SOURCE_IMU_SCK, GPIO_AF_SPI2);
				GPIO_PinAFConfig(GPIO_IMU_MISO, SOURCE_IMU_MISO, GPIO_AF_SPI2);
				GPIO_PinAFConfig(GPIO_IMU_MOSI, SOURCE_IMU_MOSI, GPIO_AF_SPI2);

				// Initalize Pins
        GPIO_InitTypeDef GPIO_InitStructure;
				// Set SPI alternate function pins
        GPIO_InitStructure.GPIO_Pin = PIN_IMU_SCK | PIN_IMU_MISO | PIN_IMU_MOSI;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_IMU_SCK, &GPIO_InitStructure);  // GPIOI
				// Set CS output pins
        GPIO_InitStructure.GPIO_Pin = PIN_IMU_CS_GYRO | PIN_IMU_CS_ACC; 
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_IMU_CS_GYRO, &GPIO_InitStructure);  // GPIOG
			  // Set Interupt input pin
				GPIO_InitStructure.GPIO_Pin = PIN_IMU_INT; 
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                // XXX does this matter when set as an input?
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
        GPIO_Init(GPIO_IMU_INT, &GPIO_InitStructure);  // GPIOG			
							
				// Program the Polarity, Phase, First Data, Baud Rate Prescaler, Slave Management, Peripheral Mode and CRC Polynomial values using the SPI_Init() function.
				SPI_InitTypeDef SPI_InitStructure;
				
				SPI_InitStructure.SPI_Direction 						=			SPI_Direction_2Lines_FullDuplex;
				SPI_InitStructure.SPI_Mode									=			SPI_Mode_Master;
				SPI_InitStructure.SPI_DataSize							=			SPI_DataSize_16b;
				SPI_InitStructure.SPI_CPOL									=			SPI_CPOL_High;
				SPI_InitStructure.SPI_CPHA									=			SPI_CPHA_2Edge;
				SPI_InitStructure.SPI_NSS										=			SPI_NSS_Soft;
				SPI_InitStructure.SPI_BaudRatePrescaler			=			SPI_BaudRatePrescaler_128;
				SPI_InitStructure.SPI_FirstBit							=			SPI_FirstBit_MSB;
				/*SPI_InitStruct.SPI_CRCPolynomial					=			?*/
				
				SPI_Init(SPI2, &SPI_InitStructure);
				
				// Enable the SPI
				SPI_Cmd(SPI2, ENABLE);
				
				GPIO_SET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);	
				GPIO_SET(GPIO_IMU_CS_GYRO, PIN_IMU_CS_GYRO);	
				
				MicroWait(1000);
				
				// Set up accelerometer
				// select accelerometer
				GPIO_RESET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);			
				// Set 2G range
				SPI_I2S_SendData(SPI2, (IMU_WRITE | ACC_PMU_RANGE) << 8 | RANGE_2G);
				while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
				{
				}
				// Set bandwidth to 250 Hz
				SPI_I2S_SendData(SPI2, (IMU_WRITE | ACC_PMU_BW) << 8 | BW_250);
				// wait for TX buffer to empty
				while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY))
				{
				}

				//MicroWait(50);
				// deselect accelerometer			
				GPIO_SET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);
				
				
				// Set up gyro
				
			}
			//
			bool IMUCommTest()
			{
					
				volatile uint16_t chipid;
				
				// select accelerometer
				GPIO_RESET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);
					
				// check chip id
				// get garbage out of receiver buffer
				while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE))
				{
					chipid = SPI_I2S_ReceiveData(SPI2);
				}		
				
				//SPI_I2S_SendData(SPI2, (IMU_READ | ACC_BGW_CHIPID) << 8);
				SPI_I2S_SendData(SPI2, 0x8000);
				// wait for TX buffer to empty
				while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY))
				{
				}
				chipid = SPI_I2S_ReceiveData(SPI2);
				// wait for RX buffer to fill

				// deselect accelerometer
				GPIO_SET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);
				
				// return false if chip ID is not right
				if(chipid != CHIPID)
				{
					return false;
				}
/*
				
				// select gyro
				GPIO_RESET(GPIO_IMU_CS_GYRO, PIN_IMU_CS_GYRO);
					
				// check chip id
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_CHIP_ID) << 8);
				chipid = SPI_I2S_ReceiveData(SPI2);
								
				// deselect gyro
				GPIO_SET(GPIO_IMU_CS_GYRO, PIN_IMU_CS_GYRO);
				
				// return false if chip ID is not right
				if(chipid != CHIPID)
				{
					return false;
				}
*/
				return true;
			}
			
			// Assuming ACC +/- 2g range
			
			void IMUReadData(IMU_DataStructure &IMUData)
			{
				static uint16_t temp_data_msb;
				static uint16_t temp_data_lsb;
				static s16 temp_data;
				
				// select accelerometer
				GPIO_RESET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);
				
				// accelerometer x values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_X_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_X_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb & 0xF0);  // should mask these properly
				temp_data = temp_data >> 4; // signed extension shift to 12 bits
				// put values into IMU Data Struct
				IMUData.acc_x	= RANGE_CONST_2G * temp_data;			// m/s^2		
				
				// accelerometer y values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_Y_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_Y_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb & 0xF0);  // should mask these properly
				temp_data = temp_data >> 4; // signed extension shift to 12 bits
				// put values into IMU Data Struct
				IMUData.acc_y	= RANGE_CONST_2G * temp_data;			// m/s^2		
				
				// accelerometer z values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_Z_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | ACC_ACCD_Z_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb & 0xF0);  // should mask these properly
				temp_data = temp_data >> 4; // signed extension shift to 12 bits
				// put values into IMU Data Struct
				IMUData.acc_z	= RANGE_CONST_2G * temp_data;			// m/s^2		
				
				// deselect accelerometer
				GPIO_SET(GPIO_IMU_CS_ACC, PIN_IMU_CS_ACC);
				
				// select gyro
				GPIO_RESET(GPIO_IMU_CS_GYRO, PIN_IMU_CS_GYRO);
				
				// gyro x values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_X_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_X_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				// put values into IMU Data Struct
				IMUData.rate_x	= RANGE_CONST_500D * temp_data;			// rad/s		
				
				// gyro y values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_Y_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_Y_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				// put values into IMU Data Struct
				IMUData.rate_y	= RANGE_CONST_500D * temp_data;			// rad/s		
	
				// gyro z values
				// get data
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_Z_MSB) << 8);
				temp_data_msb = SPI_I2S_ReceiveData(SPI2);
				SPI_I2S_SendData(SPI2, (IMU_READ | GYRO_RATE_Z_LSB) << 8);
				temp_data_lsb = SPI_I2S_ReceiveData(SPI2);
				// convert to signed
				temp_data = (temp_data_msb << 8) | (temp_data_lsb ); 
				// put values into IMU Data Struct
				IMUData.rate_z	= RANGE_CONST_500D * temp_data;			// rad/s		
									
				// deselect gyro
				GPIO_SET(GPIO_IMU_CS_GYRO, PIN_IMU_CS_GYRO);
					
			}
			
		}
  }
}
