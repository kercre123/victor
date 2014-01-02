///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Remote Control Functions.
///
/// This is the implementation of configuration Remote Control functions
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "InfraRedSensor.h"
#include "RemoteControlApi.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
u32 RemoteControlKeys[RCU_FUNCTIONS_NO] = { RCU_CUSTOM_CODE_VAL,
                                            RCU_NULL_VAL,
                                            RCU_MENU_VAL,
                                            RCU_UP_VAL,
                                            RCU_DOWN_VAL,
                                            RCU_LEFT_BACK_VAL,
                                            RCU_RIGHT_VAL,
                                            RCU_SELECT_VAL,
                                            RCU_SRC_SEL_VAL,
                                            RCU_MODE_SEL_VAL};


// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Verify the system clock in order to catch a correct command.
void REMOTE_CONTROL_CODE(NECRemoteControlMain) (int infraRedGpio)
{
    NECInfraRedMain(infraRedGpio);
    return;
}

// Initialize the remote control functionality.
void REMOTE_CONTROL_CODE(NECRemoteControlInit) (int infraRedGpio)
{
    currentRemoteKey = RCU_NULL;
    NECInfraRedInit(infraRedGpio);

    return;

}

// Gets the current command code.
u8 REMOTE_CONTROL_CODE(NECRemoteControlGetCurrentKey) (void)
{
    u8 key = currentRemoteKey;
    currentRemoteKey = RCU_NULL;
    return key;
}

// Returns the button that generated the command.
int REMOTE_CONTROL_CODE(NECRemoteControlGetButton) (unsigned int detectedCode)
{
    int i = 0;

    for (i = 0; i < RCU_FUNCTIONS_NO; i++)
    {
        if (RemoteControlKeys[i] == detectedCode)
        {
            return i;
        }
    }

    return RCU_NULL;
}
