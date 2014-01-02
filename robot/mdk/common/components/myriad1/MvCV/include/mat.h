#ifndef __MAT_H__
#define __MAT_H__

#include "mvcv_types.h"
#include <mvstl_types.h>
#include <mvstl_utils.h>

namespace mvcv 
{

/** 
 * All the defines below are used to encode various information about the type,
 * depth, size of a Mat object.
 */

#define CV_CN_MAX     512
#define CV_CN_SHIFT   3
#ifndef CV_DEPTH_MAX
#define CV_DEPTH_MAX  (1 << CV_CN_SHIFT)
#endif

#define CV_8U   0
#define CV_8S   1
#define CV_16U  2
#define CV_16S  3
#define CV_32S  4
#define CV_32F  5
#define CV_USRTYPE1 7

#define CV_MAT_DEPTH_MASK       (CV_DEPTH_MAX - 1)

#ifndef CV_MAT_DEPTH
#define CV_MAT_DEPTH(flags)     ((flags) & CV_MAT_DEPTH_MASK)
#endif

#ifndef CV_MAKETYPE
#define CV_MAKETYPE(depth,cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))
#endif
#define CV_MAKE_TYPE CV_MAKETYPE

#define CV_8UC1 CV_MAKETYPE(CV_8U,1)
#define CV_8UC2 CV_MAKETYPE(CV_8U,2)
#define CV_8UC3 CV_MAKETYPE(CV_8U,3)
#define CV_8UC4 CV_MAKETYPE(CV_8U,4)
#define CV_8UC(n) CV_MAKETYPE(CV_8U,(n))

#define CV_8SC1 CV_MAKETYPE(CV_8S,1)
#define CV_8SC2 CV_MAKETYPE(CV_8S,2)
#define CV_8SC3 CV_MAKETYPE(CV_8S,3)
#define CV_8SC4 CV_MAKETYPE(CV_8S,4)
#define CV_8SC(n) CV_MAKETYPE(CV_8S,(n))

#define CV_16UC1 CV_MAKETYPE(CV_16U,1)
#define CV_16UC2 CV_MAKETYPE(CV_16U,2)
#define CV_16UC3 CV_MAKETYPE(CV_16U,3)
#define CV_16UC4 CV_MAKETYPE(CV_16U,4)
#define CV_16UC(n) CV_MAKETYPE(CV_16U,(n))

#define CV_16SC1 CV_MAKETYPE(CV_16S,1)
#define CV_16SC2 CV_MAKETYPE(CV_16S,2)
#define CV_16SC3 CV_MAKETYPE(CV_16S,3)
#define CV_16SC4 CV_MAKETYPE(CV_16S,4)
#define CV_16SC(n) CV_MAKETYPE(CV_16S,(n))

#define CV_32SC1 CV_MAKETYPE(CV_32S,1)
#define CV_32SC2 CV_MAKETYPE(CV_32S,2)
#define CV_32SC3 CV_MAKETYPE(CV_32S,3)
#define CV_32SC4 CV_MAKETYPE(CV_32S,4)
#define CV_32SC(n) CV_MAKETYPE(CV_32S,(n))

#define CV_32FC1 CV_MAKETYPE(CV_32F,1)
#define CV_32FC2 CV_MAKETYPE(CV_32F,2)
#define CV_32FC3 CV_MAKETYPE(CV_32F,3)
#define CV_32FC4 CV_MAKETYPE(CV_32F,4)
#define CV_32FC(n) CV_MAKETYPE(CV_32F,(n))

#define CV_MAT_TYPE_MASK        (CV_DEPTH_MAX*CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags)      ((flags) & CV_MAT_TYPE_MASK)

#define CV_MAT_CN_MASK          ((CV_CN_MAX - 1) << CV_CN_SHIFT)

#ifndef CV_MAT_CN
#define CV_MAT_CN(flags)        ((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#endif

#define CV_ELEM_SIZE(type) \
    (CV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> CV_MAT_DEPTH(type)*2) & 3))

/**
 * This class represents an image as a 2D matrix which supports various operations.
 */
