/******************************************************************************
 @File         : harris_score.h
 @Author       : xxx xxx
 @Brief        : Containd a function that calculate harris score response
 Date          : 14 - 01 - 2013
 E-mail        : xxx.xxx@movidius.com
 Copyright     :  Movidius Srl 2013, Movidius Ltd 2013

HISTORY
 Version        | Date       | Owner         | Purpose
 ---------------------------------------------------------------------------
 1.0            | 14.01.2013 | xxx xxx    | First implementation
 ***************************************************************************/
  
#ifndef __HARRIS_SCORE_H__
#define __HARRIS_SCORE_H__

#include <mvstl_types.h>
#include "mvcv_types.h"

namespace mvcv
{
//!{
/// Computes Harris response over a patch of the image with a radius of 3. The patch
/// size is actually 8x8 to account for borders
/// @param[in] data			- Input patch including borders
/// @param[in] x			- X coordinate inside the patch. Only a value of 3 supported
/// @param[in] y			- Y coordinate inside the patch. Only a value of 3 supported
/// @param[in] step_width	- Step of the patch. Only a value 8 supported (2xradius + 2xborder)
/// @param[in] k			- Constant that changes the response to the edges. Typically 0.02 is used
/// return					- Corner response value
#ifdef __PC__

	#define HarrisResponse_asm HarrisResponse

#endif

#ifdef __MOVICOMPILE__

	extern "C" fp32 HarrisResponse_asm(u8 *data, u32 x, u32 y, u32 step_width, fp32 k);

#endif

fp32 HarrisResponse(u8 *data, u32 x, u32 y, u32 step_width, fp32 k);

}
#endif //__HARRIS_SCORE_H__
//!}