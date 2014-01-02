///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Implementation of the LT3906 Regulator driver
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <isaac_registers.h>
#include <DrvGpio.h>
#include <DrvI2c.h>
#include "DrvI2cMaster.h"

#include "icRegulatorLT3906.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define REG_SYS_CONTROL_1   (0x7 )
#define REG_BUCK1_TGT_V1    (0x23)
#define REG_BUCK1_TGT_V2    (0x29)
#define REG_LDO1_VCTRL      (0x39)
#define REG_LDO2_VCTRL      (0x3A)
#define REG_VCHANGE_CTRL_1  (0x20)
#define REG_BUCK_LDO_ENABLE (0x10)
#define REG_BUCK_LDO_STATUS (0x11)

// VCHANGE Bitfields
#define B1GO                (1 << 0)
#define B1VT1               (0 << 1)
#define B1VT2               (1 << 1)
#define B2GO                (1 << 4)
#define B2VT1               (0 << 5)
#define B2VT2               (1 << 5)


#define REGULATOR_POLL_COUNT (100) // Max times to poll before giving up on regulator settling 
							 	  // Emperical evidence suggests that only one poll will be necessary, so margin is great

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
static u32 localErrors          = 0;
static u8 simpleWriteProto[]    = I2C_PROTO_WRITE_8BA;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static int calculateRailConfig(tyLT3906Config * pCfg,u8 * volt, u8 * railsToEnable,u8 * expectedStatus);
static u32 regI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2);
static const u32             reg[]               = {0x23,0x29,0x39,0x3A}; 
static const u8              SYS_CTRL_DEFAULT    = 0x28;
static const u8              VCHANGE_VALUE       = B1GO | B2GO | B1VT1 | B2VT1;


// 6: Functions Implementation
// ----------------------------------------------------------------------------

int icLT3906Configure(I2CM_Device * i2cDev,u32 moduleAddr,tyLT3906Config * pCfg)
{
    tyI2cConfig     orgI2cConfig;
    u8              railsToEnable;

    //  First Calculate the required Voltage register settings
    calculateRailConfig(pCfg,pCfg->voltRegCache,&railsToEnable,&(pCfg->voltRegCache[4]));

    // In this module we care about the error return code for the I2C
    // As such we will override the default I2C error handler with our own version
    // To avoid clashing with anyone else, we will first backup and restore the I2C configuration

    // Backup I2C Configuration
    DrvI2cMGetCurrentConfig(i2cDev,&orgI2cConfig);

    // Setup our own error handler
    DrvI2cMSetErrorHandler(i2cDev,&regI2CErrorHandler);

    localErrors = 0; // Reset the Error Counter 

    // Then write the configuration
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_SYS_CONTROL_1    ,simpleWriteProto , (u8*)&SYS_CTRL_DEFAULT              , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_BUCK1_TGT_V1     ,simpleWriteProto , &(pCfg->voltRegCache[0])       , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_BUCK1_TGT_V2     ,simpleWriteProto , &(pCfg->voltRegCache[1])       , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_LDO1_VCTRL       ,simpleWriteProto , &(pCfg->voltRegCache[2])       , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_LDO2_VCTRL       ,simpleWriteProto , &(pCfg->voltRegCache[3])       , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_VCHANGE_CTRL_1   ,simpleWriteProto , (u8*)&VCHANGE_VALUE                 , 1);
    DrvI2cMTransaction(i2cDev,moduleAddr, REG_BUCK_LDO_ENABLE  ,simpleWriteProto , &railsToEnable                 , 1);

    if (localErrors)
        goto handleError;

    DrvI2cMSetErrorHandler(i2cDev,orgI2cConfig.errorHandler);
    // Otherwise we assume that all is well
    return 0;

handleError:
    // Restore original error handler
    DrvI2cMSetErrorHandler(i2cDev,orgI2cConfig.errorHandler);
    return localErrors;
}

