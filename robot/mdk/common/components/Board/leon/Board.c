///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Board setup.
///
/// This is the implementation of all board setup handling events.
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvGpioDefines.h"

#include "DrvGpio.h"
#include "DrvI2cMaster.h"
#include "brdMv0153.h"
#include "swcLedControl.h"
#include "BoardApi.h"
#include "assert.h"

// Note: this include contains static initial data so should
// only be included once. 
#include "brdGpioCfgs/brdMv0153GpioDefaults.h" 
#include "stdio.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// TODO: This needs to be removed
#define BTN0 84
#define BTN1 50
#define BTN2 51

#define SW0 51
#define SW1 50
#define SW2 84
#define SW3 83

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
tyAppDeviceHandles gAppDevHndls;
                   
// 4: Static Local Data
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int BoardInitialise(void)
{
    DrvGpioIrqResetAll();
    DrvGpioInitialiseRange(brdMv0153GpioCfgDefault);

    brd153InitialiseLeds();

    brd153InitialiseI2C(NULL,NULL,                 // Use Default I2C configuration
                        &gAppDevHndls.i2c1Handle,
                        &gAppDevHndls.i2c2Handle);

    brd153SetCoreVoltage(MV0153_DEFAULT_CORE_VOLTAGE); 

    brd153ExternalPllConfigure(EXT_PLL_CFG_74MHZ);

    // Reset Devices and take them out of reset
    brd153Reset(MV0153_RESET_ALL,RST_NORMAL);

    // Give 5 flashes of cylon eye to indicate that board setup OK.
    swcLedCylonEye(1,swcLedGetNumLeds(),2,5,20);

    return 0;
}


unsigned int BoardGetButtonState(unsigned int buttonNum)
{
  switch (buttonNum)
  {
	case 0:  return DrvGpioGetRaw(BTN0);
	case 1:  return DrvGpioGetRaw(BTN1);
	default: return DrvGpioGetRaw(BTN2);
  }
}

unsigned int BoardGetDIPSwitchState(unsigned int switchDIPNum)
{
  switch (switchDIPNum)
  {
	case 0:  return DrvGpioGetRaw(SW0);
	case 1:  return DrvGpioGetRaw(SW1);
	case 2:  return DrvGpioGetRaw(SW2);
	default:  return DrvGpioGetRaw(SW3);
  }
}

