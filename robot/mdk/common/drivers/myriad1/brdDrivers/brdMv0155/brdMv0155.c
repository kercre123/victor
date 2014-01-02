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
#include "brdMv0155.h"
#include "brdMv0155Defines.h"
#include "DrvGpio.h"



// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
TMipiBridge mv155MipiBridges[] =
    {
        {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = CAM1_CS,
            .ResetGPIO = CAM1_RESX,
            .ModeSelectGPIO = CAM1_MSEL,
        },
        {
            .i2cDev = NULL,
            .BridgeType = CSI2RXParOut,
            .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

            .ChipSelectGPIO = CAM2_CS,
            .ResetGPIO = CAM2_RESX,
            .ModeSelectGPIO = CAM2_MSEL,
        },
        {
             .i2cDev = NULL,
             .BridgeType = CSI2TXParIn,
             .BridgeI2CSlaveAddr = TC358746XBG_SLAVE_ADDRESS,

             .ChipSelectGPIO = LCD1_CS,
             .ResetGPIO = LCD1_RESX,
             .ModeSelectGPIO = LCD1_MSEL,
        },
    };

int mv155MipiBridgesCount = sizeof(mv155MipiBridges)/sizeof(mv155MipiBridges[0]);


static u32 commonI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2);

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr)
{
    slaveAddr=slaveAddr;
    regAddr=regAddr;

    printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",i2cCommsError,slaveAddr,regAddr);
    return i2cCommsError; // Because we haven't really handled the error, pass it back to the caller
}

static I2CM_Device i2cDevHandle[NUM_I2C_DEVICES];    



static tyI2cConfig i2c1Mv155DefaultConfiguration1 = 
    {
     .device                = IIC2_DEVICE,
     .sclPin                = I2C2_SCL,
     .sdaPin                = I2C2_SDA1,
     .speedKhz              = I2C2_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C2_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };

static tyI2cConfig i2c1Mv155DefaultConfiguration2 = 
    {
     .device                = IIC2_DEVICE,
     .sclPin                = I2C2_SCL,
     .sdaPin                = I2C2_SDA2,
     .speedKhz              = I2C2_SPEED_KHZ_DEFAULT,
     .addressSize           = I2C2_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };
    
static tyI2cConfig i2c1Mv155DefaultConfiguration3 = 
    {
     .device                = IIC2_DEVICE,
     .sclPin                = I2C2_SCL,
     .sdaPin                = I2C2_SDA3,
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



int brd155InitialiseI2C(tyI2cConfig * i2cCfg, I2CM_Device ** i2cDev)
{
    int ret;

    // Unless the user specifies otherwise we use the default configuration
    if (i2cCfg == NULL)
        i2cCfg = &i2c1Mv155DefaultConfiguration1;



    // Initialise the I2C device to use the I2C1 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[1], i2cCfg);
    if (ret != I2CM_STAT_OK)
    {
        return -1;
    }
    *i2cDev = &i2cDevHandle[1]; // Return the handle

    return 0;
}    


int brd155InitialiseMipiBridges(I2CM_Device * i2cDev)
{
        unsigned int i;
        for (i=0;i<(sizeof(mv155MipiBridges)/sizeof(TMipiBridge));i++) {
            icTC358746AssignI2c(&mv155MipiBridges[i], i2cDev);
            icTC358746Reset(&mv155MipiBridges[i]);
        }
        return 0;
}

static int brd155ReasignMipiBridgesI2cModule(I2CM_Device * i2cDev)
{
        unsigned int i;
        for (i=0;i<sizeof(mv155MipiBridges)/sizeof(TMipiBridge);i++) {
            icTC358746AssignI2c(&mv155MipiBridges[i], i2cDev);
        }
        return 0; 
}

void brd155GpioToCam1()
{
     DrvGpioMode(I2C2_SCL , D_GPIO_DIR_IN | D_GPIO_MODE_0); // set clock for all since order of call is not known
     DrvGpioMode(I2C2_SDA2, D_GPIO_DIR_IN | D_GPIO_MODE_4);

     DrvGpioMode(I2C2_SDA1, D_GPIO_DIR_IN | D_GPIO_MODE_7); // not talking to the oters
     DrvGpioMode(I2C2_SDA3, D_GPIO_DIR_IN | D_GPIO_MODE_7);
}

void brd155GpioToCam2()
{
     DrvGpioMode(I2C2_SCL , D_GPIO_DIR_IN | D_GPIO_MODE_0);
     DrvGpioMode(I2C2_SDA3, D_GPIO_DIR_IN | D_GPIO_MODE_1);

     DrvGpioMode(I2C2_SDA1, D_GPIO_DIR_IN | D_GPIO_MODE_7);
     DrvGpioMode(I2C2_SDA2, D_GPIO_DIR_IN | D_GPIO_MODE_7);
}

void brd155GpioToBridges()
{
     DrvGpioMode(I2C2_SCL , D_GPIO_DIR_IN | D_GPIO_MODE_0);
     DrvGpioMode(I2C2_SDA1, D_GPIO_DIR_IN | D_GPIO_MODE_0);

     DrvGpioMode(I2C2_SDA2, D_GPIO_DIR_IN | D_GPIO_MODE_7);
     DrvGpioMode(I2C2_SDA3, D_GPIO_DIR_IN | D_GPIO_MODE_7);
}

void brd155GpioToI2cDevice(TBrd155I2cDevicesEnum tsh, I2CM_Device ** i2cDev)
{
    switch (tsh)
    {
    
    case ToshibaCam1:
         brd155GpioToCam1();
         // get new handler and use it later 
         brd155InitialiseI2C(&i2c1Mv155DefaultConfiguration2, i2cDev);
         break;
         
    case ToshibaCam2:
         brd155GpioToCam2();
         // get new handler and use it later 
         brd155InitialiseI2C(&i2c1Mv155DefaultConfiguration3, i2cDev);
         
         break;
    
    case ToshibaChip1:
    case ToshibaChip2:
    case ToshibaChip3:
    default:
         // set i2c gpio to bridges
         brd155GpioToBridges();                  
         // get another i2c module handler with config for this chip, 
         // TO DO : we should not do this all the time
         brd155InitialiseI2C(&i2c1Mv155DefaultConfiguration1, i2cDev);
         
         // assign the handler to the i2c modules
         brd155ReasignMipiBridgesI2cModule(*i2cDev);

         break;
    }
}

I2CM_StatusType brd155ToshibaCamRegSet(I2CM_Device * i2cDev, u32 data[][2], u32 len)
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

I2CM_StatusType brd155ToshibaCamRegGet(I2CM_Device * i2cDev, u32 data[][2], u32 len)
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
