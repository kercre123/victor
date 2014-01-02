///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     IR functions
///
/// This is the header that describes the IR operations.
///

#ifndef _INFRARED_SENSOR_H_
#define _INFRARED_SENSOR_H_


// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "mv_types.h"

// 2: Defines
// ----------------------------------------------------------------------------

#define TOLERANCE 33.3 // In percent [%], plus/minus
//#define IR_PRINTF(x...) { printf("IR: "); printf(x); }    // Enabled
#define IR_PRINTF(x...) {}                                  // Disabled

#define IR_detected(x) ((void)(x))
//#define IR_detected(x) (*(u32vp)0x80000000=(x))


#ifndef REMOTE_CONTROL_DATA
#define REMOTE_CONTROL_DATA(x) x
#endif

#ifndef REMOTE_CONTROL_CODE
#define REMOTE_CONTROL_CODE(x) x
#endif

#ifndef INFRA_RED_INTERRUPT_LEVEL
#define INFRA_RED_INTERRUPT_LEVEL 2
#endif

// Debug only
//#define STRUCT_MARGINS

#define KH_REMOTE
#ifdef KH_REMOTE
    #define RCU_CUSTOM_CODE_VAL     0xfe01
    #define RCU_MENU_VAL            0x0A
    #define RCU_UP_VAL              0x10
    #define RCU_DOWN_VAL            0x1F
    #define RCU_LEFT_BACK_VAL       0x04
    #define RCU_RIGHT_VAL           0x07
    #define RCU_SELECT_VAL          0x08
    #define RCU_SRC_SEL_VAL         0x02
    #define RCU_MODE_SEL_VAL        0x05
#endif //RP_RCU


// 3: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef volatile unsigned *u32vp;
typedef void (*callback)( int );


// 4: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
extern volatile u8 currentRemoteKey;


// 5:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// The parameters must be set in order to initiate the driver.
/// @param[in] infraRedGpio  the IR GPIO
/// @return    int
///
int NECInfraRedInit(int infraRedGpio);

/// Makes sure that the timer uses the system clock properly.
/// @param[in] infraRedGpio  the IR GPIO
/// @return    void
///
void NECInfraRedMain(int infraRedGpio);

/// Sets the cmd code.
/// @param[in] key  the command code
/// @return    void
///
void NECRemoteControlSetCurrentKey (u8 key);
#endif //_INFRARED_SENSOR_H_

