///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Lcd Functions.
///
/// This is the implementation of Lcd library.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "isaac_registers.h"

#include "DrvGpio.h"
#include "DrvLcd.h"
#include "DrvCif.h"
#include "DrvCpr.h"
#include "DrvSvu.h"

#include "DrvIcb.h"
#include "DrvIcbDefines.h"
#include "DrvTimer.h"
#include <stdio.h>
#include "app_config.h"
#include "LcdMv119Api.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------
void Lcd119GpioConfig(void) {
	DrvGpioModeRange(100, 102, 0x0000); // pclk, hsink, vsink
	DrvGpioModeRange(104, 111, 0x0000); // data 0 - 7
	DrvGpioModeRange(133, 136, 0x0003); // data 8 - 11
	DrvGpioModeRange(129, 132, 0x0003); // data 12 - 15
	DrvGpioModeRange(127, 128, 0x0003); // data 16 - 17
	DrvGpioModeRange(137, 140, 0x0003); // data 18 - 21
	DrvGpioModeRange(97, 98, 0x0003); // data 22 - 23

	DrvGpioMode(103, 0x0000); // lcd data enable
	DrvGpioMode(112, 0x0007); // lcd backlight control pin
	DrvGpioMode(113, 0x0007); // lcd disable pin - 1 to enable
	DrvGpioSetPinHi(112);   // Turn LCD backlight on
	DrvGpioSetPinHi(113);   // Turn LCD on
}
