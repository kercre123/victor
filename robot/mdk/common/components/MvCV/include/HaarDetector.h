///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief Interface for the Haar Detector algorithm used for object detection.
///
#ifndef __HAAR_FEATURES_H__
#define __HAAR_FEATURES_H__

//---------------------------------------------------------------------------//
//------------ Included types first then APIs from other modules ------------//
//---------------------------------------------------------------------------//

// VS and Win
#ifndef __LEON__
#include <mvcv.h>
#include <mvstl.h>
#endif

#include <HaarCascade.h>

//---------------------------------------------------------------------------//
//-------- Exported variables --- these should be avoided at all costs ------//
//---------------------------------------------------------------------------//

// Maximum dimensions for rectangle containers
#define MAX_CANDIDATES             80 // maximum number of rectangles found for all scales
#define MAX_CLASSES                10 // maximum number of rectangle classes - used  
#define FLT_MIN          1.175494350822287507969e-38f

#define MVCV_NODES_FRONTALFACE_MAX  2
#define MVCV_INTER_LINEAR           1
#define MVCV_MAX_STAGES            20

//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//
//------------------------------ Specific Structs/Types -------------------------------//
//-------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------//
typedef int sumtype;
typedef float sqsumtype;
typedef unsigned char uchar;

typedef struct Point
{
	s32 x, y;
} Point_t;

static inline int min(int a, int b)
{
	if (a < b)
		return a;
	return b;
}

static  inline int max(int a, int b)
{
	if (a > b)
		return a;
	return b;
}

#ifndef __LEON__

typedef struct HidHaarTreeNode
{
    sumtype *p[MVCV_HAAR_FEATURE_MAX][4];
    float   weight[MVCV_HAAR_FEATURE_MAX];
    float   threshold;
    float left;
    float right;
}
HidHaarTreeNode;

typedef struct HidHaarClassifier
{
    HidHaarTreeNode node[2];
}
HidHaarClassifier;

typedef struct HidHaarStageClassifier
{
    HidHaarClassifier *classifier;
    float             threshold;
    uint8_t           count;
}
HidHaarStageClassifier;

struct HidHaarClassifierCascade
{
    HidHaarStageClassifier stage_classifier[MVCV_MAX_STAGES];
    float                  inv_window_area;
    mvcv::Mat              sum, sqsum;
    sqsumtype              *pq0, *pq1, *pq2, *pq3;
    sumtype                *p0, *p1, *p2, *p3;
    uint8_t                count;
};

#endif

//---------------------------------------------------------------------------//
//-------------------------- Exported Functions -----------------------------//
//---------------------------------------------------------------------------//
/// Shave entry point
/// @param cfg: Pointer to the FaceDetectConfig structure which
///				must be placed in CMX
#ifndef __LEON__

int cvRoundLocal( float value );

void SetImagesForHaarClassifierCascade(HaarClassifierCascade* _cascade,
	u8* hiddenCascadeBuff, mvcv::Mat* sum, mvcv::Mat* sqsum, float scale, u32* outChecksum);

HidHaarClassifierCascade* CreateHidHaarClassifierCascade(HaarClassifierCascade* cascade, 
	u8* hiddenCascadeBuff);

mvstl::vector<mvcv::Rect> HaarDetectObjectsForROC(mvcv::Mat& img, mvcv::Mat& sum, mvcv::Mat& sqsum, 
	u8* cascadeBuffer, u8* hidCascadeLocalBuffer, u8* hidCascadesAllBuffers[], 
	fp32 scaleFactor, u32 minNeighbors,	mvcv::CvSize maxWinSize, mvcv::CvSize minWinSize, 
	u32 winSizes[], u32 flags, bool usePrecomputedHid);

#endif

#endif // __HAAR_FEATURES_H__
