#ifndef __MVCV_INTERNAL__
#define __MVCV_INTERNAL__

#include "mvcv_types.h"
#include "calcOpticalFlow.h"

namespace mvcv
{

size_t alignSize(size_t sz, int n);
int* alignPtr(int* ptr, int n);
int borderInterpolate(int p, int len, int borderType);
void cvPyrDown(CvMat* src, CvMat* _dst, int _dsz, CvSize _src_size, CvSize _dst_size);
void intersect(CvPoint2D32f pt, CvSize win_size, CvSize imgSize, CvPoint* min_pt, CvPoint* max_pt);
void icvInitPyramidalAlgorithm(u8* imgA,  u8* imgB, int imgStep, CvSize imgSize, u8* pyrA, u8* pyrB, 
	int level, CvTermCriteria* criteria, int flags, u8*** imgI, u8*** imgJ, int** step, CvSize** size,   
	float** scale, u8** buffer);
const void*	icvAdjustRect(const void* srcptr, int src_step, int pix_size, CvSize src_size, CvSize win_size,
	CvPoint ip, CvRect* pRect);
int icvCalcOpticalFlowPyrLK_8uC1R(u8* imgA, u8* imgB, u32 imgStep, CvSize imgSize, u8* pyrA, u8* pyrB,   
	CvPoint2D32f* featuresA, CvPoint2D32f* featuresB, u32 count, CvSize winSize, u32 level, u8* status,   
	fp32* error, CvTermCriteria criteria, u32 flags);
s32 calcOpticalFlowPyrLK(u8* imgA, u8* imgB, CvSize size, u8* pyrA, u8* pyrB, CvPoint2D32f* featuresA, CvPoint2D32f* featuresB,   
	u8* status, fp32* error, CvSize winSize, u32 level, CvTermCriteria criteria, u32 flags, u32 no_points);

}

#endif
