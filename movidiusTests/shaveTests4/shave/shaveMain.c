///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave hello world source code
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

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

char helloShave[250];

int main( void )
{
#ifdef MYRIAD1
    printf("\n\nHello from SHAVE!\n\n\n");
#else //MYRIAD2
    strncpy(helloShave, "Hello from SHAVE!\n", 250);
#endif

    return 0;
}
