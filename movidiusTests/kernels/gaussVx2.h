///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Gauss Vx2 filter
///
///    Containd a function that applay downscale 2x vertical with a gaussian filters with kernel 5x5. 
///    Have to be use in combination with GaussHx2 for obtain correct output. 
///    Gaussian 5x5 filter was decomposed in liniar 1x5, applayed horizontal and vertical
  
#ifndef __GAUSS_V_X2_H__
#define __GAUSS_V_X2_H__

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Gauss vertical kernel on PC
/// @param[in] in line address
/// @param[in] out line address
/// @param[in] in line width
/// @return void
namespace mvcv
{
#ifdef __PC__
    void GaussVx2(u8** p_inLine, u8* p_outLine, int width);
#else
    extern "C"
    void GaussVx2(u8** p_inLine, u8* p_outLine, int width);
#endif
}
#endif //__GAUSS_V_X2_H__
