/**
File: geometry.h
Author: Peter Barnum
Created: 2013

Definitions of geometry_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_POINT_H_
#define _ANKICORETECHEMBEDDED_COMMON_POINT_H_

#include "coretech/common/robot/array2d_declarations.h"
#include "coretech/common/robot/geometry_declarations.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/matrix.h"

#include "coretech/common/shared/utilities_shared.h"

namespace Anki
{
  namespace Embedded
  {
#if 0
#pragma mark --- 2D Point Implementations ---
#endif
    template<typename Type> Point<Type>::Point()
      : x(static_cast<Type>(0)), y(static_cast<Type>(0))
    {
    }

    template<typename Type> Point<Type>::Point(const Type x, const Type y)
      : x(x), y(y)
    {
    }

    template<typename Type> Point<Type>::Point(const Point<Type>& pt)
      : x(pt.x), y(pt.y)
    {
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> Point<Type>::Point(const cv::Point_<Type>& pt)
      : x(pt.x), y(pt.y)
    {
    }
#endif

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> cv::Point_<Type> Point<Type>::get_CvPoint_() const
    {
      return cv::Point_<Type>(x,y);
    }
#endif

    template<typename Type> template<typename InType> void Point<Type>::SetCast(const Point<InType> &in)
    {
      this->x = saturate_cast<Type>(in.x);
      this->y = saturate_cast<Type>(in.y);
    }

    template<typename Type> void Point<Type>::Print() const
    {
      CoreTechPrint("(%d,%d) ", this->x, this->y);
    }

    template<typename Type> bool Point<Type>::operator== (const Point<Type> &point2) const
    {
      if(this->x == point2.x && this->y == point2.y)
        return true;

      return false;
    }

    template<typename Type> Point<Type> Point<Type>::operator+ (const Point<Type> &point2) const
    {
      return Point<Type>(this->x+point2.x, this->y+point2.y);
    }

    template<typename Type> Point<Type> Point<Type>::operator- (const Point<Type> &point2) const
    {
      return Point<Type>(this->x-point2.x, this->y-point2.y);
    }

    template<typename Type> Point<Type> Point<Type>::operator- () const
    {
      return Point<Type>(-this->x, -this->y);
    }

    template<typename Type> Point<Type>& Point<Type>::operator*= (const Type value)
    {
      this->x *= value;
      this->y *= value;
      return *this;
    }

    template<typename Type> Point<Type>& Point<Type>::operator-= (const Type value)
    {
      this->x -= value;
      this->y -= value;
      return *this;
    }

    template<typename Type> Point<Type>& Point<Type>::operator+= (const Point<Type> &point2)
    {
      this->x += point2.x;
      this->y += point2.y;
      return *this;
    }

    template<typename Type> Point<Type>& Point<Type>::operator-= (const Point<Type> &point2)
    {
      this->x -= point2.x;
      this->y -= point2.y;
      return *this;
    }

    template<typename Type> inline Point<Type>& Point<Type>::operator= (const Point<Type> &point2)
    {
      this->x = point2.x;
      this->y = point2.y;

      return *this;
    }

    template<typename Type> f32 Point<Type>::Dist(const Point<Type> &point2) const
    {
      return (f32)sqrt((this->x - point2.x)*(this->x - point2.x) + (this->y - point2.y)*(this->y - point2.y));
    }

    template<typename Type> f32 Point<Type>::Length() const
    {
      return (f32)sqrt((f32)((this->x*this->x) + (this->y*this->y)));
    }

    // #pragma mark --- Point Specializations ---
    template<> void Point<f32>::Print() const;
    template<> void Point<f64>::Print() const;

#if 0
#pragma mark --- 3D Point Implementations ---
#endif

    template<typename Type> Point3<Type>::Point3()
      : x(static_cast<Type>(0)), y(static_cast<Type>(0)), z(static_cast<Type>(0))
    {
    }

    template<typename Type> Point3<Type>::Point3(const Type x, const Type y, const Type z)
      : x(x), y(y), z(z)
    {
    }

    template<typename Type> Point3<Type>::Point3(const Point3<Type>& pt)
      : x(pt.x), y(pt.y), z(pt.z)
    {
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> Point3<Type>::Point3(const cv::Point3_<Type>& pt)
      : x(pt.x), y(pt.y), z(pt.z)
    {
    }
#endif

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> cv::Point3_<Type> Point3<Type>::get_CvPoint_() const
    {
      return cv::Point3_<Type>(x,y,z);
    }
#endif

    template<typename Type> void Point3<Type>::Print() const
    {
      CoreTechPrint("(%d,%d,%d) ", this->x, this->y, this->z);
    }

    template<typename Type> bool Point3<Type>::operator== (const Point3<Type> &point2) const
    {
      if(this->x == point2.x && this->y == point2.y && this->z == point2.z)
        return true;

      return false;
    }

    template<typename Type> Point3<Type> Point3<Type>::operator+ (const Point3<Type> &point2) const
    {
      return Point3<Type>(this->x+point2.x, this->y+point2.y, this->z+point2.z);
    }

    template<typename Type> Point3<Type> Point3<Type>::operator- (const Point3<Type> &point2) const
    {
      return Point3<Type>(this->x-point2.x, this->y-point2.y, this->z-point2.z);
    }

    template<typename Type> Point3<Type> Point3<Type>::operator- () const
    {
      return Point3<Type>(-this->x, -this->y, -this->z);
    }

    template<typename Type> Point3<Type>& Point3<Type>::operator*= (const Type value)
    {
      this->x *= value;
      this->y *= value;
      this->z *= value;
      return *this;
    }

    template<typename Type> Point3<Type>& Point3<Type>::operator-= (const Type value)
    {
      this->x -= value;
      this->y -= value;
      this->z -= value;
      return *this;
    }

    template<typename Type> Point3<Type>& Point3<Type>::operator-= (const Point3<Type> &point2)
    {
      this->x -= point2.x;
      this->y -= point2.y;
      this->z -= point2.z;
      return *this;
    }

    template<typename Type> inline Point3<Type>& Point3<Type>::operator= (const Point3<Type> &point2)
    {
      this->x = point2.x;
      this->y = point2.y;
      this->z = point2.z;
      return *this;
    }

    template<typename Type> f32 Point3<Type>::Dist(const Point3<Type> &point2) const
    {
      return (f32)sqrt((this->x - point2.x)*(this->x - point2.x) +
        (this->y - point2.y)*(this->y - point2.y) +
        (this->z - point2.z)*(this->z - point2.z));
    }

    template<typename Type> f32 Point3<Type>::Length() const
    {
      return (f32)sqrt((f32)((this->x*this->x) + (this->y*this->y) + (this->z*this->z)));
    }

    template<typename Type> f32 Point3<Type>::MakeUnitLength()
    {
      const f32 L = this->Length();
      if(L != 0) {
        this->operator*=(1.f / L);
      }
      return L;
    }

    template<typename Type>
    Type DotProduct(const Point3<Type>& point1, const Point3<Type>& point2)
    {
      return (point1.x*point2.x) + (point1.y*point2.y) + (point1.z*point2.z);
    }

    template<typename Type>
    Point3<Type> CrossProduct(const Point3<Type>& point1, const Point3<Type>& point2)
    {
      return Point3<Type>(-point2.y*point1.z + point1.y*point2.z,
        point2.x*point1.z - point1.x*point2.z,
        -point2.x*point1.y + point1.x*point2.y);
    }

    template<typename Type>
    Point3<Type> operator* (const Array<Type>& M, const Point3<Type>& p)
    {
      // Matrix M must be 3x3
      AnkiAssert(AreEqualSize(3, 3, M));

      return Point3<Type>(M[0][0]*p.x + M[0][1]*p.y + M[0][2]*p.z,
        M[1][0]*p.x + M[1][1]*p.y + M[1][2]*p.z,
        M[2][0]*p.x + M[2][1]*p.y + M[2][2]*p.z);
    }

    // #pragma mark --- Point Specializations ---
    template<> void Point3<f32>::Print() const;
    template<> void Point3<f64>::Print() const;

#if 0
#pragma mark --- Pose Implementations ---
#endif

    template<typename Type>
    Result ComputePoseDiff(const Array<Type>& R1, const Point3<Type>& T1,
      const Array<Type>& R2, const Point3<Type>& T2,
      Array<Type>& Rdiff, Point3<Type>& Tdiff,
      MemoryStack scratch)
    {
      // All the rotation matrices should be 3x3
      AnkiAssert(AreEqualSize(3, 3, R1));
      AnkiAssert(AreEqualSize(3, 3, R2));
      AnkiAssert(AreEqualSize(3, 3, Rdiff));

      Array<Type> invR1 = Array<Type>(3,3,scratch);
      Matrix::Transpose(R1, invR1);

      Matrix::Multiply(invR1, R2, Rdiff);
      Tdiff = invR1 * (T2 - T1);

      return RESULT_OK;
    }

#if 0
#pragma mark --- Rectangle Implementations ---
#endif

    template<typename Type> Rectangle<Type>::Rectangle()
      : left(static_cast<Type>(0)), right(static_cast<Type>(0)), top(static_cast<Type>(0)), bottom(static_cast<Type>(0))
    {
    }

    template<typename Type> Rectangle<Type>::Rectangle(const Type left, const Type right, const Type top, const Type bottom)
      : left(left), right(right), top(top), bottom(bottom)
    {
    }

    template<typename Type> Rectangle<Type>::Rectangle(const Rectangle<Type>& rect)
      : left(rect.left), right(rect.right), top(rect.top), bottom(rect.bottom)
    {
    }

    template<typename Type> void Rectangle<Type>::Print() const
    {
      CoreTechPrint("(%d,%d)->(%d,%d) ", this->left, this->top, this->right, this->bottom);
    }

    template<typename Type> template<typename OutType> Point<OutType> Rectangle<Type>::ComputeCenter() const
    {
      Point<OutType> center(
        (static_cast<OutType>(this->left) + static_cast<OutType>(this->right)) / 2,
        (static_cast<OutType>(this->top) + static_cast<OutType>(this->bottom)) / 2);

      return center;
    }

    template<typename Type> template<typename OutType> Rectangle<OutType> Rectangle<Type>::ComputeScaledRectangle(const f32 scalePercent) const
    {
      const f32 width = static_cast<f32>(this->get_width());
      const f32 height = static_cast<f32>(this->get_height());

      const f32 scaledWidth = width * scalePercent;
      const f32 scaledHeight = height * scalePercent;

      const f32 dx2 = (scaledWidth - width) / 2.0f;
      const f32 dy2 = (scaledHeight - height) / 2.0f;

      Rectangle<OutType> scaledRect(
        static_cast<OutType>( static_cast<f32>(this->left)   - dx2 ),
        static_cast<OutType>( static_cast<f32>(this->right)  + dx2 ),
        static_cast<OutType>( static_cast<f32>(this->top)    - dy2 ),
        static_cast<OutType>( static_cast<f32>(this->bottom) + dy2 ));

      return scaledRect;
    }

    template<typename Type> bool Rectangle<Type>::operator== (const Rectangle<Type> &rectangle2) const
    {
      if(this->left == rectangle2.left && this->top == rectangle2.top && this->right == rectangle2.right && this->bottom == rectangle2.bottom)
        return true;

      return false;
    }

    template<typename Type> Rectangle<Type> Rectangle<Type>::operator+ (const Rectangle<Type> &rectangle2) const
    {
      return Rectangle<Type>(this->top+rectangle2.top, this->bottom+rectangle2.bottom, this->left+rectangle2.left, this->right+rectangle2.right);
    }

    template<typename Type> Rectangle<Type> Rectangle<Type>::operator- (const Rectangle<Type> &rectangle2) const
    {
      return Rectangle<Type>(this->top-rectangle2.top, this->bottom-rectangle2.bottom, this->left-rectangle2.left, this->right-rectangle2.right);
    }

    template<typename Type> inline Rectangle<Type>& Rectangle<Type>::operator= (const Rectangle<Type> &rect2)
    {
      this->left = rect2.left;
      this->right = rect2.right;
      this->top = rect2.top;
      this->bottom = rect2.bottom;

      return *this;
    }

    template<typename Type> Type Rectangle<Type>::get_width() const
    {
      return right - left;
    }

    template<typename Type> Type Rectangle<Type>::get_height() const
    {
      return bottom - top;
    }

    // #pragma mark --- Rectangle Specializations ---
    template<> void Rectangle<f32>::Print() const;
    template<> void Rectangle<f64>::Print() const;

    // #pragma mark --- Quadrilateral Definitions ---

    template<typename Type> Quadrilateral<Type>::Quadrilateral()
    {
      for(s32 i=0; i<4; i++) {
        corners[i] = Point<Type>();
      }
    }

    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Point<Type> &corner1, const Point<Type> &corner2, const Point<Type> &corner3, const Point<Type> &corner4)
    {
      corners[0] = corner1;
      corners[1] = corner2;
      corners[2] = corner3;
      corners[3] = corner4;
    }

    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Quadrilateral<Type>& quad2)
    {
      for(s32 i=0; i<4; i++) {
        this->corners[i] = quad2.corners[i];
      }
    }

    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Rectangle<Type>& rect)
    {
      this->corners[0].x = rect.left;   this->corners[0].y = rect.top;
      this->corners[1].x = rect.right;  this->corners[1].y = rect.top;
      this->corners[2].x = rect.left;   this->corners[2].y = rect.bottom;
      this->corners[3].x = rect.right;  this->corners[3].y = rect.bottom;
    }

    template<typename Type> void Quadrilateral<Type>::Print() const
    {
      CoreTechPrint("{(%d,%d), (%d,%d), (%d,%d), (%d,%d)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }

    template<typename Type> template<typename OutType> Point<OutType> Quadrilateral<Type>::ComputeCenter() const
    {
      Point<OutType> center(0, 0);

      for(s32 i=0; i<4; i++) {
        center.x += static_cast<OutType>(this->corners[i].x);
        center.y += static_cast<OutType>(this->corners[i].y);
      }

      center.x /= 4;
      center.y /= 4;

      return center;
    }

    template<typename Type> template<typename OutType> Rectangle<OutType> Quadrilateral<Type>::ComputeBoundingRectangle() const
    {
      Rectangle<OutType> boundingRect(
        static_cast<OutType>(this->corners[0].x),
        static_cast<OutType>(this->corners[0].x),
        static_cast<OutType>(this->corners[0].y),
        static_cast<OutType>(this->corners[0].y));

      // Initialize the template rectangle to the bounding box of the given
      // quadrilateral
      for(s32 i=1; i<4; ++i) {
        boundingRect.left   = MIN(boundingRect.left,   static_cast<OutType>(this->corners[i].x));
        boundingRect.right  = MAX(boundingRect.right,  static_cast<OutType>(this->corners[i].x));
        boundingRect.top    = MIN(boundingRect.top,    static_cast<OutType>(this->corners[i].y));
        boundingRect.bottom = MAX(boundingRect.bottom, static_cast<OutType>(this->corners[i].y));
      }

      return boundingRect;
    }

    template<typename Type> template<typename OutType> Quadrilateral<OutType> Quadrilateral<Type>::ComputeClockwiseCorners() const
    {
      char tmpBuffer[128];
      MemoryStack scratch(tmpBuffer, 128);

      Array<f32> thetas(1,4,scratch);
      Array<s32> indexes(1,4,scratch);
      Point<f32> center = this->ComputeCenter<f32>();

      for(s32 i=0; i<4; i++) {
        f32 rho = 0.0f;

        Cart2Pol<f32>(
          static_cast<f32>(this->corners[i].x) - center.x,
          static_cast<f32>(this->corners[i].y) - center.y,
          rho, thetas[0][i]);
      }

      Matrix::InsertionSort(thetas, indexes, 1);

      const Quadrilateral<OutType> sortedQuad(
        Point<OutType>(static_cast<OutType>(this->corners[indexes[0][0]].x), static_cast<OutType>(this->corners[indexes[0][0]].y)),
        Point<OutType>(static_cast<OutType>(this->corners[indexes[0][1]].x), static_cast<OutType>(this->corners[indexes[0][1]].y)),
        Point<OutType>(static_cast<OutType>(this->corners[indexes[0][2]].x), static_cast<OutType>(this->corners[indexes[0][2]].y)),
        Point<OutType>(static_cast<OutType>(this->corners[indexes[0][3]].x), static_cast<OutType>(this->corners[indexes[0][3]].y)));

      return sortedQuad;
    }

    template<typename Type> template<typename OutType> Quadrilateral<OutType> Quadrilateral<Type>::ComputeRotatedCorners(const f32 radians) const
    {
      Point<f32> center = this->ComputeCenter<f32>();

      Quadrilateral<OutType> rotatedQuad;

      for(s32 i=0; i<4; i++) {
        f32 rho;
        f32 theta;

        if((center.x == static_cast<f32>(this->corners[i].x)) && (center.y == static_cast<f32>(this->corners[i].y))) {
          return *this;
        }

        Cart2Pol<f32>(
          static_cast<f32>(this->corners[i].x) - center.x,
          static_cast<f32>(this->corners[i].y) - center.y,
          rho, theta);

        f32 newX;
        f32 newY;

        Pol2Cart<f32>(
          rho, theta + radians,
          newX, newY);

        rotatedQuad[i] = Point<OutType>(static_cast<OutType>(newX + center.x), static_cast<OutType>(newY + center.y));
      }

      return rotatedQuad;
    }

    template<typename Type> bool Quadrilateral<Type>::IsConvex() const
    {
      Quadrilateral<Type> sortedQuad = this->ComputeClockwiseCorners<Type>();

      for(s32 iCorner=0; iCorner<4; iCorner++) {
        const Point<Type> &corner1 = sortedQuad[iCorner];
        const Point<Type> &corner2 = sortedQuad[(iCorner+1) % 4];
        const Point<Type> &corner3 = sortedQuad[(iCorner+2) % 4];

        const Type orientation =
          ((corner2.y - corner1.y) * (corner3.x - corner2.x)) -
          ((corner2.x - corner1.x) * (corner3.y - corner2.y));

        if((orientation - static_cast<Type>(0.001)) > 0) {
          return false;
        }
      }

      return true;
    }

    template<typename Type> bool Quadrilateral<Type>::operator== (const Quadrilateral<Type> &quad2) const
    {
      for(s32 i=0; i<4; i++) {
        if(!(this->corners[i] == quad2.corners[i]))
          return false;
      }

      return true;
    }

    template<typename Type> Quadrilateral<Type> Quadrilateral<Type>::operator+ (const Quadrilateral<Type> &quad2) const
    {
      Quadrilateral<Type> newQuad;

      for(s32 i=0; i<4; i++) {
        newQuad.corners[i] = this->corners[i] + quad2.corners[i];
      }

      return newQuad;
    }

    template<typename Type> Quadrilateral<Type> Quadrilateral<Type>::operator- (const Quadrilateral<Type> &quad2) const
    {
      Quadrilateral<Type> newQuad;

      for(s32 i=0; i<4; i++) {
        newQuad.corners[i] = this->corners[i] - quad2.corners[i];
      }

      return newQuad;
    }

    template<typename Type> inline Quadrilateral<Type>& Quadrilateral<Type>::operator= (const Quadrilateral<Type> &quad2)
    {
      for(s32 i=0; i<4; i++) {
        this->corners[i] = quad2.corners[i];
      }

      return *this;
    }

    template<typename Type>
    template<typename InType>
    void Quadrilateral<Type>::SetCast(const Quadrilateral<InType> &quad2)
    {
      for(s32 i=0; i<4; i++) {
        this->corners[i].SetCast(quad2.corners[i]);
      }
    }

    template<typename Type> inline const Point<Type>& Quadrilateral<Type>::operator[] (const s32 index) const
    {
      return corners[index];
    }

    template<typename Type> inline Point<Type>& Quadrilateral<Type>::operator[] (const s32 index)
    {
      return corners[index];
    }

    // #pragma mark --- Quadrilateral Specializations ---
    template<> void Quadrilateral<f32>::Print() const;
    template<> void Quadrilateral<f64>::Print() const;
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_H_
