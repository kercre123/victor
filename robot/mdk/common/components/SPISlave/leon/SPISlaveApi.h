///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup SPISlave SPI Slave API
/// @{
/// @brief     SPI Slave Functions API.
///


#ifndef _I2C_SLAVE_API_H_
#define _I2C_SLAVE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "SPISlaveApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

///Initialize the SPI slave interface
///@param hndl SPI interface handle
///@param blockNumber 
///@param cfg Timing configuration structure
///@param cb_ptr SPI callback function
int SPISlaveInit(spiSlaveHandle *hndl, u32 blockNumber, spiSlaveTiming *cfg, spiCallBackStruct_t *cb_ptr);
/// @}
#endif