class Mat
{
public:
	s32 type;		/**< Type of Mat object (CV_8UC1, CV_16SC2, CV_32FC3 etc) */
	s32 rows;		/**< Number of rows */
	s32 cols;		/**< Number of columns */
	s32 flags;		/**< Flags which encode various information like element size, channels etc */
	s32 size[2];	/**< size[0] = rows, size[1] = cols */
    s32 step0[2];	/**< step0[0] = step size in bytes, step0[1] = size of one element */
	u8* data;		/**< Buffer which holds the data */
	u8 allocated;	/** Bool value telling if the memeber data was dynamically allocated
					 * when the object was constructed */
	size_t step;	/**< Same value as step0[0] */

    /** Default constructor */
    Mat();
    /** Constructs 2D matrix of the specified size and type */
    Mat(s32 _rows, s32 _cols, s32 _type, u8* _data = 0, bool allocate = true, size_t _step = AUTO_STEP);
	/** Constructs 2D matrix of the specified size and type */
	Mat(s32 _rows, s32 _cols, s32 _type, Scalar s, u8* _data = 0, bool allocate = true);
	/** Constructs 2D matrix of the specified size and type */
	Mat(const Mat&, const Range& rowRange, const Range& colRange);
	/** Copy constructor is needed for functions returning objects of this type */
	Mat(const Mat& m);
	/** Destructor frees up memory if it was allocated upon construction */
	~Mat();
	//template<typename _Tp> _Tp& at(int i0, int i1);
	///Return reference to the specified array element
	///i, j Indices along the dimensions 0 and 1, respectively
	float& at(int i0, int i1);
	
	///Copies the matrix to another one.
	///@param dst Pointer to the destination matrix.
	void copyTo(Mat& dst);
	
	///Converts array to another datatype with optional scaling.
	///@param m     The destination matrix. If it does not have a proper size or type before the operation, it will be reallocated
	///@param rtype The desired destination matrix type, or rather, the depth (since the number of channels will be the same with the source one). If rtype is negative, the destination matrix will have the same type as the source.
	///@param alpha The optional scale factor
	///@param beta  The optional delta, added to the scaled values.
	void convertTo(Mat& m, int rtype, float alpha = 1, float beta = 0 ) const;
	
	///Allocates new array data if needed.
	
	///@param _rows   The new number of rows
	///@param _cols   The new number of columns
	///@param _type   The new matrix type
	void create(int _rows, int _cols, int _type);
	
	///Matrix assignment operators
	///@param m The assigned, right-hand-side matrix. 
	///Matrix assignment is O(1) operation, that is, no data is copied. 
	///Instead, the data is shared and the reference counter, if any, is incremented. 
	///Before assigning new data, the old data is dereferenced via Mat::release .
	Mat& operator = (const Mat& m);
	
	///Extracts a rectangular submatrix
	///@param r 	   The extracted submatrix specified as a rectangle
	Mat operator () (const Rect& r);

	///Extracts a rectangular submatrix
	///@param rowRange The start and the end row of the extracted submatrix. The upper boundary is not included. To select all the rows, use Range::all()
	///@param colRange The start and the end column of the extracted submatrix. The upper boundary is not included. To select all the columns, use Range::all()
	Mat operator () (const Range& rowRange, const Range& colRange);
	
	///Matrix divide operator
	///@param m the divisor matrix
	Mat operator / (const Mat& m);
	
	///Matrix divide operator
	///@param m the divisor float
	Mat operator / (const float m);

	///Performs element-wise multiplication or division of the two matrices
	///@param m     Another matrix, of the same type and the same size as *this , or a matrix expression
	///@param scale The optional scale factor
	Mat mul(Mat& m, float scale = 1.0);

	//!@{
	///Return pointer to the specified matrix row
	///@param y – The 0-based row index
	
	u8* ptr(int y = 0);
	const u8* ptr(int y = 0) const;
	//!@}
	
	///@return matrix element depth
	size_t depth() const;
	
	///@return normalized step
	size_t step1() const;
};

/*! \class Mat
	\brief OpenCV C++ n-dimensional dense array class.
	\fn Mat::Mat();
	The default class constructor	
*/

/*! 
	\overload Mat(s32 _rows, s32 _cols, s32 _type, u8* _data = 0, bool allocate = true, size_t _step = AUTO_STEP);
	\overload Mat(s32 _rows, s32 _cols, s32 _type, Scalar s, u8* _data = 0, bool allocate = true);
	\overload Mat(const Mat&, const Range& rowRange, const Range& colRange);
*/
/*template<typename _Tp> inline _Tp& Mat::at(int i0, int i1)
{
    return ((_Tp*)(data + step[0]*i0))[i1];
}*/

}

#endif
