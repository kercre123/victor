#ifdef _WIN32
#include <math.h>
#include <iostream>
#endif

#include <stdio.h>

#include "harris.h"
#include "ac_defs.h"
#include "mvcv_types.h"
#include <alloc.h>
#include <mvstl_utils.h>

namespace mvcv
{

#define FILTER_VAL 9

//Crop an image patch by splitting it in height DMA transfers, each having width length
void stride_copy(unsigned char *src,
	int           src_stride,
	unsigned char *dst,
	int           dst_stride,
	int           width,
	int           height)
{
	int i;

	for (i = 0; i < height; i++)
	{
		dma_copy(dst, src, width);

		src += src_stride;
		dst += dst_stride;
	}
}

//Computes the sum of pixels within the rectangle specified by the top-left start co-ordinate and size
inline float BoxIntegral(int* data, int row, int col, int rows, int cols, unsigned int width) 
{
	const float one_over_255 = (1.0f / 255.0f);

	int r1 = (row        - 1)*width;
	int c1 = (col        - 1);
	int r2 = (row + rows - 1)*width;
	int c2 = (col + cols - 1);

	int A = data[r1 + c1];
	int B = data[r1 + c2];
	int C = data[r2 + c1];
	int D = data[r2 + c2];

	return  ((A - B - C + D) * one_over_255);
}

//Calculate corner responses for supplied integral image
void buildResponseLayer(FastHessian *fh, unsigned int width, unsigned int height, unsigned int grid_width, unsigned int grid_height)
{
	int			   b = (fh->responseMap.filter - 1) / 2 + 1;    // border for this filter
	int			   l = fh->responseMap.filter / 3;              // lobe for this filter (filter size / 3)
	int			   w = fh->responseMap.filter;                  // filter size
	float inverse_area = 1.f / (w * w);							// normalisation factor

	float Dxx, Dyy, Dxy;
	unsigned int ar, ac;

	int* img_int = (int*)malloc((CORNER_FILTER_SIZE + 1) * width * sizeof(int));
	float* responses = (float*)malloc(width * sizeof(float));

	for (ar = (grid_height - 1); ar < (height - (grid_height - 1)); ++ar) 
	{	  
		// copy DDR -> CMX
		stride_copy((unsigned char*)(fh->img + (ar - 2 * l) * width * 4), width * 4,
			(unsigned char*)img_int, width * 4, 
			width * 4, CORNER_FILTER_SIZE + 1);

		for (ac = (grid_width - 1); ac < (width - (grid_width - 1)); ++ac) 
		{
			int r = 6;

			//andreil: se calculeaza diferenta ca dreptunghiul mare - dreptunghiul mic !!!
			// dreptunghiul negativ are pondere -2, dar in plus, tre scazut si parte alba de sub dreptunchiul negru
			// deci -3, de aia se inmulteste cu -3 !!!

			// Compute response components
			Dxx = BoxIntegral(img_int, r - l + 1, ac - b,      2*l - 1, w, width)
				- BoxIntegral(img_int, r - l + 1, ac - l / 2,  2*l - 1, l, width) * 3;

			Dyy = BoxIntegral(img_int, r - b,     ac - l + 1, w,  2*l - 1, width)
				- BoxIntegral(img_int, r - l / 2, ac - l + 1, l,  2*l - 1, width) * 3;

			Dxy = + BoxIntegral(img_int, r - l, ac + 1, l, l, width)
				+ BoxIntegral(img_int, r + 1, ac - l, l, l, width)
				- BoxIntegral(img_int, r - l, ac - l, l, l, width)
				- BoxIntegral(img_int, r + 1, ac + 1, l, l, width);

			// Normalise the filter responses with respect to their size
			Dxx *= inverse_area;
			Dyy *= inverse_area;
			Dxy *= inverse_area;

			responses[ac] = (Dxx * Dyy - 0.912f * Dxy * Dxy);
			//responses[ac] = MIN(abs(Dxx), abs(Dyy)); //KLT corner
			//responses[ac] = MIN(Dxx, Dyy); //Shi-Tomasi
		}

		// copy CMX -> DDR
		stride_copy((unsigned char*)responses, width * 4,
			(unsigned char*)(fh->responseMap.responses + ar * width), width * 4,
			width * 4, 1);
	}

	free(img_int);
	free(responses);
}

inline float getResponse(float *rl, unsigned int row, unsigned int column, unsigned int grid_width)
{	
	return rl[row * grid_width + column];
}

void my_memset(int *dest, int val, int num)
{
	while(num--)
		*dest++ = val;
}

inline void add_Interest_Point(FastHessian *fh, int r, int c)
{
	int idx;

	idx = fh->ipts->size;
	fh->ipts->size++; //increment to make room for next element
	fh->ipts->pts[idx].xL = c;
	fh->ipts->pts[idx].yL = r;
}

//Select corners based on threshold and localization
void getInterestPoints(FastHessian* fh, CvSize size, unsigned int grid_width, unsigned int grid_height)
{
	const unsigned int	width = size.width;
	const unsigned int	height = size.height;
	const unsigned int	num_G_W = width  / grid_width;
	const unsigned int	num_G_H = height / grid_height;
	float		cell_max_resp;
	int			cell_max_X;
	int			cell_max_Y;
	unsigned int gx, gy, i, j;
	float*		responses;
	float		response_value;
	int			count = 0;  

	//Clear the vector of exisiting ipts
	fh->ipts->size = 0;

	//Build the response map
	buildResponseLayer(fh, width, height, grid_width, grid_height);

	responses = (float*)malloc(grid_width * grid_height * sizeof(float));

	//Consider a border of g_GRID_W vertical and g_GRID_H horizontal where no points should be detected
	for (gy = 1; gy < num_G_H - 1; gy++)
	{
		for (gx = 1; gx < num_G_W - 1; gx++)
		{	
			//Bring one patch DDR -> CMX
			stride_copy((unsigned char*)(fh->responseMap.responses + gy * width * grid_height + gx * grid_width), width * 4,
				(unsigned char*)responses, grid_width * 4,
				grid_width * 4, grid_height);

			cell_max_resp = 0;

			//Find maximum response in current patch
			for (i = 0; i < grid_height; i++)
				for (j = 0; j < grid_width; j++)
				{
					response_value = getResponse(responses, i, j, grid_width);
					if (response_value > cell_max_resp)
					{
						cell_max_resp = response_value;
						cell_max_X    = gx * grid_width + j;
						cell_max_Y    = gy * grid_height + i;
					}
				}

			if ( /*(cell_max_resp > 0) &&*/ (cell_max_resp > fh->thresh) && (count < MAX_POINTS) )
			{			  			  
				count++;
				add_Interest_Point(fh, cell_max_Y, cell_max_X);
			}
		}
	}

	free(responses);
}

inline void swap_pint(int** a, int** b)
{
	int* tmp = *a;
	*a = *b;
	*b =  tmp;
}

//Compute integral image of the input image
void Integral(MVIplImage* src, MVIplImage* dst, CvSize size)
{
	const int width					= size.width;
	const int height				= size.height;
	const int src_line_len			= width * sizeof(char);
	const int dst_line_len			= width * sizeof(int);
	int* prev_line_buff				= (int*)malloc(dst_line_len);
	int* cur_line_buff				= (int*)malloc(dst_line_len);
	unsigned char* src_line_buff	= (unsigned char*)malloc(src_line_len);
	
	int* prev_line = prev_line_buff;
	int* cur_line = cur_line_buff;
	unsigned char* src_line = src_line_buff;
	
	//Bring first line to local buffer
	dma_copy(src_line, src, src_line_len);

	//Calculate first line of integral image
	cur_line[0] = src_line[0];
	for (int i = 1; i < width; i++)
		cur_line[i] = cur_line[i - 1] + src_line[i];

	//Evict first line to main buffer
	dma_copy(dst, (unsigned char*)cur_line, dst_line_len);

	//Swap local buffers (double buffering)
	swap_pint(&prev_line, &cur_line);

	//For the next (height - 1) lines: bring one line -> calculate integral image -> throw out
	for (int i = 1; i < height; i++)
	{
		dma_copy(src_line, src + i * src_line_len, src_line_len);

		int row_sum = 0;

		for (int j = 0; j < width; j++)
		{
			row_sum += src_line[j];
			cur_line[j] = row_sum + prev_line[j];
		}

		dma_copy(dst + i * dst_line_len, (unsigned char*)cur_line, dst_line_len);

		swap_pint(&prev_line, &cur_line);
	}

	free(src_line_buff);
	free(cur_line_buff);
	free(prev_line_buff);
}

inline void copy_points(CvPoint2D32f dst[], const IpVec* src)
{
	for (int i = 0; i < src->size; i++)
	{
		dst[i].x = (float)src->pts[i].xL;
		dst[i].y = (float)src->pts[i].yL;
	}
}

//Finds interest points (corners) in an image using Harris corner detector and returns their count
int harris(MVIplImage* input_img,	//Image to find corners in
	MVIplImage* integral_img,		//Integral image buffer 
	MVIplImage* responses_img,		//Filter responses buffer
	CvSize size,					//Size of input image
	CvPoint2D32f** corners,			//Found corners vector
	float    thres)					//Corner response threshold
{  
	//printf("Harris...");

	CvPoint2D32f* _corners = null;
	IpVec* ipts = (IpVec*)malloc(sizeof(IpVec));
	FastHessian fh;

	fh.img    = integral_img;
	fh.ipts   = ipts;
	fh.thresh = thres;
	fh.responseMap.filter = FILTER_VAL;
	fh.responseMap.responses = (float*)responses_img;

	//Calculate integral image
	Integral(input_img, integral_img, size);

	//Extract interest points and store in vector ipts
	getInterestPoints(&fh, size, GRID_CELLS_W, GRID_CELLS_H);

	if (ipts->size > 0)
	{
		//Copy interest points in the output vector
		_corners = (CvPoint2D32f*)malloc(ipts->size * sizeof(CvPoint2D32f));
		copy_points(_corners, ipts);
	}

	*corners = _corners;

	free(ipts);

	//printf("done\n");

	return ipts->size;
}

}