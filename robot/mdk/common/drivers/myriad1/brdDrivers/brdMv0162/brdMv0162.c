///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for use on MV0153 to configure MV0155 Daughtercard
/// 
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include "brdMv0162.h"



// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
TMipiBridge mv162MipiBridges[] =
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

        {
            .i2cDev = NULL,
            .BridgeType = CSI2TXParIn,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_AT_CS,
            .ResetGPIO = GPIO_MIPI_AT_RESX,
            .ModeSelectGPIO = GPIO_MIPI_AT_MSEL,
        },
        {
            .i2cDev = NULL,
            .BridgeType = CSI2TXParIn,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_BT_CS,
            .ResetGPIO = GPIO_MIPI_BT_RESX,
            .ModeSelectGPIO = GPIO_MIPI_BT_MSEL,
        },
        {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = GPIO_MIPI_CT_CS,
            .ResetGPIO = GPIO_MIPI_CT_RESX,
            .ModeSelectGPIO = GPIO_MIPI_CT_MSEL,
        },


    };

int mv162MipiBridgesCount = sizeof(mv162MipiBridges)/sizeof(mv162MipiBridges[0]);

static u32 commonI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2);

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr)
{
    slaveAddr=slaveAddr;
    regAddr=regAddr;

    printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",i2cCommsError,slaveAddr,regAddr);
    return i2cCommsError; // Because we haven't really handled the error, pass it back to the caller
}

static I2CM_Device i2cDevHandle[NUM_I2C_DEVICES];    

static tyI2cConfig i2c0Mv162DefaultConfiguration = 
    {
     .device                = IIC1_DEVICE,
     .sclPin                = I2C1_SCL,
     .sdaPin                = I2C1_SDA,
     .speedKhz              = I2C1_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C1_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };

static tyI2cConfig i2c1Mv162DefaultConfiguration = 
    {
     .device                = IIC2_DEVICE,
     .sclPin                = I2C2_SCL,
     .sdaPin                = I2C2_SDA,
     .speedKhz              = I2C2_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C2_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------



int brd162InitialiseI2C(tyI2cConfig * i2cCfg0, tyI2cConfig * i2cCfg1,I2CM_Device ** i2c1Dev,I2CM_Device ** i2c2Dev)
{
    int ret;

    // Unless the user specifies otherwise we use the default configuration
    if (i2cCfg0 == NULL)
        i2cCfg0 = &i2c0Mv162DefaultConfiguration;

    if (i2cCfg1 == NULL)
        i2cCfg1 = &i2c1Mv162DefaultConfiguration;


    // Initialise the I2C device to use the I2C0 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[0], i2cCfg0);
    if (ret != I2CM_STAT_OK)
    {
        return -1;
    }
    *i2c1Dev = &i2cDevHandle[0]; // Return the handle

    // Initialise the I2C device to use the I2C1 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[1], i2cCfg1);
    if (ret != I2CM_STAT_OK)
    {
        return -2;
    }
    *i2c2Dev = &i2cDevHandle[1]; // Return the handle

    return 0;
}    


int brd162InitialiseMipiBridges(I2CM_Device * i2cDev1, I2CM_Device * i2cDev2)
{
        unsigned int i;
        for (i=0;i<3;i++) {
            icTC358746AssignI2c(&mv162MipiBridges[i], i2cDev2);
            icTC358746Reset(&mv162MipiBridges[i]);
        }
        for (i=3;i<(sizeof(mv162MipiBridges)/sizeof(TMipiBridge));i++) {
            icTC358746AssignI2c(&mv162MipiBridges[i], i2cDev1);
            icTC358746Reset(&mv162MipiBridges[i]);
        }
        return 0; 
}
