#include <calcOpticalFlow.h>

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <mvcv_types.h>
#include <mvcv_internal.h>
#include <mat.h>
#include <samplers.h>
#include <kernels.h>
#include <alloc.h>
#include <mvstl_utils.h>

namespace mvcv
{

//#define PROFILING

// Statistics
static u32 copyCnt = 0;
static u32 bilInterpFullCnt = 0;
static u32 bilInterpPadCnt = 0;
static u32 calcGCnt = 0;
static u32 calcBxByCnt = 0;
static u32 calcBxBy11x11cnt = 0;
static u32 scharrCnt = 0;
static u32 scharr13x13Cnt = 0;

#ifdef __MOVICOMPILE__

#define CYCLE_CNT_START	(*(int*)0x80020090 = 0) // set TimFreeCount to 0 to measure cycle count of one iteration
#define CYCLE_CNT_STOP  (*(int*)0x80020090)

#else

#define CYCLE_CNT_START 0
#define CYCLE_CNT_STOP 0

#endif

typedef enum
{
	NoProf = 0,
	StartOfLoopProf,
	BilFilt1Prof,
	BetweenBilFilt1ScharrProf,
	ScharrProf,
	BetweenScharrBilFilt2Prof,
	BilFilt2Prof,
	BetweenBilFilt2CalcBxByProf,
	CalcBxByProf,
	BetweenCalcBxByCalcGProf,
	CalcGProf,
	EndOfLoopProf,
	NrSteps
} ProfilingStep;

typedef struct
{
	ProfilingStep	step;
	u32				level;
	u32				iteration;
	u32				cycles;
} ProfilingUnit;

typedef struct
{
	ProfilingStep	step;
	u32				calls;
	u32				cycles;
	u32				dummyStructPadding;
} ProfilingSummary;

typedef struct
{
	ProfilingSummary	profilingSummary[NrSteps];
	ProfilingUnit		profilingList[5 * 9 * NrSteps]; // levels * iterations * steps
} ProfilingPoint;

ProfilingUnit		profilingList[5 * 1 * 9 * 5];
ProfilingSummary	profilingSummary[NrSteps];
u32 profilingListCnt = 0;

static inline void Profile(ProfilingStep step, u32 level, u32 iteration, u32 cycles)
{
	//Read current timestamp and subtract from previous
	profilingList[profilingListCnt].cycles = cycles;
	profilingList[profilingListCnt].step = step;
	profilingList[profilingListCnt].level = level;
	profilingList[profilingListCnt].iteration = iteration;
	profilingListCnt++;
}

static inline void ProfileSummary()
{
	for (u32 i = 0; i < profilingListCnt; i++)
	{
		ProfilingStep step = profilingList[i].step;

		profilingSummary[step].step = step;
		profilingSummary[step].calls++;
		profilingSummary[step].cycles += profilingList[i].cycles;
		profilingSummary[0].cycles += profilingList[i].cycles;
	}
}

#ifdef PROFILING
#define PROF(fcn, step, level, iteration) \
		CYCLE_CNT_START; \
		fcn; \
		Profile(step, level, iteration, CYCLE_CNT_STOP);
#else
#define PROF(fcn, step, level, iteration) fcn;
#endif

void mvcvPyrDown(Mat* src, Mat* dst)
{
	const int PD_SZ = 5;
	int cn = 1;
	int bufstep = (int)alignSize(dst->cols*cn, 16);
	int* _buf = (int*)mv_malloc((bufstep*PD_SZ + 16) * sizeof(int));
	int* buf = alignPtr((int*)_buf, 16);
	int* tabL = (int*)mv_malloc((PD_SZ + 2) * sizeof(int));
	int* tabR = (int*)mv_malloc((PD_SZ + 2) * sizeof(int));
	//int tabL[512 * (PD_SZ + 2)];
	//int tabR[512 * (PD_SZ + 2)];
	int *_tabM = (int*)mv_malloc((dst->cols*cn) * sizeof(int));
	int* tabM = _tabM;
	int* rows[PD_SZ];

	svu_assert(src->rows > 0 && src->rows < 2048);
	svu_assert(src->cols > 0 && src->cols < 2048);
	svu_assert(dst->rows > 0 && dst->rows < 2048);
	svu_assert(dst->cols > 0 && dst->cols < 2048);
	svu_assert(tabL != null && tabR != null && _tabM != null && _buf != null);

	int k;
	int x;
	int sy0 = -PD_SZ / 2;
	int sy = sy0;
	int width0;
	width0 = MIN((src->cols - PD_SZ / 2 - 1) / 2 + 1, dst->cols);

	for (x = 0; x <= PD_SZ + 1; x++)
	{
		int sx0 = borderInterpolate(x - PD_SZ / 2, src->cols, 4) * cn;
		int sx1 = borderInterpolate(x + width0 * 2 - PD_SZ / 2, src->cols, 4) * cn;
		for( k = 0; k < cn; k++ )
		{
			tabL[x*cn + k] = sx0 + k;
			tabR[x*cn + k] = sx1 + k;
		}
	}

	CvSize ssize((src->cols - 1) * cn, src->rows - 1);
	CvSize dsize((dst->cols - 1) * cn, dst->rows - 1);
	width0 *= cn;

	for (x = 0; x < dsize.width; x++)
		tabM[x] = (x / cn) * 2 * cn + x % cn;

	u8* local_src_buff = (u8*)mv_malloc(ssize.width + 1);
	u8* local_dst_buff = (u8*)mv_malloc(dsize.width + 1);

	for (int y = 0; y < dsize.height; y++)
	{
		u8* dst_buf = local_dst_buff;
		int *row0, *row1, *row2, *row3, *row4;

		// fill the ring buffer (horizontal convolution and decimation)
		for (; sy <= y * 2 + 2; sy++)
		{
			int* row = buf + ((sy - sy0) % PD_SZ) * bufstep;
			int _sy = borderInterpolate(sy, ssize.height, 4);
			int limit = cn;
			const int* tab = tabL;
			u8* src_buf = local_src_buff;

			//bring data in CMX
			smartCopy(src_buf, src->ptr(_sy), src->step);

			for (x = 0; ;)
			{
				for (; x < limit; x++)
				{
					row[x] = src_buf[tab[x + cn * 2]] * 6 + (src_buf[tab[x + cn]] + src_buf[tab[x + cn * 3]]) * 4 +
						src_buf[tab[x]] + src_buf[tab[x + cn * 4]];
				}

				if (x == dsize.width)
					break;

				for (; x < width0; x++)
				{
					row[x] = src_buf[x * 2] * 6 + (src_buf[x * 2 - 1] + src_buf[x * 2 + 1]) * 4 + src_buf[x * 2 - 2] + src_buf[x * 2 + 2];
				}

				limit = dsize.width;
				tab = tabR - x;
			}
		}

		// do vertical convolution and decimation and write the result to the destination image
		for (k = 0; k < PD_SZ; k++)
			rows[k] = buf + ((y * 2 - PD_SZ / 2 + k - sy0) % PD_SZ) * bufstep;
		row0 = rows[0]; row1 = rows[1]; row2 = rows[2]; row3 = rows[3]; row4 = rows[4];

		//reset index
		x = 0;

		for (; x < dsize.width; x++)
			dst_buf[x] = castOp(row2[x]*6 + (row1[x] + row3[x])*4 + row0[x] + row4[x]);
		dst_buf[dsize.width] = 0;

		//throw data to DDR
		if (IS_DDR(dst->ptr(y)))
			smartCopy(dst->ptr(y), dst_buf, dsize.width + 1);
		else
			memcpy(dst->ptr(y), dst_buf, dsize.width + 1);
	}

	memset(local_dst_buff, 0, dsize.width + 1);
	if (IS_DDR(dst->ptr(dsize.height)))
		smartCopy(dst->ptr(dsize.height), local_src_buff, dsize.width + 1);
	else
		memcpy(dst->ptr(dsize.height), local_src_buff, dsize.width + 1);

	mv_free(local_dst_buff);
	mv_free(local_src_buff);

	mv_free(tabL);
	mv_free(tabR);
	mv_free(_buf);
	mv_free(_tabM);
}

void mvcvBuildPyramid(Mat pyr[], int levels, u8* const pyrBufs[])
{
	// Build pyramid
	for (int i = 1; i < levels; i++)
	{
		cvSetData(&pyr[i], pyr[i - 1].rows / 2 + 1, pyr[i - 1].cols / 2 + 1, pyrBufs[i], pyr[i].type);
		if (pyr[i].data)
		{
			mvcvPyrDown(&pyr[i - 1], &pyr[i]);
		}
	}
}

static inline void
	intersect_tuned( CvPoint2D32fW pt, ClSizeW win_size, ClSizeW imgSize,
	CvPointW* min_pt, CvPointW* max_pt )
{
	CvPointW ipt;

	ipt.x = cvFloor( pt.x );
	ipt.y = cvFloor( pt.y );

	ipt.x -= win_size.x;
	ipt.y -= win_size.y;

	win_size.x = win_size.x * 2 + 1;
	win_size.y = win_size.y * 2 + 1;

	min_pt->x = MAX( 0, -ipt.x );
	min_pt->y = MAX( 0, -ipt.y );
	max_pt->x = MIN( win_size.x, imgSize.x - ipt.x );
	max_pt->y = MIN( win_size.y, imgSize.y - ipt.y );
}

#define CalcG CalcG_asm
#define CalcBxBy CalcBxBy_asm

s32 mvcvCalcOpticalFlowPyrLK_tuned(Mat pyrA[], Mat pyrB[], CvPoint2D32f* featuresA, CvPoint2D32f* featuresB,
	u8 *status, fp32 *error, CvSize _winSize, u32 levels, CvTermCriteria criteria, u32 flags, u32 count)
{
	// Normalized Scharr kernel
	static MVCV_STATIC_DATA const float smoothKernel[] = { 0.09375, 0.3125, 0.09375 };  // 3/32, 10/32, 3/32
	static MVCV_STATIC_DATA const u32 xx		= 0;
	static MVCV_STATIC_DATA const u32 yy		= 1;
	static MVCV_STATIC_DATA const u32 xy		= 2;
	static MVCV_STATIC_DATA const u32 D			= 3;

	ClSizeW winSize = clSizeW(_winSize.width, _winSize.height);

	s32 l;
	u32 i;
	u32 level = levels - 1;

	//Some auxiliary variables we'll need
	ClSize auxclsize1,auxclsize2;
	CvPoint auxcvpoint1,auxcvpoint2;

	CvSize maxPatchSize;
	//maxPatchSize = cvSize(winSize.width * (2 + 1), winSize.height * (2 + 1));
	maxPatchSize.width = winSize.x * (2 + 1);
	maxPatchSize.height = winSize.y * (2 + 1);

	// Check input arguments
	if (!featuresA || !featuresB)
		return CV_NULLPTR_ERR;

	if (winSize.x == 1 || winSize.y == 1)
		return CV_BADSIZE_ERR;

	if ((flags & ~0xF) != 0)
		return CV_BADFLAG_ERR;

	if (count == 0)
		return CV_BADRANGE_ERR;

	// Initialize status
	for (i = 0; i < count; i++)
	{
		status[i] = 1;
		error[i] = 0;
	}

	// Initialize guesses
	if( !(flags & CV_LKFLOW_INITIAL_GUESSES) )
		for (i = 0; i < count; i++)
		{
			featuresB[i].x = featuresA[i].x;
			featuresB[i].y = featuresA[i].y;
		}

	// Scale coordinates to the highest pyramid level
	for (i = 0; i < count; i++)
	{
		const float scale = 1.f / (1 << level) * 0.5f;
		featuresB[i].x = featuresB[i].x * scale;
		featuresB[i].y = featuresB[i].y * scale;
	}

	CvSize patchSize = cvSize( winSize.x * 2 + 1, winSize.y * 2 + 1 );
	int patchLen = patchSize.width * patchSize.height;
	int srcPatchLen = (patchSize.width + 2) * (patchSize.height + 2);

	u8* patchI		= (u8*)mv_malloc(srcPatchLen * sizeof(patchI[0]));
	u8* patchJ		= (u8*)mv_malloc(patchLen * sizeof(patchJ[0]));
	float* Ix		= (float*)mv_malloc(patchLen * sizeof(Ix[0]));
	float* Iy		= (float*)mv_malloc(patchLen * sizeof(Iy[0]));
	float* convBuff	= (float*)mv_malloc((patchSize.width + 2) * 2 * sizeof(convBuff[0]));

	// Do processing from top pyramid level (smallest image)
	// to the bottom (original image)
	for (l = level; l >= 0; l--)
	{
		float scaleL = 1.f / (1 << l);
		ClSizeW levelSize = clSizeW(pyrA[l].cols, pyrA[l].rows);

		// Find flow for each given point
		for (i = 0; i < count; i++)
		{
			CvPoint2D32fW v;
			CvPointW minI, maxI, minJ, maxJ;
			ClSizeW isz, jsz;
			int pt_status;
			CvPoint2D32fW u;
			CvPointW prev_minJ = cvPointW(-1, -1);
			CvPointW prev_maxJ = cvPointW(-1, -1);
			float prev_mx = 0;
			float prev_my = 0;
			int j;

			PROF(
			v.x = featuresB[i].x * 2;
			v.y = featuresB[i].y * 2;

			pt_status = status[i];

			minI = maxI = minJ = maxJ = cvPointW(0, 0);

			u.x = featuresA[i].x * scaleL;
			u.y = featuresA[i].y * scaleL;

			intersect_tuned( u, winSize, levelSize, &minI, &maxI );
			isz = jsz = clSizeW(maxI.x - minI.x + 2, maxI.y - minI.y + 2);

			u.x += (minI.x - (patchSize.width - maxI.x + 1)) * 0.5f;
			u.y += (minI.y - (patchSize.height - maxI.y + 1)) * 0.5f;
			, StartOfLoopProf, l, 0);

			s32 sampleRes;

			PROF(sampleRes = icvGetRectSubPix_8u32f_C1R_tuned(pyrA[l].data, pyrA[l].step, levelSize,
				patchI, isz.x*sizeof(patchI[0]), isz, u, !IS_DDR(pyrA[l].data)), BilFilt1Prof, l, 0);

			PROF(
			if (isz.x < 3 || isz.y < 3 || sampleRes < 0)
			{
				// Point is outside the first image. take the next
				status[i] = 0;
				//error[i] = 0;
				error[i] = 9999999.9f;
				continue;
			},
			BetweenBilFilt1ScharrProf, l, 0);

			// Vectorize
			PROF(conv3x3fp32( patchI, isz.x*sizeof(patchI[0]), Ix, Iy,
				(isz.x-2)*sizeof(Ix[0]), isz, smoothKernel, convBuff), ScharrProf, l, 0);

			// does not have same effect as clal inside..?
			//for (j=0; j<isz.x; j++)
			//	convBuff[i] = 0;
			//printf("%d 2 (%0.2f, %0.2f)\n",i,featuresB[i].x,featuresB[i].y);

			Scalar G = cvScalar(0.0, 0.0, 0.0, 0.0);

			for( j = 0; j < criteria.max_iter; j++ )
			{
				float mx, my;
				CvPoint2D32fW _v;
				PROF(
				intersect_tuned( v, winSize, levelSize, &minJ, &maxJ );

				minJ.x = MAX( minJ.x, minI.x );
				minJ.y = MAX( minJ.y, minI.y );

				maxJ.x = MIN( maxJ.x, maxI.x );
				maxJ.y = MIN( maxJ.y, maxI.y );

				jsz = clSizeW(maxJ.x - minJ.x, maxJ.y - minJ.y);

				_v.x = v.x + (minJ.x - (patchSize.width - maxJ.x + 1)) * 0.5f;
				_v.y = v.y + (minJ.y - (patchSize.height - maxJ.y + 1)) * 0.5f;
				, BetweenScharrBilFilt2Prof, l, j);
				s32 sampleRes;
;
				PROF(sampleRes = icvGetRectSubPix_8u32f_C1R_tuned(pyrB[l].data, pyrB[l].step, levelSize,
					patchJ, jsz.x*sizeof(patchJ[0]), jsz, _v, !IS_DDR(pyrA[l].data)), BilFilt2Prof, l, j);

				PROF(
				// Vectorize
				if (jsz.x < 1 || jsz.y < 1 || sampleRes < 0)
				{
					// point is outside of the second image. take the next
					pt_status = 0;
					//error[i] = 0
					error[i] = 9999999.9f;
					break;
				},
				BetweenBilFilt2CalcBxByProf, l, j);

				// Compute mismatch vector
				CvPoint2D32f b;
				auxclsize1.x=isz.x;auxclsize1.y=isz.y;
				auxclsize2.x=jsz.x;auxclsize2.y=jsz.y;
				auxcvpoint1.x=minI.x;auxcvpoint1.y=minI.y;
				auxcvpoint2.x=minJ.x;auxcvpoint2.y=minJ.y;
				//PROF(b = CalcBxBy(patchI, isz, Ix, Iy, minI, patchJ, jsz, minJ, error[i]), CalcBxByProf, l, j);
				PROF(b = CalcBxBy(patchI, auxclsize1, Ix, Iy, auxcvpoint1, patchJ, auxclsize2, auxcvpoint2, error[i]), CalcBxByProf, l, j);

				PROF(
				if (l == 0)

					error[i] = sqrt(error[i]);

				// Check if the candidate point has moved more than 1 pixel
				// after last iteration's guess. If it did, we need to recompute
				// the gradients matrix.
				if( maxJ.x != prev_maxJ.x || maxJ.y != prev_maxJ.y ||
					minJ.x != prev_minJ.x || minJ.y != prev_minJ.y )
				{
					, BetweenCalcBxByCalcGProf, l, j);

					// Compute gradients matrix and its determinant (eq. 24)
					//G = CalcG(Ix, isz, minI, Iy, jsz, minJ);
					//PROF(CalcG(Ix, isz, minI, Iy, jsz, minJ, G), CalcGProf, l, j);

				auxclsize1.x=isz.x;auxclsize1.y=isz.y;
				auxclsize2.x=jsz.x;auxclsize2.y=jsz.y;
				auxcvpoint1.x=minI.x;auxcvpoint1.y=minI.y;
				auxcvpoint2.x=minJ.x;auxcvpoint2.y=minJ.y;

				PROF(CalcG(Ix, auxclsize1, auxcvpoint1, Iy, auxclsize2, auxcvpoint2, G), CalcGProf, l, j);

					PROF(
					// If determinant is too close to 0, the matrix is not
					// invertible, therefore the motion vector can't be calculated
					if( G[D] < DBL_EPSILON )
					{

						pt_status = 0;
						error[i] = 9999999.9f;
						break;
					}

					G[D] = 1.f / G[D];

					prev_minJ = minJ;
					prev_maxJ = maxJ;
				}
				//end if

				// Compute the optical flow vector by solving (eq. 25)
				mx = (float) ((G[yy] * b.x - G[xy] * b.y) * G[D]);
				my = (float) ((G[xx] * b.y - G[xy] * b.x) * G[D]);

				// Make the guess for the next iteration
				v.x += mx;
				v.y += my;

				if( mx * mx + my * my < criteria.epsilon )
					break;

				// If the point moved too little, we found it
				if( j > 0 && fabs(mx + prev_mx) < 0.01 && fabs(my + prev_my) < 0.01 )
				{
					v.x -= mx * 0.5f;
					v.y -= my * 0.5f;
					break;
				}

				// Hold the last motion vector
				prev_mx = mx;
				prev_my = my;
			}

			// If the point slips off the image, its lost
			if (v.x < 0 || v.y < 0 || v.x > levelSize.x || v.y > levelSize.y - 8)
			{
				pt_status = 0;
				error[i] = 9999999.9f;
			}

			featuresB[i].x = v.x;
			featuresB[i].y = v.y;

			status[i] = (char)pt_status;

			, EndOfLoopProf, l, j);
		} // end of point processing loop (i)
		//printf("%d - (%0.2f, %0.2f)\n",i,featuresB[i].x,featuresB[i].y);
	}

	mv_free(patchI);
	mv_free(patchJ);
	mv_free(Ix);
	mv_free(Iy);
	mv_free(convBuff);

	//printf("minx %d, miny %d\n", minx, miny);

#ifdef PROFILING
	ProfileSummary();
	printf("Profile summary:\n");
	printf("Step  Calls  Cycles \n");
	for (u32 i = 0; i < NrSteps; i++)
		printf("% 5d % 5d % 7d\n", profilingSummary[i].step, profilingSummary[i].calls, profilingSummary[i].cycles);
#endif

	return CV_OK;
}

}
