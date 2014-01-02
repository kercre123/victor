///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Boot daughtercard.
///
/// This is the implementation of a method to boot daughter card via i2c.
///
///

// 1: Includes
// ----------------------------------------------------------------------------

#include "bootDcardApi.h"
#include "DrvTimer.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
static u8 bootProto[] =
{
    S_ADDR_WR,      // First byte is the slaveAddress (with Write bit set)
    DATAW,          // Next an i2c write operation; the  byte written is dataBuffer[index++]
    LOOP_MINUS_1    // Loops back to DATAW and repeats until lengthOfWrite bytes have been written
};

u32 boot_dbg;
u32 boot_run;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

I2CM_StatusType bootDcardI2C(u8 *bootImg, u32 bootSize, I2CM_Device* dev)
{
	I2CM_StatusType retValue;
	tyI2cConfig     orgI2cConfig;
	u32 retry = 0;

	// overly complicated handling for i2c errors
	DrvI2cMGetCurrentConfig(dev,&orgI2cConfig);
	DrvI2cMSetErrorHandler(dev, NULL);

	do
	{
		retry++;
		// multiple tries to boot in case we need to wait due to power up sequence of dcard
		retValue = DrvI2cMTransaction(dev, BOOT_DCARD_ADDR_I2C, 0, bootProto, bootImg, bootSize);
		SleepMs(10);
		boot_dbg = retValue;
		boot_run = retry;
	} while ( (retValue != I2CM_STAT_OK) && (retry < BOOT_DCARD_TIMEOUT) );

	DrvI2cMSetErrorHandler(dev, orgI2cConfig.errorHandler);
	return retValue;
}

