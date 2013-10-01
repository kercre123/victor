///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

// 1: Includes
// ----------------------------------------------------------------------------
#ifdef _WIN32
    #include <iostream>
    #include "myriadModel.h"
    #include <gaussVx2.h>
#endif

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
namespace mvcv
{

void GaussVx2(u8** p_inLine, u8 *p_outLine, int width)
{
    unsigned int gaus_matrix[3] = {16, 64,96};
    int i;

    u8* outLine = (u8*) mmGetPtr((u32)p_outLine);
    u8* inLine[5];
    for (i = 0; i < 5; i++)
      inLine[i] = (u8*) mmGetPtr((u32)p_inLine[i]);

    for (i = 0; i < width; i++)
    {
        outLine[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2])) >> 8;
    }
}

}


