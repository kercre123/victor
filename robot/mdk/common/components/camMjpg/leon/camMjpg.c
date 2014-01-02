#include "DrvSvu.h"
#include "mv_types.h"
#include "stdio.h"
#include "camMjpgApi.h"
#include "swcSliceUtils.h"

unsigned int DBG_VAR = 0;

void JPG_component_test()
{
	printf("CamMjpg component call.\n");
}

void process_jpeg(unsigned int sensor_in, unsigned int jpeg_out)
{
	int i = 0;
	unsigned int block_size = 0;
	
	unsigned int *in_buffer;
	unsigned int *jpeg_buffer;
	
	in_buffer = sensor_in;
	jpeg_buffer = jpeg_out;
	
	block_size = ((in_buffer[0] & 0xFF) << 8) | ((in_buffer[0] & 0xFF00) >> 8);
	
	swcDmaMemcpy(0, jpeg_out, sensor_in, 100, 0);
	
	DBG_VAR = block_size;
	//printf("First word: %x.\n", block_size);
	
	
}
