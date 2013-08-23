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
    #include <stdio.h>
    #include "myriadModel.h"
    #include <gaussHx2.h>
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

void GaussHx2(u8 *inLine, u8 *outLine, int width)
{
    unsigned int gaus_matrix[3] = {16, 64, 96 };
    int i, c = 0;
    u8* pInLine  = mmGetPtr((u32)inLine);
    u8* pOutLine = mmGetPtr((u32)outLine);
    

    for (i = 0; i < (width << 1); i += 2)
    {
        pOutLine[i >> 1] = (((pInLine[i - 2] + pInLine[i + 2]) * gaus_matrix[0]) + ((pInLine[i - 1] + pInLine[i + 1]) * gaus_matrix[1]) + (pInLine[i] * gaus_matrix[2])) >> 8;
        c++;
    }
}

}
