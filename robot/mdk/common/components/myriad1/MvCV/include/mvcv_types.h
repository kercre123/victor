#ifndef __CV_TYPES_H__
#define __CV_TYPES_H__

//Guard against inclusion in applications which contain standard OpenCV code
//#ifndef __OPENCV_OLD_CV_H__


#ifndef _WIN32
#include <stddef.h>
#endif

#include <assert.h>
#include <VectorTypes.h>
#include <mvstl_types.h>
#include <mvstl_utils.h>

#ifdef __MOVICOMPILE__
#define MVCV_STATIC_DATA __attribute((section(".mvcv_static_data")))
#else
#define MVCV_STATIC_DATA
#endif

namespace mvcv
{

#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#define CV_PI   3.1415926535897932384626433832795

/** Flags used by Optical Flow */
#define CV_TERMCRIT_ITER	1
#define CV_TERMCRIT_EPS		2

#define  CV_LKFLOW_PYR_A_READY       1
#define  CV_LKFLOW_PYR_B_READY       2
#define  CV_LKFLOW_INITIAL_GUESSES   4
#define  CV_LKFLOW_GET_MIN_EIGENVALS 8

#define CV_OK				0
#define CV_BADFLAG_ERR		-12
#define	CV_BADRANGE_ERR		-44
#define	CV_NULLPTR_ERR		-2
#define CV_BADSIZE_ERR		-1
/******************************************************************/

#define	DIM					4104
#define VecOp			8
#define castOp(x)		((x + (1 << (VecOp - 1))) >> VecOp)

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#ifndef CV_MAT_DEPTH
#define CV_MAT_DEPTH(flags)     ((flags) & ((1 << 3) - 1))
#endif
#ifndef CV_MAKETYPE
#define CV_MAKETYPE(depth,cn)	(CV_MAT_DEPTH(depth) + (((cn)-1) << 3))
#endif

/* 0x3a50 = 11 10 10 01 01 00 00 ~ array of log2(sizeof(arr_type_elem)) */
#define CV_ELEM_SIZE(type)		(CV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> CV_MAT_DEPTH(type)*2) & 3))

#define CV_MAT_CN_MASK          ((CV_CN_MAX - 1) << CV_CN_SHIFT)
#define CV_MAT_CN(flags)        ((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#define CV_MAT_TYPE_MASK        (CV_DEPTH_MAX*CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags)      ((flags) & CV_MAT_TYPE_MASK)
#define CV_MAT_CONT_FLAG_SHIFT  14
#define CV_MAT_CONT_FLAG        (1 << CV_MAT_CONT_FLAG_SHIFT)
#define CV_IS_MAT_CONT(flags)   ((flags) & CV_MAT_CONT_FLAG)
#define CV_IS_CONT_MAT          CV_IS_MAT_CONT
#define CV_SUBMAT_FLAG_SHIFT    15
#define CV_SUBMAT_FLAG          (1 << CV_SUBMAT_FLAG_SHIFT)
#define CV_IS_SUBMAT(flags)     ((flags) & CV_MAT_SUBMAT_FLAG)

enum { MAGIC_VAL = 0x42FF0000, AUTO_STEP = 0, CONTINUOUS_FLAG = CV_MAT_CONT_FLAG, SUBMATRIX_FLAG = CV_SUBMAT_FLAG };


/******************************** CvSize's & CvBox **************************************/

/**
 * Point types mapped on vector types
 */
#ifdef __MOVICOMPILE__

typedef int2	CLPoint32i;
typedef short2	CLPoint16i;
typedef ushort2	CLPoint16ui;
typedef float2	CLPoint32f;
typedef half2	CLPoint16f;

//Working vectorized types
typedef int4 CLPoint32iW;
typedef short4 CLPoint16iW;
typedef ushort4 CLPoint16uiW;
typedef float4 CLPoint32fW;
typedef half4 CLPoint16fW;
#else

#ifdef __PC__

template<typename T> class ClPoint
{
public:
	T x;
    T y;

	inline ClPoint()
	{}

	inline ClPoint(T x0, T y0)
	{
		x = x0;
		y = y0;
	}
};

typedef ClPoint<u32>	CLPoint32u;
typedef ClPoint<s32>	CLPoint32i;
typedef ClPoint<u16>	CLPoint16ui;
typedef ClPoint<fp32>	CLPoint32f;
typedef ClPoint<fp16>	CLPoint16f;

template<typename T> ClPoint<T> operator + (ClPoint<T> a, ClPoint<T> b)
{
	ClPoint<T> res;
	
	res.x = a.x + b.x;
	res.y = a.y + b.y;

	return res;
}

template<typename T> ClPoint<T> operator - (ClPoint<T> a, ClPoint<T> b)
{
	ClPoint<T> res;
	
	res.x = a.x - b.x;
	res.y = a.y - b.y;

	return res;
}

//template<typename T> ClPoint<T> operator * (fp32 s)
//{
//	ClPoint<T> res;
//	
//	res.x = a.x * s;
//	res.y = a.y * s;
//
//	return res;
//}

#endif

#endif

/*struct CvPoint
{
    int x;
    int y;

	inline CvPoint()
	{}

	inline CvPoint(int x0, int y0)
	{
		x = x0;
		y = y0;
	}
};

struct CvPoint2D32f
{
    float x;
    float y;

	inline CvPoint2D32f()
	{}

	inline CvPoint2D32f(float x0, float y0)
	{
		x = x0;
		y = y0;
	}
};*/

struct CvRect
{
    int x;
    int y;
    int width;
    int height;
};

#ifdef __MOVICOMPILE__

typedef int2 ClSize;
//Working version
typedef int4 ClSizeW;

#else

struct ClSize
{
    int x;
    int y;

