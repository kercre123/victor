///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup Myriad2ShaveModulePing Myriad2 Ping test API for Shave processors
/// @{
/// @brief     Ping test for all devices using Shave processor
///

#ifndef MYRIAD2_SHAVE_MODULE_PING_H
#define MYRIAD2_SHAVE_MODULE_PING_H
#include <mv_types.h>


///Ping all modules on the bus and report errors if found
///@param void
int Myriad2ShaveModulePing(void);
/// @}
#endif //MYRIAD2_SHAVE_MODULE_PING_H
