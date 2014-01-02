///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     MvCV types
///
/// This is the implementation of MvCV types
///

#include <mvcv_types.h>

namespace mvcv
{

CvSize::CvSize()
{
}

CvSize::CvSize(int width0, int height0)
{
	width = width0;
	height = height0;
}

CvSize cvSize( int width, int height )
{
    CvSize s;

    s.width = width;
    s.height = height;

    return s;
}

ClSize clSize( int width, int height )
{
    ClSize s;

    s.x = width;
    s.y = height;

    return s;
}

ClSizeW clSizeW( int width, int height )
{
    ClSizeW s;

    s.x = width;
    s.y = height;

    return s;
}

Scalar cvScalar(float s0, float s1, float s2, float s3)
{
	Scalar s;

	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;

	return s;
}

CvPoint cvPoint( int x, int y )
{
    CvPoint p;

    p.x = x;
    p.y = y;

    return p;
}

CvPointW cvPointW( int x, int y )
{
    CvPointW p;

    p.x = x;
    p.y = y;

    return p;
}

CvPoint2D32f cvPoint2D32f( float x, float y )
{
    CvPoint2D32f p;

    p.x = (float)x;
    p.y = (float)y;

    return p;
}

CvPoint2D32fW cvPoint2D32fW( float x, float y )
{
    CvPoint2D32fW p;

    p.x = (float)x;
    p.y = (float)y;

    return p;
}

CvRect cvRect( int x, int y, int width, int height )
{
    CvRect r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}

CvTermCriteria cvTermCriteria( int type, int max_iter, float epsilon )
{
    CvTermCriteria t;

    t.type = type;
    t.max_iter = max_iter;
    t.epsilon = (float)epsilon;

    return t;
}

int cvRound( float value )
{
   int a;
   a =   (int)(value + (value >= 0 ? 0.5 : -0.5));
   return a;
}

int cvFloor( float value )
{
    int i = cvRound(value);
    Cv32suf diff;
    diff.f = (float)(value - i);
    return i - (diff.i < 0);
}

int cvCeil(float value)
{
	int i = cvRound(value);
    Cv32suf diff;
    diff.f = (float)(i - value);
    return i + (diff.i < 0);
}

int cvAlign( int size, int align )
{
    return (size + align - 1) & -align;
}

}