	inline ClSize()
	{}

	inline ClSize(int width0, int height0)
	{
		x = width0;
		y = height0;
	}
};

#endif

struct CvSize
{
    int width;
    int height;

	CvSize();

	CvSize(int width0, int height0);
};


#ifdef __MOVICOMPILE__

typedef float4 Scalar;

#else

struct Scalar
{
	float val[4];

	inline Scalar()
	{}

	inline Scalar(float val0, float val1, float val2, float val3)
	{
		val[0] = val0;
		val[1] = val1;
		val[2] = val2;
		val[3] = val3;
	}

	inline Scalar(float val0)
	{
		val[0] = val0;
	}

	inline float& operator [] (const int i)
	{
		return val[i];
	}
};

#endif

struct Rect
{
	int x, y, width, height;

	inline Rect()
	{}

	inline Rect(int x0, int y0, int width0, int height0)
	{
		x = x0;
		y = y0;
		width = width0;
		height = height0;
	}
};

struct Range
{
	int start, end;

	inline Range()
	{}

	inline Range(int start0, int end0)
	{
		assert(end0 > start0);

		start = start0;
		end = end0;
	}
};

/*********************************** CvTermCriteria *************************************/

struct CvTermCriteria
{
    int		type;  /* may be combination of
                     CV_TERMCRIT_ITER
                     CV_TERMCRIT_EPS */
    u32		max_iter;
    fp32	epsilon;
};

struct CvMat
{
    int type;
    int step;
    u8* ptr;
    int width;
    int height;
};

union Cv32suf
{
    int i;
    unsigned u;
    float f;
};

typedef CLPoint32i		CvPoint;
typedef CLPoint16ui		CvPoint2D16ui;
typedef CLPoint32f		CvPoint2D32f;

typedef CvPoint2D32f	Point2f;
typedef CvPoint			Point;
typedef CvPoint			Point2i;

//Working *4 versions
#ifdef __PC__
typedef ClSize          ClSizeW;
typedef CLPoint32i      CLPoint32iW;
typedef CLPoint16ui  	CLPoint16uiW;
typedef CLPoint32f      CLPoint32fW;
#endif
typedef CLPoint32iW		CvPointW;
typedef CLPoint16uiW	CvPoint2D16uiW;
typedef CLPoint32fW		CvPoint2D32fW;

typedef CvPoint2D32fW	Point2fW;
typedef CvPointW		PointW;
typedef CvPointW		Point2iW;

/**
 * These functions construct basic data types
 */
CvSize cvSize(int width, int height);
ClSize clSize(int width, int height);
ClSizeW clSizeW(int width, int height);
Scalar cvScalar(float s0, float s1, float s2, float s3);
CvPoint cvPoint(int x, int y);
CvPointW cvPointW(int x, int y);
CvPoint2D32f cvPoint2D32f(float x, float y);
CvPoint2D32fW cvPoint2D32fW(float x, float y);
CvRect cvRect(int x, int y, int width, int height);
CvTermCriteria cvTermCriteria(int type, int max_iter, float epsilon);
int cvRound(float value);
int cvFloor(float value);
int cvCeil(float value);
int cvAlign(int size, int align);

CvMat cvMat( int height, int width, int type);
CvMat cvMat(CvSize size, u8* data, int type);
void cvSetData( CvMat* arr, u8* data, int step);

class Mat;
void cvSetData(Mat* mat, int rows, int cols, u8* const data, int type);

//Threshold types
enum
{
    CV_THRESH_BINARY      = 0,  /* value = value > threshold ? max_value : 0       */
    CV_THRESH_BINARY_INV  = 1,  /* value = value > threshold ? 0 : max_value       */
    CV_THRESH_TRUNC       = 2,  /* value = value > threshold ? threshold : value   */
    CV_THRESH_TOZERO      = 3,  /* value = value > threshold ? value : 0           */
    CV_THRESH_TOZERO_INV  = 4,  /* value = value > threshold ? 0 : value           */
    CV_THRESH_MASK        = 7,
    CV_THRESH_OTSU        = 8  /* use Otsu algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values */
};

//Border types
#define IPL_BORDER_CONSTANT		0
#define IPL_BORDER_REPLICATE	1
#define IPL_BORDER_REFLECT		2
#define IPL_BORDER_WRAP			3
#define IPL_BORDER_REFLECT_101  4
#define IPL_BORDER_TRANSPARENT  5

}


//#endif

#endif
