/******************************************************************************
 @File         : harris_score.cpp
 @Author       : xxx xxx
 @Brief        : Containd a function that calculate harris score response
 Date          : 14 - 01 - 2013
 E-mail        : xxx.xxx@movidius.com
 Copyright     : C Movidius Srl 2013, C Movidius Ltd 2013

HISTORY
 Version        | Date       | Owner         | Purpose
 ---------------------------------------------------------------------------
 1.0            | 14.01.2013 | xxx xxx    | First implementation
 ***************************************************************************/

#ifdef _WIN32
#include <iostream>
#endif

#include <mvcv_types.h>
#include <mvcv_internal.h>

#include <math.h>
#include <alloc.h>
#include <mvstl_utils.h>

namespace mvcv
{

void dummyHarrisResponse()
{
}

fp32 HarrisResponse(u8 *data, u32 x, u32 y, u32 step_width, fp32 k) 
{
#define RADIUS  (3) // asm, for speed reason suport jus radius 3 

	int dx;
	int dy;

	float xx = 0;
	float xy = 0;
	float yy = 0;

	// Skip border and move the pointer to the first pixel
	data += step_width + 1;

	for (u32 r = y - RADIUS; r < y + RADIUS; r++)
	{
		for (u32 c = x - RADIUS; c < x + RADIUS; c++)
		{
			int index = r * step_width + c;
			dx = data[index - 1] - data[index + 1];
			dy = data[index - step_width] - data[index + step_width];
            //printf ("%x - %x\n", dx, dy);
			xx += dx * dx;
			xy += dx * dy;
			yy += dy * dy;
		}
	}

	float det = xx * yy - xy * xy;
	float trace = xx + yy;

	//k changes the response of edges.
	//seems sensitive to window size, line thickness, and image blur =o/
	return (det - k * trace * trace);
}

}

