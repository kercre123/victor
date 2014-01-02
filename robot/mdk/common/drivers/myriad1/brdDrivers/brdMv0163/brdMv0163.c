///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0163 Daughtercard
/// 
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include "brdMv0163.h"
#include "brdMv0163Defines.h"
#include "DrvGpio.h"



// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
TMipiBridge mv163MipiBridges[] =
    {
        {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_A_CS,
            .ResetGPIO = GPIO_MIPI_A_RESX,
            .ModeSelectGPIO = GPIO_MIPI_A_MSEL,
        },
        {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_B_CS,
            .ResetGPIO = GPIO_MIPI_B_RESX,
            .ModeSelectGPIO = GPIO_MIPI_B_MSEL,
        },
        {
            .i2cDev = NULL,
            .BridgeType = CSI2TXParIn,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_C_CS,
            .ResetGPIO = GPIO_MIPI_C_RESX,
            .ModeSelectGPIO = GPIO_MIPI_C_MSEL,
        },
    };

int mv163MipiBridgesCount = sizeof(mv163MipiBridges)/sizeof(mv163MipiBridges[0]);

static u32 commonI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2);

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr)
{
    slaveAddr=slaveAddr;
    regAddr=regAddr;

    printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",i2cCommsError,slaveAddr,regAddr);
    return i2cCommsError; // Because we haven't really handled the error, pass it back to the caller
}

static I2CM_Device i2cDevHandle[NUM_I2C_DEVICES];    

// I2C1
static tyI2cConfig i2c1Mv163DefaultConfiguration = 
    {
     .device                = GPIO_BITBASH,
     .sclPin                = I2C1_SCL,
     .sdaPin                = I2C1_SDA,
     .speedKhz              = I2C1_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C1_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };

// I2C2
static tyI2cConfig i2c2Mv163DefaultConfiguration = 
    {
     .device                = IIC2_DEVICE,
     .sclPin                = I2C2_SCL,
     .sdaPin                = I2C2_SDA,
     .speedKhz              = I2C2_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C2_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };


static u8 protoCamWrite[] =
    {
        S_ADDR_WR,
        R_ADDR_H,
        R_ADDR_L,
        DATAW,
        LOOP_MINUS_1
    };

static u8 protoCamRead[] =
    {
        S_ADDR_WR,
        R_ADDR_H,
        R_ADDR_L,
        S_ADDR_RD,
        DATAR,
        LOOP_MINUS_1
    };

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------



int brd163InitialiseI2C(tyI2cConfig * i2cCfg, I2CM_Device ** i2cDev)
{
    int ret;

    // Unless the user specifies otherwise we use the default configuration
    if (i2cCfg == NULL)
        i2cCfg = &i2c2Mv163DefaultConfiguration;

	// Initialise the I2C device to use the I2C0 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[0], i2cCfg);
    if (ret != I2CM_STAT_OK)
    {
        return -1;
    }
    *i2cDev = &i2cDevHandle[0]; // Return the handle

    return 0;
}    


int brd163InitialiseMipiBridges(I2CM_Device * i2cDev)
{
        unsigned int i;
        for (i=0;i<(sizeof(mv163MipiBridges)/sizeof(TMipiBridge));i++) {
            icTC358746AssignI2c(&mv163MipiBridges[i], i2cDev);
            icTC358746Reset(&mv163MipiBridges[i]);
        }
        return 0; 
}

int brd163ReasignMipiBridgesI2cModule(I2CM_Device * i2cDev)
{
        unsigned int i;
        for (i=0;i<(sizeof(mv163MipiBridges)/sizeof(TMipiBridge));i++) {
            icTC358746AssignI2c(&mv163MipiBridges[i], i2cDev);
        }
        return 0; 
}

void brd163GpioToI2cDevice(TBrd163I2cDevicesEnum tsh, I2CM_Device ** i2cDev)
{
    switch (tsh)
    {
    case ToshibaChip1:
         // get another i2c module handler with config for this chip, 
         // TO DO : we should not do this all the time
         brd163InitialiseI2C(&i2c2Mv163DefaultConfiguration, i2cDev);
         
         // assign the handler to the i2c modules
         brd163ReasignMipiBridgesI2cModule(*i2cDev);

         break;
          
    case ToshibaChip2:
		 DrvGpioSetPin(112,0); // Keep MSEL B low for CSI -> Par_out
         brd163InitialiseI2C(&i2c2Mv163DefaultConfiguration, i2cDev);
         brd163ReasignMipiBridgesI2cModule(*i2cDev);
         
         break;
         
    case ToshibaChip3:
         brd163InitialiseI2C(&i2c2Mv163DefaultConfiguration, i2cDev);
         brd163ReasignMipiBridgesI2cModule(*i2cDev);
         
         break;
         
     case ToshibaCam1:
		  DrvGpioSetPin(112,1);	// Bring I2C_LS_EN high to enable level shifter
          //get new handler and use it later 
          brd163InitialiseI2C(&i2c1Mv163DefaultConfiguration, i2cDev);
          break;
         
     case ToshibaCam2:
          //get new handler and use it later 
          brd163InitialiseI2C(&i2c2Mv163DefaultConfiguration, i2cDev);
          break;
         
    default:
          brd163InitialiseI2C(&i2c2Mv163DefaultConfiguration, i2cDev);
          brd163ReasignMipiBridgesI2cModule(*i2cDev);
          
    }
}

I2CM_StatusType brd163ToshibaCamRegSet(I2CM_Device * i2cDev, u32 data[][2], u32 len)
{
    u32 i;
    I2CM_StatusType retVal;
    u8 dataToBeWritten;
    
    for (i=0; i<len ; ++i)
     {
          dataToBeWritten = data[i][1] & 0xFF;
          // write each register 
          retVal = DrvI2cMTransaction(i2cDev, 
                                        TSH_CAM_I2C_ADR, 
                                        data[i][0], 
                                        protoCamWrite, 
                                        &dataToBeWritten, 
                                        1);

          // abort if there is an error           
          if (I2CM_STAT_OK != retVal) 
          {   
              
              return retVal;
          }
     }
     return 0;
}

I2CM_StatusType brd163ToshibaCamRegGet(I2CM_Device * i2cDev, u32 data[][2], u32 len)
{
    u32 i;
    u8 dataToBeRead;
    I2CM_StatusType retVal;
    
    for (i=0; i<len ; ++i)
     {
          // write each register 
          retVal = DrvI2cMTransaction(i2cDev, 
                                        TSH_CAM_I2C_ADR, 
                                        data[i][0], 
                                        protoCamRead, 
                                        &dataToBeRead, 
                                        1);

          // abort if there is an error           
          if (I2CM_STAT_OK != retVal) 
          {   
              return retVal;
          }
          else
          { 
              data[i][1] = dataToBeRead;
          }
     }
     return 0;
     
}
