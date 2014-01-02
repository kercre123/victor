#ifdef _WIN32
#include <iostream>
#endif

#include <calcOpticalFlow.h>

#include <math.h>

#include <alloc.h>

#include <mvstl_utils.h>
#include <mvcv_types.h>
#include <mvcv_internal.h>
#include <samplers.h>


namespace mvcv
{

//    Purpose: 
//      Calculates optical flow between two images for certain set of points. 
//    Context: 
//    Parameters: 
//            imgA     - pointer to first frame (time t) 
//            imgB     - pointer to second frame (time t+1) 
//            imgStep  - full width of the source images in bytes 
//            imgSize  - size of the source images 
//            pyrA     - buffer for pyramid for the first frame. 
//                       if the pointer is not NULL, the buffer must have size enough to 
//                       store pyramid (from level 1 to level #<level> (see below)) 
//                       (imgSize.width*imgSize.height/3 will be enough)). 
//            pyrB     - similar to pyrA, but for the second frame. 
//                        
//                       for both parameters above the following rules work: 
//                           If pointer is 0, the function allocates the buffer internally, 
//                           calculates pyramid and releases the buffer after processing. 
//                           Else (it should be large enough then) the function calculates 
//                           pyramid and stores it in the buffer unless the 
//                           CV_LKFLOW_PYR_A[B]_READY flag is set. In both cases 
//                           (flag is set or not) the subsequent calls may reuse the calculated 
//                           pyramid by setting CV_LKFLOW_PYR_A[B]_READY. 
// 
//            featuresA - array of points, for which the flow needs to be found 
//            count    - number of feature points  
//            winSize  - size of search window on each pyramid level 
//            level    - maximal pyramid level number 
//                         (if 0, pyramids are not used (single level), 
//                          if 1, two levels are used etc.) 
// 
//            next parameters are arrays of <count> elements. 
//            ------------------------------------------------------ 
//            featuresB - array of 2D points, containing calculated 
//                       new positions of input features (in the second image). 
//            status   - array, every element of which will be set to 1 if the flow for the 
//                       corresponding feature has been found, 0 else. 
//            error    - array of double numbers, containing difference between 
//                       patches around the original and moved points 
//                       (it is optional parameter, can be NULL). 
//            ------------------------------------------------------ 
//            criteria   - specifies when to stop the iteration process of finding flow 
//                         for each point on each pyramid level 
// 
//            flags      - miscellaneous flags: 
//                            CV_LKFLOW_PYR_A_READY - pyramid for the first frame 
//                                                      is precalculated before call 
//                            CV_LKFLOW_PYR_B_READY - pyramid for the second frame 
//                                                      is precalculated before call 
//                            CV_LKFLOW_INITIAL_GUESSES - featuresB array holds initial 
//                                                       guesses about new features' 
//                                                       locations before function call. 
//    Returns: CV_OK       - all ok 
//             CV_OUTOFMEM_ERR - insufficient memory for function work 
//             CV_NULLPTR_ERR  - if one of input pointers is NULL 
//             CV_BADSIZE_ERR  - wrong input sizes interrelation 
// 
//    Notes:  For calculating spatial derivatives 3x3 Scharr operator is used. 
//            The values of pixels beyond the image are determined using replication mode. 
//F*/ 

/*!
Aligns buffer size by the certain number of bytes

This small inline function aligns a buffer size by the certian number of bytes by enlarging it.
*/
inline size_t alignSize(size_t sz, int n)
{
	return (sz + n-1) & -n;
}
/*!
Aligns pointer by the certain number of bytes

This small inline function aligns the pointer by the certian number of bytes by shifting
it forward by 0 or a positive offset.
*/  
inline int* alignPtr(int* ptr, int n)
{
	return (int*)(((size_t)ptr + n-1) & -n);
}

/*
Various border types, image boundaries are denoted with '|'

* BORDER_REPLICATE:     aaaaaa|abcdefgh|hhhhhhh
* BORDER_REFLECT:       fedcba|abcdefgh|hgfedcb
* BORDER_REFLECT_101:   gfedcb|abcdefgh|gfedcba
* BORDER_WRAP:          cdefgh|abcdefgh|abcdefg        
* BORDER_CONSTANT:      iiiiii|abcdefgh|iiiiiii  with some specified 'i'
*/
inline int borderInterpolate( int p, int len, int borderType )
{
	int delta = borderType == 4;
	do
	{
		if( p < 0 )
			p = -p - 1 + delta;
		else
			p = len - 1 - (p - len) - delta;
	}
	while( (unsigned)p >= (unsigned)len );
	return p;
}

void cvPyrDown( CvMat* _src, CvMat* _dst, int _dsz, CvSize _src_size, CvSize _dst_size )
{
	const int PD_SZ = 5;
	CvSize ssize = _src_size, dsize = _dst_size;
	int cn = 1;
	int bufstep = (int)alignSize(dsize.width*cn, 16);
	int* _buf = (int*)mv_malloc((bufstep*PD_SZ + 16) * sizeof(int));  
	int* buf = alignPtr((int*)_buf, 16);
	int* tabL = (int*)mv_malloc((PD_SZ + 2) * sizeof(int));
	int* tabR = (int*)mv_malloc((PD_SZ + 2) * sizeof(int));
	//int tabL[512 * (PD_SZ + 2)];
	//int tabR[512 * (PD_SZ + 2)];
	int *_tabM = (int*)mv_malloc((dsize.width*cn) * sizeof(int));  
	int* tabM = _tabM;
	int* rows[PD_SZ];
	
	svu_assert(_src->height > 0 && _src->height < 2048);
	svu_assert(_src->width > 0 && _src->width < 2048);
	svu_assert(_dst->height > 0 && _dst->height < 2048);
	svu_assert(_dst->width > 0 && _dst->width < 2048);
	svu_assert(_src_size.height > 0 && _src_size.height < 2048);
	svu_assert(_src_size.width > 0 && _src_size.width < 2048);
	svu_assert(_dst_size.height > 0 && _dst_size.height < 2048);
	svu_assert(_dst_size.width > 0 && _dst_size.width < 2048);
	svu_assert(tabL != null && tabR != null && _tabM != null && buf != null);

	int k, x, sy0 = -PD_SZ/2, sy = sy0, width0;
	width0 = MIN((ssize.width-PD_SZ/2-1)/2 + 1, dsize.width);

	for( x = 0; x <= PD_SZ+1; x++ )
	{
		int sx0 = borderInterpolate(x - PD_SZ/2, ssize.width, 4)*cn;
		int sx1 = borderInterpolate(x + width0*2 - PD_SZ/2, ssize.width, 4)*cn;
		for( k = 0; k < cn; k++ )
		{
			tabL[x*cn + k] = sx0 + k;
			tabR[x*cn + k] = sx1 + k;
		}
	}

	ssize.width *= cn;
	dsize.width *= cn;
	width0 *= cn;

	for( x = 0; x < dsize.width; x++ )
		tabM[x] = (x/cn)*2*cn + x % cn;

	u8* local_src_buff = (u8*)mv_malloc(_src->step);
	u8* local_dst_buff = (u8*)mv_malloc(_dst->step);
	
	for( int y = 0; y < dsize.height; y++ )
	{
		u8* dst = local_dst_buff;
		int *row0, *row1, *row2, *row3, *row4;
		
		// fill the ring buffer (horizontal convolution and decimation)
		for( ; sy <= y*2 + 2; sy++ )
		{
			int* row = buf + ((sy - sy0) % PD_SZ)*bufstep;
			int _sy = borderInterpolate(sy, ssize.height, 4);
			int limit = cn;
			const int* tab = tabL;
			u8* src = local_src_buff;

			//bring data in CMX
			dma_copy(src, _src->ptr + _src->step*_sy, _src->step);

			for( x = 0;;)
			{
				for( ; x < limit; x++ )
				{
					row[x] = src[tab[x+cn*2]]*6 + (src[tab[x+cn]] + src[tab[x+cn*3]])*4 +
						src[tab[x]] + src[tab[x+cn*4]];
				}

				if( x == dsize.width )
					break;

				for( ; x < width0; x++ ){
					row[x] = src[x*2]*6 + (src[x*2 - 1] + src[x*2 + 1])*4 + src[x*2 - 2] + src[x*2 + 2];
				}

				limit = dsize.width;
				tab = tabR - x;
			}
		}

		// do vertical convolution and decimation and write the result to the destination image
		for( k = 0; k < PD_SZ; k++ )
			rows[k] = buf + ((y*2 - PD_SZ/2 + k - sy0) % PD_SZ)*bufstep;
		row0 = rows[0]; row1 = rows[1]; row2 = rows[2]; row3 = rows[3]; row4 = rows[4];

		//reset index
		x = 0;

		for( ; x < dsize.width; x++ )
			dst[x] = castOp(row2[x]*6 + (row1[x] + row3[x])*4 + row0[x] + row4[x]);

		//throw data to DDR
		dma_copy(_dst->ptr + _dst->step * y, dst, _dst->step);
	}

	mv_free(local_dst_buff);
	mv_free(local_src_buff);

	mv_free(tabL);
	mv_free(tabR);
	mv_free(_buf);
	mv_free(_tabM);
}

inline void
	intersect( CvPoint2D32f pt, CvSize win_size, CvSize imgSize,
	CvPoint* min_pt, CvPoint* max_pt )
{
	CvPoint ipt;

	ipt.x = cvFloor( pt.x );
	ipt.y = cvFloor( pt.y );

	ipt.x -= win_size.width;
	ipt.y -= win_size.height;

	win_size.width = win_size.width * 2 + 1;
	win_size.height = win_size.height * 2 + 1;

	min_pt->x = MAX( 0, -ipt.x );
	min_pt->y = MAX( 0, -ipt.y );
	max_pt->x = MIN( win_size.width, imgSize.width - ipt.x );
	max_pt->y = MIN( win_size.height, imgSize.height - ipt.y );
}

void
	icvInitPyramidalAlgorithm(  u8 * imgA,  u8 * imgB,   
	int imgStep, CvSize imgSize,   
	u8 * pyrA, u8 * pyrB,   
	int level,   
	CvTermCriteria * criteria,   
	int flags,   
	u8 *** imgI, u8 *** imgJ,   
	int **step, CvSize** size,   
	float **scale, u8** buffer  )
{
	const int ALIGN = 8;
	int pyrBytes, bufferBytes = 0, elem_size;
	int level1 = level + 1;

	int i;
	CvSize  levelSize;

	*imgI = *imgJ = 0;
	*step = 0;
	*scale = 0;
	*size = 0;

	/* compare squared values */
	criteria->epsilon *= criteria->epsilon;

	/* set pointers and step for every level */
	pyrBytes = 0;

	elem_size = sizeof( imgA[0]);//CV_ELEM_SIZE(imgA->type);
	levelSize = imgSize;

	//pyramid level > 0
	for( i = 1; i < level1; i++ )
	{
		levelSize.width = (levelSize.width + 1) >> 1;
		levelSize.height = (levelSize.height + 1) >> 1;

		int tstep = cvAlign(levelSize.width,ALIGN) * elem_size;
		pyrBytes += tstep * levelSize.height;
	}

	/* buffer_size = <size for patches> + <size for pyramids> */

	bufferBytes = (level1 >= 0) * ((pyrA == 0) + (pyrB == 0)) * pyrBytes +   
		(sizeof( imgI[0][0] ) * 2 + sizeof( step[0][0] ) +   
		sizeof(size[0][0]) + sizeof( scale[0][0] )) * level1;   

	*imgI = (u8 **) (u8*)(*buffer);
	*imgJ = *imgI + level1;
	*step = (int *) (*imgJ + level1);
	*scale = (float *) (*step + level1);
	*size = (CvSize *)(*scale + level1);

	imgI[0][0] = (u8*)imgA;   
	imgJ[0][0] = (u8*)imgB;   //DIFFERENT
	step[0][0] = imgStep;   
	scale[0][0] = 1;   
	size[0][0] = imgSize;

	//pyramid level > 0
	if( level > 0 )
	{
		u8 *bufPtr = (u8 *) (*size + level1);   
		u8 *ptrA = pyrA;   
		u8 *ptrB = pyrB;  

		if( !ptrA )
		{
			ptrA = bufPtr;
			bufPtr += pyrBytes;
		}

		if( !ptrB )
			ptrB = bufPtr;

		levelSize = imgSize;

		/* build pyramids for both frames */
		for( i = 1; i <= level; i++ )
		{
			int levelBytes;
			CvMat prev_level, next_level;

			levelSize.width = (levelSize.width + 1) >> 1;
			levelSize.height = (levelSize.height + 1) >> 1;

			size[0][i] = levelSize;
			step[0][i] = cvAlign( levelSize.width, ALIGN ) * elem_size;
			scale[0][i] = scale[0][i - 1] * 0.5;

			levelBytes = step[0][i] * levelSize.height;

			imgI[0][i] = (u8 *) ptrA;

			ptrA += levelBytes;

			CvSize pl_size, nl_size;

			if( !(flags & CV_LKFLOW_PYR_A_READY) )
			{
				prev_level = cvMat( size[0][i-1].height, size[0][i-1].width, 0 );
				next_level = cvMat( size[0][i].height, size[0][i].width, 0 );

				pl_size = size[0][i-1];
				nl_size = size[0][i];

				cvSetData( &prev_level, imgI[0][i-1], step[0][i-1] );
				cvSetData( &next_level, imgI[0][i], step[0][i] );

				cvPyrDown( &prev_level, &next_level, size[0][i].height, pl_size, nl_size );
			}

			imgJ[0][i] = (u8 *) ptrB;
			ptrB += levelBytes;

			if( !(flags & CV_LKFLOW_PYR_B_READY) )
			{
				prev_level = cvMat( size[0][i-1].height, size[0][i-1].width, 0 );
				next_level = cvMat( size[0][i].height, size[0][i].width, 0 );

				pl_size = size[0][i-1];
				nl_size = size[0][i];
				
				cvSetData( &prev_level, imgJ[0][i-1], step[0][i-1] );
				cvSetData( &next_level, imgJ[0][i], step[0][i] );
				cvPyrDown( &prev_level, &next_level, size[0][i].height, pl_size, nl_size );
			}
		}
	}
}

/* compute dI/dx and dI/dy */
void
	icvCalcIxIy_32f( const float* src, int src_step, float* dstX, float* dstY, int dst_step,
	CvSize src_size, const float* smooth_k, float* buffer0 )
{
	int src_width = src_size.width, dst_width = src_size.width-2;
	int x, height = src_size.height - 2;
	float* buffer1 = buffer0 + src_width;

	src_step /= sizeof(src[0]);
	dst_step /= sizeof(dstX[0]);

	for( ; height--; src += src_step, dstX += dst_step, dstY += dst_step )
	{
		const float* src2 = src + src_step;
		const float* src3 = src + src_step*2;

		for( x = 0; x < src_width; x++ )
		{
			float t0 = (src3[x] + src[x])*smooth_k[0] + src2[x]*smooth_k[1];
			float t1 = src3[x] - src[x];
			buffer0[x] = t0; buffer1[x] = t1;
		}

		for( x = 0; x < dst_width; x++ )
		{
			float t0 = buffer0[x+2] - buffer0[x];
			float t1 = (buffer1[x] + buffer1[x+2])*smooth_k[0] + buffer1[x+1]*smooth_k[1];
			dstX[x] = t0; 
			dstY[x] = t1;
		}
	}
}

const void*
	icvAdjustRect( const void* srcptr, int src_step, int pix_size,
	CvSize src_size, CvSize win_size,
	CvPoint ip, CvRect* pRect )
{
	CvRect rect;
	const char* src = (const char*)srcptr;

	if( ip.x >= 0 )
	{
		src += ip.x*pix_size;
		rect.x = 0;
	}
	else
	{
		rect.x = -ip.x;
		if( rect.x > win_size.width )
			rect.x = win_size.width;
	}

	if( ip.x + win_size.width < src_size.width )
		rect.width = win_size.width;
	else
	{
		rect.width = src_size.width - ip.x - 1;
		if( rect.width < 0 )
		{
			src += rect.width*pix_size;
			rect.width = 0;
		}
	}

	if( ip.y >= 0 )
	{
		src += ip.y * src_step;
		rect.y = 0;
	}
	else
		rect.y = -ip.y;

	if( ip.y + win_size.height < src_size.height )
		rect.height = win_size.height;
	else
	{
		rect.height = src_size.height - ip.y - 1;
		if( rect.height < 0 )
		{
			src += rect.height*src_step;
			rect.height = 0;
		}
	}

	*pRect = rect;
	return src - rect.x*pix_size;
}

int icvGetRectSubPix_8u32f_C1R
	( const u8* src, int src_step, CvSize src_size,
	float* dst, int dst_step, CvSize win_size, CvPoint2D32f center )
{
	CvPoint ip;
	float  a12, a22, b1, b2;
	float a, b;
	float s = 0;
	int j,i;

	center.x -= (win_size.width-1)*0.5f;
	center.y -= (win_size.height-1)*0.5f;

	ip.x = cvFloor( center.x );
	ip.y = cvFloor( center.y );

	if( win_size.width <= 0 || win_size.height <= 0 )
		return CV_BADRANGE_ERR;

	a = center.x - ip.x;
	b = center.y - ip.y;
	a = MAX(a,0.0001f);
	a12 = a*(1.f-b);
	a22 = a*b;
	b1 = 1.f - b;
	b2 = b;
	s = (1. - a)/a;

	src_step /= sizeof(src[0]);
	dst_step /= sizeof(dst[0]);

	//Extract a patch from the big image using bilinear interpolation over the floating point coordinates.
	//A double ring buffer is used in order to avoid copying the same line twice (preserves the last line):
	//bring first line, bring next, do computation, cycle buffers, bring next line.

	u8 local_src_buff[15 * 2]; //Should be exactly (max(win_size.width) + 2 + 1) * 2
	u8* src0;
	u8* src1;

	src0 = local_src_buff;
	src1 = local_src_buff + win_size.width + 2;

	if( 0 <= ip.x && ip.x + win_size.width < src_size.width &&
		0 <= ip.y && ip.y + win_size.height < src_size.height )
	{
		//Extracted rectangle is totally inside the image
		src += ip.y * src_step + ip.x;
		
		//Bring first line
		dma_copy(src0, (u8*)src, win_size.width + 2);

		for( ; win_size.height--; dst += dst_step )
		{
			src += src_step;

			//Bring next line
			dma_copy(src1, (u8*)src, win_size.width + 2);
			
			float prev = (1 - a) * (b1 * src0[0] + b2 * src1[0]);
				
			for( j = 0; j < win_size.width; j++ )
			{
				float t = a12 * src0[j+1] + a22 * src1[j+1];
				dst[j] = prev + t;
				prev = (float)(t*s);
			}

			u8* tmp = src0;
			src0 = src1;
			src1 = tmp;
		}
	}
	//The corner coordinates should take into account the restriction above
	else
	{
		CvRect r;

		src = (const u8*)icvAdjustRect( src, src_step*sizeof(*src),
			sizeof(*src), src_size, win_size,ip, &r);

		dma_copy(src0, (u8*)src, win_size.width + 2);

		for( i = 0; i < win_size.height; i++, dst += dst_step )
		{
			const u8 *src2 = src + src_step;

			if( i < r.y || i >= r.height )
				src2 -= src_step;

			dma_copy(src1, (u8*)src2, win_size.width + 2);

			for( j = 0; j < r.x; j++ )
			{
				//float s0 = (src[r.x])*b1 + (src2[r.x])*b2;
				float s0 = src0[r.x] * b1 + src1[r.x] * b2;

				dst[j] = (float)(s0);
			}

			if( j < r.width )
			{
				float prev = (1 - a)*(b1*(src[j]) + b2*(src2[j]));

				for( ; j < r.width; j++ )
				{
					//float t = a12*(src[j+1]) + a22*(src2[j+1]);
					float t = a12 * src0[j+1] + a22 * src1[j+1];
					dst[j] = prev + t;
					prev = (float)(t*s);
				}
			}

			for( ; j < win_size.width; j++ )
			{
				//float s0 = (src[r.width])*b1 + (src2[r.width])*b2;
				float s0 = src0[r.width] * b1 + src1[r.width] * b2;

				dst[j] = (float)(s0);
			}

			if( i < r.height )
				src = src2;

			u8* tmp = src0;
			src0 = src1;
			src1 = tmp;
		}
	}

	return CV_OK;
}

int icvCalcOpticalFlowPyrLK_8uC1R(u8 * imgA,   
	u8* imgB,   
	u32 imgStep,   
	CvSize imgSize,   
	u8* pyrA,   
	u8* pyrB,   
	CvPoint2D32f* featuresA,   
	CvPoint2D32f* featuresB,   
	u32 count,   
	CvSize winSize,   
	u32 level,   
	u8 *status,   
	fp32 *error,
	CvTermCriteria criteria, 
	u32 flags )   
{   
	int l;

	static MVCV_STATIC_DATA const float smoothKernel[] = { 0.09375, 0.3125, 0.09375 };  // 3/32, 10/32, 3/32
	//static const float smoothKernel[] = {3.0, 10.0, 3.0};

	u8 **imgI = 0;   
	u8 **imgJ = 0;   
	int *step = 0;   
	float *scale = 0;   
	CvSize* size = 0;   

	int i;   

	/* check input arguments */   
	if( !featuresA || !featuresB )   
		return CV_NULLPTR_ERR;   
	if( winSize.width == 1 || winSize.height == 1 )   
		return CV_BADSIZE_ERR;   

	if( (flags & ~0xF) != 0 )   
		return CV_BADFLAG_ERR;   
	if( count == 0 )   
		return CV_BADRANGE_ERR;   

	u8* pyr_buffer = (u8*)mv_malloc(DIM*sizeof(u8));

	icvInitPyramidalAlgorithm( imgA, imgB, imgStep, imgSize,   
		pyrA, pyrB, level, &criteria, flags,   
		&imgI, &imgJ, &step, &size, &scale, &pyr_buffer);

	//Initialize status
	for(i = 0; i< count; i++)
	{
		status[i] = 1;
		error[i] = 0;
	}

	if( !(flags & CV_LKFLOW_INITIAL_GUESSES) )
		for (i = 0; i< count; i++)
		{
			featuresB[i].x = featuresA[i].x + 5;
			featuresB[i].y = featuresA[i].y + 5;
		}

	//Scale coordinates
	for( i = 0; i < count; i++ )
	{
		featuresB[i].x = (float)(featuresB[i].x * scale[level] * 0.5);
		featuresB[i].y = (float)(featuresB[i].y * scale[level] * 0.5);
	}

	/* do processing from top pyramid level (smallest image)  
	to the bottom (original image) */   
	for( l = level; l >= 0; l-- )   
	{
		CvSize patchSize = cvSize( winSize.width * 2 + 1, winSize.height * 2 + 1 );
		int patchLen = patchSize.width * patchSize.height;
		int srcPatchLen = (patchSize.width + 2)*(patchSize.height + 2);

		float *buf = (float*)mv_malloc((patchLen*3 + srcPatchLen) * sizeof(float));  
		float* patchI = buf;
		float* patchJ = patchI + srcPatchLen;
		float* Ix = patchJ + patchLen;
		float* Iy = Ix + patchLen;
		float scaleL = 1.f/(1 << l);
		CvSize levelSize = size[l];

		//float smoothKernel[] = { 3.0 / (1 << l), 10.0 / (1 << l), 3.0 / (1 << l) };

		// find flow for each given point
		for( i = 0; i < count; i++ )
		{
			CvPoint2D32f v;
			CvPoint minI, maxI, minJ, maxJ;
			CvSize isz, jsz;
			int pt_status;
			CvPoint2D32f u;
			CvPoint prev_minJ = cvPoint(-1, -1);
			CvPoint prev_maxJ = cvPoint(-1, -1);
			float Gxx = 0, Gxy = 0, Gyy = 0, D = 0, minEig = 0;
			float prev_mx = 0, prev_my = 0;
			int j, x, y;

			v.x = featuresB[i].x*2;
			v.y = featuresB[i].y*2;

			pt_status = status[i];

			minI = maxI = minJ = maxJ = cvPoint(0, 0);

			u.x = featuresA[i].x * scaleL;
			u.y = featuresA[i].y * scaleL;

			intersect( u, winSize, levelSize, &minI, &maxI );
			isz = jsz = cvSize(maxI.x - minI.x + 2, maxI.y - minI.y + 2);
			u.x += (minI.x - (patchSize.width - maxI.x + 1))*0.5f;
			u.y += (minI.y - (patchSize.height - maxI.y + 1))*0.5f;

			if( isz.width < 3 || isz.height < 3 ||
				icvGetRectSubPix_8u32f_C1R( imgI[l], step[l], levelSize,   
				patchI, isz.width*sizeof(patchI[0]), isz, u ) < 0 )
			{
				// point is outside the first image. take the next
				status[i] = 0;
				continue;
			}

			// Vectorize
			icvCalcIxIy_32f( patchI, isz.width*sizeof(patchI[0]), Ix, Iy,
				(isz.width-2)*sizeof(patchI[0]), isz, smoothKernel, patchJ );

			for( j = 0; j < criteria.max_iter; j++ )
			{
				float bx = 0, by = 0;
				float mx, my;
				CvPoint2D32f _v;

				intersect( v, winSize, levelSize, &minJ, &maxJ );

				minJ.x = MAX( minJ.x, minI.x );
				minJ.y = MAX( minJ.y, minI.y );

				maxJ.x = MIN( maxJ.x, maxI.x );
				maxJ.y = MIN( maxJ.y, maxI.y );

				jsz = cvSize(maxJ.x - minJ.x, maxJ.y - minJ.y);

				_v.x = v.x + (minJ.x - (patchSize.width - maxJ.x + 1))*0.5f;
				_v.y = v.y + (minJ.y - (patchSize.height - maxJ.y + 1))*0.5f;

				// Vectorize
				if( jsz.width < 1 || jsz.height < 1 ||
					icvGetRectSubPix_8u32f_C1R( imgJ[l], step[l], levelSize,   
					patchJ, jsz.width*sizeof(patchJ[0]), jsz, _v  ) < 0 )
				{
					// point is outside of the second image. take the next
					pt_status = 0;
					break;
				}

				// Check if the candidate point has moved more than 1 pixel
				// after last iteration's guess. If it did, we need to recompute
				// the gradients matrix.
				if( maxJ.x == prev_maxJ.x && maxJ.y == prev_maxJ.y &&
					minJ.x == prev_minJ.x && minJ.y == prev_minJ.y )
				{
					for( y = 0; y < jsz.height; y++ )
					{
						const float* pi = patchI +
							(y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
						const float* pj = patchJ + y*jsz.width;
						const float* ix = Ix +
							(y + minJ.y - minI.y)*(isz.width-2) + minJ.x - minI.x;
						const float* iy = Iy + (ix - Ix);

						// Vectorize
						for( x = 0; x < jsz.width; x++ )
						{
							float t0 = pi[x] - pj[x];
							bx += t0 * ix[x];
							by += t0 * iy[x];
						}
					}
				}
				else
				{
					Gxx = Gyy = Gxy = 0;
					for( y = 0; y < jsz.height; y++ )
					{
						const float* pi = patchI +
							(y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
						const float* pj = patchJ + y*jsz.width;
						const float* ix = Ix +
							(y + minJ.y - minI.y)*(isz.width-2) + minJ.x - minI.x;
						const float* iy = Iy + (ix - Ix);

						// Vectorize
						for( x = 0; x < jsz.width; x++ )
						{
							float t = pi[x] - pj[x];
							bx += (float) (t * ix[x]);
							by += (float) (t * iy[x]);

							// Compute the gradients matrix (eq. 24)
							Gxx += ix[x] * ix[x];
							Gxy += ix[x] * iy[x];
							Gyy += iy[x] * iy[x];
						}
					}

					// Determinant of gradients matrix must not be 0 so the
					// matrix in (eq. 28) is invertible
					D = Gxx * Gyy - Gxy * Gxy;
					if( D < DBL_EPSILON )
					{
						pt_status = 0;
						break;
					}

					// The minimum eigen value is a measure of cornerness:
					// the greater the value, the stronger the corner
					if( flags & CV_LKFLOW_GET_MIN_EIGENVALS )
						minEig = (Gyy + Gxx - sqrt((Gxx-Gyy)*(Gxx-Gyy) + 4.*Gxy*Gxy))/(2*jsz.height*jsz.width);

					D = 1. / D;

					prev_minJ = minJ;
					prev_maxJ = maxJ;
				}
				//end if

				// Compute the optical flow vector by solving (eq. 25)
				mx = (float) ((Gyy * bx - Gxy * by) * D);
				my = (float) ((Gxx * by - Gxy * bx) * D);

				// Make the guess for the next iteration
				v.x += mx;
				v.y += my;
				
				if( mx * mx + my * my < criteria.epsilon )
					break;

				// If the point moved too little, we found it
				if( j > 0 && fabs(mx + prev_mx) < 0.01 && fabs(my + prev_my) < 0.01 )
				{
					v.x -= mx*0.5f;
					v.y -= my*0.5f;
					break;
				}

				// Hold the last motion vector
				prev_mx = mx;
				prev_my = my;
			}

			// If the point slips off the image, its lost
			if (v.x < 0 || v.y < 0)
				pt_status = 0;

			featuresB[i] = v;
			status[i] = (char)pt_status;
			 
			// Calculate error
			if( l == 0 && error && pt_status )
			{				
				float err = 0;

				// Hold minimum eigen value as an error measurement
				if( flags & CV_LKFLOW_GET_MIN_EIGENVALS )
					err = minEig;
				else
				{
					// Compute SSD as an error measurement
					for( y = 0; y < jsz.height; y++ )
					{
						const float* pi = patchI +
							(y + minJ.y - minI.y + 1)*isz.width + minJ.x - minI.x + 1;
						const float* pj = patchJ + y*jsz.width;

						for( x = 0; x < jsz.width; x++ )
						{
							float t = pi[x] - pj[x];
							err += t * t;
						}
					}
					err = sqrt(err);
				}
				error[i] = (float)err;
			}
		} // end of point processing loop (i)

		mv_free(buf);
	}

	mv_free(pyr_buffer);

	return CV_OK;
}

s32 calcOpticalFlowPyrLK(u8* imgA, u8* imgB, CvSize size, u8* pyrA, u8* pyrB, CvPoint2D32f* featuresA, CvPoint2D32f* featuresB,   
	u8 *status, fp32 *error, CvSize winSize, u32 level, CvTermCriteria criteria, u32 flags, u32 no_points )
{
	CvSize maxPatchSize;
	//maxPatchSize = cvSize(winSize.width * (2 + 1), winSize.height * (2 + 1));
	maxPatchSize.width = winSize.width * (2 + 1);
	maxPatchSize.height = winSize.width * (2 + 1);
	
	return icvCalcOpticalFlowPyrLK_8uC1R(imgA, imgB, size.width, size, pyrA, pyrB, featuresA, featuresB,  
		no_points, winSize, level, status, error, criteria, flags);
}

}