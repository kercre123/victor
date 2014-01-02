///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup BoardApi Board Setup Functions
/// @{
/// @brief     Board Setup Functions API.
///
/// This is the API to board setup library implementation.
///

#ifndef _BOARD_API_H_
#define _BOARD_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "BoardApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
extern tyAppDeviceHandles gAppDevHndls;


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// This function completely initialses the MV0153 board and sets up all GPIOS
/// @return Non-zeron on error   
int BoardInitialise(void);

/// This function returns GPIO value of button button_num
/// @param[in] button_num button to be checked
/// @return GPIO value
///
unsigned int BoardGetButtonState(unsigned int button_num);

/// This function returns GPIO value of DIP switch switchDIPNum
/// @param[in] switchDIPNum switch to be checked
/// @return GPIO value
///
unsigned int BoardGetDIPSwitchState(unsigned int switchDIPNum);
/// @}
#endif // _BOARD_API_H_
