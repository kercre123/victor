#ifndef __POINT_H__
#define __POINT_H__

namespace mvcv
{

//////////////////////////////// 2D Point ////////////////////////////////

/*!
  template 2D point class.
  
  The class defines a point in 2D space. Data type of the point coordinates is specified
  as a template parameter. There are a few shorter aliases available for user convenience. 
  See cv::Point, cv::Point2i, cv::Point2f and cv::Point2d.
*/  
/*template<typename _Tp> 
class Point_
{
public:
    typedef _Tp value_type;
    
    // various constructors
    Point_();
    Point_(_Tp _x, _Tp _y);

    Point_& operator = (const Point_& pt);
    
    _Tp x, y; //< the point coordinates
};

typedef Point_<float>		Point2f;
typedef Point_<int>		Point2i;
typedef Point2i			Point;

template<typename _Tp> inline Point_<_Tp>::Point_() : x(0), y(0) {}
template<typename _Tp> inline Point_<_Tp>::Point_(_Tp _x, _Tp _y) : x(_x), y(_y) {}

template<typename _Tp> inline Point_<_Tp>& Point_<_Tp>::operator = (const Point_& pt)
{ x = pt.x; y = pt.y; return *this; }

template<typename _Tp> static inline Point_<_Tp>&
	operator += (Point_<_Tp>& a, const Point_<_Tp>& b)
{
    a.x = a.x + b.x;
    a.y = a.y + b.y;
    return a;
}

template<typename _Tp> static inline Point_<_Tp>&
	operator -= (Point_<_Tp>& a, const Point_<_Tp>& b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    return a;
}

template<typename _Tp> static inline Point_<_Tp>&
	operator *= (Point_<_Tp>& a, int b)
{
    a.x = a.x*b;
    a.y = a.y*b;
    return a;
}

template<typename _Tp> static inline Point_<_Tp>&
	operator *= (Point_<_Tp>& a, float b)
{
    a.x = a.x*b;
    a.y = a.y*b;
    return a;
}

template<typename _Tp> static inline bool operator == (const Point_<_Tp>& a, const Point_<_Tp>& b)
{ return a.x == b.x && a.y == b.y; }

template<typename _Tp> static inline bool operator != (const Point_<_Tp>& a, const Point_<_Tp>& b)
{ return a.x != b.x || a.y != b.y; }

template<typename _Tp> static inline Point_<_Tp> operator + (const Point_<_Tp>& a, const Point_<_Tp>& b)
{ return Point_<_Tp>( a.x + b.x, a.y + b.y ); }

template<typename _Tp> static inline Point_<_Tp> operator - (const Point_<_Tp>& a, const Point_<_Tp>& b)
{ return Point_<_Tp>( a.x - b.x, a.y - b.y ); }

template<typename _Tp> static inline Point_<_Tp> operator - (const Point_<_Tp>& a)
{ return Point_<_Tp>( -a.x, -a.y ); }

template<typename _Tp> static inline Point_<_Tp> operator * (const Point_<_Tp>& a, int b)
{ return Point_<_Tp>( a.x*b, a.y*b ); }
    
template<typename _Tp> static inline Point_<_Tp> operator * (const Point_<_Tp>& a, float b)
{ return Point_<_Tp>( a.x*b, a.y*b ); }

inline Point2f cvPoint2D32f(float x, float y)
{
    Point2f p;

    p.x = (float)x;
    p.y = (float)y;

    return p;
}

inline Point2i cvPoint(int x, int y)
{
    Point2i p;

    p.x = x;
    p.y = y;

    return p;
}*/

}

#endif
