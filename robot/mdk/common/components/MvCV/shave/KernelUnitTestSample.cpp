///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave kernel example
///

// 1: Includes
// ----------------------------------------------------------------------------

#include <mv_types.h>
#include <KernelUnitTestSample.h>
#include <string.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
u8* lines[5];
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------


#ifdef __cplusplus
namespace mvcv{
#endif


void blendkernel(u8* in1, u8* in2, u8* out, u32 width, u32 height)
{
	u32 i, j;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			out[j + i * width] = in1[j + i * width] | in2[j + i * width];
		}
	}
	return;
}

u8* dummy_kernel_result(u8* in_tile, u8* out_tile_space, int width, int height)
{
	//Dummy kernel doesn't do anything, just copies data around
	memcpy(&out_tile_space[0], in_tile, width * height);
	return &out_tile_space[0];
}

//in height is implicitly 5 and out height 1
void avg3x5kernel_newint(u8** in_lines, u8** out_lines, u32 width)
{
	int x,y;
	unsigned int i;
	float sum;

	//Initialize lines pointers
	lines[0] = in_lines[0];
	lines[1] = in_lines[1];
	lines[2] = in_lines[2];
	lines[3] = in_lines[3];
	lines[4] = in_lines[4];

	//Go on the whole line
	for (i = 0; i < width; i++)
	{
		sum = 0.0f;
		for (x = 0; x < 5; x++)
		{
			for (y = 0; y < 3; y++)
			{
				sum += (float)lines[x][y];
			}
			lines[x]++;
		}
		out_lines[0][i] = (u8)(sum / 15.0f);
	}
	return;
}



//height is implicitly 3
void avg1x5kernel(u8** in, u8** out, u32 width)
{
	int x,y;
	unsigned int i;
	u8* lines[5];
	float sum;

	//Initialize lines pointers
	lines[0] = in[0];
	lines[1] = in[1];
	lines[2] = in[2];
	lines[3] = in[3];
	lines[4] = in[4];

	//Go on the whole line
	for (i = 0; i < width; i++)
	{
		sum = 0.0f;
		for (x = 0; x < 5; x++)
		{
			for (y = 0; y < 1; y++)
			{
				sum += (float)lines[x][y];
			}
			lines[x]++;
		}
		out[0][i] = (u8)(sum / 5.0f);
	}
	return;
}

#ifdef __cplusplus
}
#endif