int icLT3906CheckVoltageOK(I2CM_Device * i2cDev,u32 moduleAddr,tyLT3906Config * pCfg)
{
    tyI2cConfig     orgI2cConfig;
    u8              currentStatus;
    u8              value;
    u32             statusWaitTimeout = REGULATOR_POLL_COUNT;
    int             i;

    // In this module we care about the error return code for the I2C
    // As such we will override the default I2C error handler with our own version
    // To avoid clashing with anyone else, we will first backup and restore the I2C configuration

    // Backup I2C Configuration
    DrvI2cMGetCurrentConfig(i2cDev,&orgI2cConfig);

    // Setup our own error handler
    DrvI2cMSetErrorHandler(i2cDev,&regI2CErrorHandler);

    localErrors = 0; // Reset the Error Counter 

    // Next we read back the 4 voltage configuration registers to ensure we set the right voltages
    for (i=0;i<4;i++)
    {
        value=DrvI2cMReadByte(i2cDev,moduleAddr,reg[i]);
        if (value != pCfg->voltRegCache[i])
            goto handleError;
    }

    if (localErrors)
        goto handleError;

    // Then we wait to see that the Regulator achieves the correct status
    do
    {
        currentStatus = DrvI2cMReadByte(i2cDev,moduleAddr,REG_BUCK_LDO_STATUS);
        if (( currentStatus & pCfg->voltRegCache[4]) == pCfg->voltRegCache[4])
            break;
    } while (--statusWaitTimeout);

    if (statusWaitTimeout == 0)
    {
        localErrors = -99;
        goto handleError;
    }

    // Otherwise we assume that all is well
    DrvI2cMSetErrorHandler(i2cDev,orgI2cConfig.errorHandler);
    return 0;

handleError:
    // Restore original error handler
    DrvI2cMSetErrorHandler(i2cDev,orgI2cConfig.errorHandler);
    return localErrors;
}

static u32 regI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2)
{
    error=error;    // Parameters not actually needed.
    param1=param1;
    param2=param2; 
    localErrors++;
    return 0; // Return success as we have dealt with the error
}

static int calculateRailConfig(tyLT3906Config * pCfg,u8 * volt, u8 * railsToEnable,u8 * expectedStatus)
{
    *railsToEnable  = 0;
    *expectedStatus = 0;

    *railsToEnable |= (1<<5); // Bit 5 is reserved and set to 1 

    if (pCfg->switchedVoltage1 != 0) 
    {
      volt[0] = pCfg->switchedVoltage1;
      if ((volt[0] >= 80) && (volt[0] <= 200)) 
      {
        *railsToEnable  |= (1<<0); // Enable BK1
        *expectedStatus |= (1<<0); // Expect BK1_OK
        volt[0] = (volt[0]-80)/5 + 1 ; // what's the filed 
      }
      else
          return -1;
    }

    if (pCfg->switchedVoltage2 != 0)
    {
      volt[1] = pCfg->switchedVoltage2;
      if ((volt[1] >= 10) && (volt[1] <= 35)) 
      {
        *railsToEnable  |= (1<<2); // Enable BK2
        *expectedStatus |= (1<<2); // Expect BK2_OK
        if (volt[1]<=22)
          volt[1] = (volt[1]-10) + 1 ; // what's the filed 
        else    
          volt[1] = (volt[1]-10)  ; // what's the filed 
      }
      else
          return -2;
    }

    if  (pCfg->ldoVoltage1  != 0)  
    {
      volt[2] = pCfg->ldoVoltage1 ;
      if ((volt[2] >= 10) && (volt[2] <= 35)) 
      {
        *railsToEnable   |= (1<<4); // Enable LDO1
        *expectedStatus  |= (1<<4); // Expect LDO1_OK
        if (volt[2]<=22)
          volt[2] = (volt[2]-10) + 1 ; // what's the filed 
        else    
          volt[2] = (volt[2]-10)  ; // what's the filed 
      }
      else
          return -3;
    }

    if (pCfg->ldoVoltage2   != 0) 
    {
      volt[3] = pCfg->ldoVoltage2 ;
      if ((volt[3] >= 10) && (volt[3] <= 35)) 
      {
        *railsToEnable  |= (1<<6); // Enable LDO2
        *expectedStatus |= (1<<5); // Expect LDO2_OK
        if (volt[3]<=22)
            volt[3] = (volt[3]-10) + 1 ; // what's the filed 
        else    
            volt[3] = (volt[3]-10)  ; // what's the filed 
      }
      else
          return -4;
    }
    return 0;
}
