///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @addtogroup RemoteControl
/// @{
/// @brief     Definitions and types needed by the Remote Control protocol
///

#ifndef _REMOTE_CONTROL_API_DEFINES_H_
#define _REMOTE_CONTROL_API_DEFINES_H_


// 1: Defines
// ----------------------------------------------------------------------------

//#define RCU_PRINTF(x...) { printf("rcu: "); printf(x); }
#define RCU_PRINTF(x...) {}

// Debouncing control the shortest interval between successive key down events
#define RCU_DEBOUNCE_INTERVAL_MS        300
// This is a reserved key that should never be used
#define RCU_NULL_VAL            0xFF
// Number of functions we need from the Remote Control Unit
#define RCU_FUNCTIONS_NO    10
#define IR_GPIO 58
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
enum rcu_fct
{
    RCU_CUSTOM_CODE,
    RCU_NULL,
    RCU_MENU,
    RCU_UP,
    RCU_DOWN,
    RCU_LEFT_BACK,
    RCU_RIGHT,
    RCU_SELECT,
    RCU_SRC_SEL,
    RCU_MODE_SEL,
};


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
/// @}
#endif //_REMOTE_CONTROL_API_DEFINES_H_

