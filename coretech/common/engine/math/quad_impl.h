/**
 * File: quad_impl.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Implements a general N-dimensional Quadrilateral class for holding
 *              four points in arbitrary positions.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_COMMON_QUAD_IMPL_H_
#define _ANKICORETECH_COMMON_QUAD_IMPL_H_

#include "coretech/common/engine/math/quad.h"

#include "coretech/common/engine/exceptions.h"

#include "coretech/common/engine/math/linearAlgebra_impl.h"
#include "coretech/common/engine/math/triangle_impl.h"
#include "coretech/common/engine/utils/helpers/compareFcns.h"
#include "coretech/common/shared/utilities_shared.h"

#include "kazmath/src/kazmath.h"

#include "util/logging/logging.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp" // for minAreaRect
#endif

#include <cmath>
#include <array>


namespace Anki {
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral()
  {
    
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Point<N,T> &cornerTopLeft,
                                    const Point<N,T> &cornerBottomLeft,
                                    const Point<N,T> &cornerTopRight,
                                    const Point<N,T> &cornerBottomRight)
  {
    (*this)[Quad::TopLeft]     = cornerTopLeft;
    (*this)[Quad::TopRight]    = cornerTopRight;
    (*this)[Quad::BottomLeft]  = cornerBottomLeft;
    (*this)[Quad::BottomRight] = cornerBottomRight;
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral(std::initializer_list<Point<N,T> >& points)
  : std::array<Point<N,T>, 4>(points)
  {
    
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Quadrilateral<N,T>& quad)
  : std::array<Point<N,T>,4>(quad)
  {
  
  }

  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Rectangle<T>& rect)
  {
    static_assert(N == 2, "can only create 2d Quad from Rectangle");

    (*this)[Quad::TopLeft].x() = rect.GetX();
    (*this)[Quad::TopLeft].y() = rect.GetY();
    
    (*this)[Quad::BottomLeft].x() = rect.GetX();
    (*this)[Quad::BottomLeft].y() = rect.GetY() + rect.GetHeight();
    
    (*this)[Quad::TopRight].x() = rect.GetX() + rect.GetWidth();
    (*this)[Quad::TopRight].y() = rect.GetY();
    
    (*this)[Quad::BottomRight].x() = rect.GetX() + rect.GetWidth();
    (*this)[Quad::BottomRight].y() = rect.GetY() + rect.GetHeight();
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Quadrilateral<N+1,T>& quad)
  {

    (*this)[Quad::TopLeft] = Point<N,T>(quad[Quad::TopLeft]);
    (*this)[Quad::TopRight] = Point<N,T>(quad[Quad::TopRight]);
    (*this)[Quad::BottomLeft] = Point<N,T>(quad[Quad::BottomLeft]);
    (*this)[Quad::BottomRight] = Point<N,T>(quad[Quad::BottomRight]);
    
  }
  

  template<QuadDimType N, typename T>
  inline void Quadrilateral<N,T>::Print(void) const
  {
    using namespace Quad;
    CoreTechPrint("Quad: ");
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      CoreTechPrint("(");
      for (u8 p = 0; p < N; ++p) {
        CoreTechPrint(" %f", (*this)[i][p]);
      }
      CoreTechPrint(")\n");
    }
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T> Quadrilateral<N,T>::operator+
  (const Quadrilateral<N,T> &quad2) const
  {
    Quadrilateral<N,T> newQuad(*this);
    newQuad += quad2;
    return newQuad;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T> Quadrilateral<N,T>::operator-
  (const Quadrilateral<N,T> &quad2) const
  {
    Quadrilateral<N,T> newQuad(*this);
    newQuad -= quad2;
    return newQuad;
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>& Quadrilateral<N,T>::operator+= (const Quadrilateral<N,T> &quad2)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] += quad2[i];
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T>& Quadrilateral<N,T>::operator-= (const Quadrilateral<N,T> &quad2)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] -= quad2[i];
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T>&  Quadrilateral<N,T>::operator*=(const T value)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] *= value;
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T>&  Quadrilateral<N,T>::operator+=(const T value)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] += value;
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T>&  Quadrilateral<N,T>::operator+=(const Point<N,T> &point)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] += point;
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  inline Quadrilateral<N,T>&  Quadrilateral<N,T>::operator-=(const Point<N,T> &point)
  {
    using namespace Quad;
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      (*this)[i] -= point;
    }
    return *this;
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>  Quadrilateral<N,T>::GetScaled(const T scaleFactor) const
  {
    Quadrilateral<N,T> scaledQuad(*this);
    scaledQuad *= scaleFactor;
    return scaledQuad;
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::GetMinX(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].x(),
               MIN((*this)[TopRight].x(),
                   MIN((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::GetMinY(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].y(),
               MIN((*this)[TopRight].y(),
                   MIN((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::GetMaxX(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].x(),
               MAX((*this)[TopRight].x(),
                   MAX((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::GetMaxY(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].y(),
               MAX((*this)[TopRight].y(),
                   MAX((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<QuadDimType N, typename T>
  Point<N,T> Quadrilateral<N,T>::ComputeCentroid(void) const
  {
    using namespace Quad;
    
    Point<N,T> centroid((*this)[TopLeft]);
    centroid += (*this)[TopRight];
    centroid += (*this)[BottomLeft];
    centroid += (*this)[BottomRight];
    centroid /= T(4);

    return centroid;
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T>& Quadrilateral<N,T>::Scale(const T scaleFactor)
  {
    using namespace Quad;
    
    Point<N,T> centroid = this->ComputeCentroid();
    
    // Shift each point to be relative to the center point, scale it,
    // and then put it back relative to that center
    for(CornerName i_corner = FirstCorner;
        i_corner < NumCorners; ++i_corner)
    {
      (*this)[i_corner] -= centroid;
      (*this)[i_corner] *= scaleFactor;
      (*this)[i_corner] += centroid;
    }
    
    return *this;
  } // operator*=
  
  template<QuadDimType N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::operator[] (const Quad::CornerName whichCorner) const
  {
    //return this->corners[whichCorner];
    return std::array<Point<N,T>,4>::operator[](static_cast<int>(whichCorner));
  }

  
  template<QuadDimType N, typename T>
  inline Point<N,T>& Quadrilateral<N,T>::operator[] (const Quad::CornerName whichCorner)
  {
    //return this->corners[whichCorner];
    return std::array<Point<N,T>,4>::operator[](static_cast<int>(whichCorner));
  }
  
  template<QuadDimType N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::GetTopLeft() const {
    return this->operator[](Quad::TopLeft);
  }
  
  template<QuadDimType N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::GetBottomLeft() const {
    return this->operator[](Quad::BottomLeft);
  }
  
  template<QuadDimType N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::GetTopRight() const {
    return this->operator[](Quad::TopRight);
  }
  
  template<QuadDimType N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::GetBottomRight() const {
    return this->operator[](Quad::BottomRight);
  }
  
  template<QuadDimType N, typename T>
  Quadrilateral<2,T> Quadrilateral<N,T>::SortCornersClockwise() const
  {
    // Catch invalid usage of this method with a non-2D quad at compile time.
    static_assert(N==2, "No normal should be supplied when sorting a 2D Quadrilateral's corners.");

    using namespace Quad;
    
    std::array<std::pair<f32,Quad::CornerName>,4> angleIndexPairs;
    
    Point<N,T> center = this->ComputeCentroid();
    
    for(CornerName i=FirstCorner; i<NumCorners; ++i) {
      Point<N,T> corner((*this)[i]);
      corner -= center;
      
      angleIndexPairs[i].first  = atan2f(static_cast<f32>(corner.y()), static_cast<f32>(corner.x()));
      angleIndexPairs[i].second = i;
    }
    
    // Sort by angle, bringing corner index along for the ride
    std::sort(angleIndexPairs.begin(), angleIndexPairs.end(), CompareFirst<f32,Quad::CornerName>());
    
    // Re-order the corners using the sorted indices
    const Quadrilateral<2,T> sortedQuad((*this)[angleIndexPairs[0].second],   // TopLeft    (min angle)
                                        (*this)[angleIndexPairs[3].second],   // BottomLeft (max angle)
                                        (*this)[angleIndexPairs[1].second],   // TopRight
                                        (*this)[angleIndexPairs[2].second]);
    
    return sortedQuad;
    
  } // SortCornersClockWise(), Specialized for N==2
  
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T> Quadrilateral<N,T>::SortCornersClockwise(const Point<N,T>& aroundNormal) const
  {
    // Catch invalid usage of this method with a 2D quad at compile time.
    static_assert(N != 2, "A normal must be supplied when sorting an ND Quadrilateral's corners.");
    
    using namespace Quad;
    
    Point<N,T> unitNormal(aroundNormal);
    unitNormal.MakeUnitLength();

    Point<N,T> center = this->ComputeCentroid();
    SmallSquareMatrix<N,T> P = GetProjectionOperator(unitNormal);
    center = P * center;

    // Need to establish an arbitrary coordinate system in the plane.
    // Use the first corner as the first coordinate axis.
    Point<N,T> unitU(P * (*this)[Quad::FirstCorner]);
    unitU -= center;
    unitU.MakeUnitLength();
    
    // The cross product of the first coordinate axis vector with the normal
    // to define the (orthogonal) other coordinate axis vector
    Point<N,T> unitV( CrossProduct(unitNormal, unitU) );
    
    // By definition, the first corner's angle is 0.  Sort the other three
    // relative to it, using the plane's coordinate system we just established
    std::array<std::pair<f32,CornerName>,3> angleIndexPairs;
    for(s32 i=0; i<3; i++)
    {
      CornerName index = static_cast<CornerName>(i+1);
      Point<N,T> corner( P * (*this)[index] );
      corner -= center;
      
      const T u = DotProduct(corner, unitU);
      const T v = DotProduct(corner, unitV);
      
      angleIndexPairs[i].first = atan2f(static_cast<f32>(v), static_cast<f32>(u));
      angleIndexPairs[i].second = index;
    }
    
    // Sort by angle, taking the corner index along for the ride
    std::sort(angleIndexPairs.begin(), angleIndexPairs.end(), CompareFirst<f32,Quad::CornerName>());
    
    const Quadrilateral<N,T> sortedQuad((*this)[Quad::FirstCorner],
                                        (*this)[angleIndexPairs[0].second],
                                        (*this)[angleIndexPairs[1].second],
                                        (*this)[angleIndexPairs[2].second]);
              
    return sortedQuad;
  }
  
  template<typename T>
  Quadrilateral<2,T> GetBoundingQuad(std::vector<Point<2,T> >& points)
  {
#if ANKICORETECH_USE_OPENCV
    std::vector<cv::Point2f> cvPoints(points.size());
    for(s32 i_corner = 0; i_corner < points.size(); ++i_corner) {
      cvPoints[i_corner].x = static_cast<f32>(points[i_corner].x());
      cvPoints[i_corner].y = static_cast<f32>(points[i_corner].y());
    }
    
    cv::RotatedRect cvRotatedRect;
  
    try {
      cvRotatedRect = cv::minAreaRect(cvPoints);
    }
    catch(...) {
      // No idea how this happens, but we dont' want to die when it does.
      // Instead, I'm catching it here and using a simpler bounding rectangle if
      // the rotated rect doesn't work.
      // COZMO-1916
      std::string pointsStr;
      for(auto const& pt : cvPoints) {
        pointsStr += " (" + std::to_string(pt.x)  + "," + std::to_string(pt.y) + ")";
      }
      PRINT_NAMED_ERROR("GetBoundingQuad.CvMinAreaRectFailed.COZMO-1916",
                        "cvPoints.size=%zu%s",
                        cvPoints.size(), pointsStr.c_str());
      
      Rectangle<T> boundingRect(points);
      
      cvRotatedRect.center.x = boundingRect.GetXmid();
      cvRotatedRect.center.y = boundingRect.GetYmid();
      cvRotatedRect.size.width  = boundingRect.GetWidth();
      cvRotatedRect.size.height = boundingRect.GetHeight();
      cvRotatedRect.angle = 0.f;
      
    } // try/catch
    
    cv::Point2f cvQuad[4];
    cvRotatedRect.points(cvQuad);
    Quadrilateral<2,T> boundingQuad(Point<2,T>(static_cast<T>(cvQuad[0].x), static_cast<T>(cvQuad[0].y)),
                                    Point<2,T>(static_cast<T>(cvQuad[1].x), static_cast<T>(cvQuad[1].y)),
                                    Point<2,T>(static_cast<T>(cvQuad[2].x), static_cast<T>(cvQuad[2].y)),
                                    Point<2,T>(static_cast<T>(cvQuad[3].x), static_cast<T>(cvQuad[3].y)));
    
    boundingQuad = boundingQuad.SortCornersClockwise();
    
#else
    CORETECH_THROW("GetBoundingQuad() currently requires OpenCV.");
    Quad2f boundingQuad;
#endif
    
    return boundingQuad;
  }
 
  template<QuadDimType N, typename T>
  T Quadrilateral<N,T>::ComputeArea() const
  {
    using namespace Quad;
    
    static_assert(N==2, "ComputeArea() method only available for 2D quads.");
    
    // Area of the quad is the sum of the area of the two triangles formed
    // by cutting the quad in half with an edge from TopLeft to BottomRight.
    // Note this assumes the quads corners are sorted!
    
    const Triangle<T> triangle1(this->operator[](TopLeft),
                                this->operator[](BottomLeft),
                                this->operator[](BottomRight));
    
    const Triangle<T> triangle2(this->operator[](TopLeft),
                                this->operator[](TopRight),
                                this->operator[](BottomRight));
    
    const T area = triangle1.GetArea() + triangle2.GetArea();
    
    return area;
  } // ComputeArea()
  
  template<QuadDimType N, typename T>
  bool Quadrilateral<N,T>::Contains(const Point<2,T>& point) const
  {
    using namespace Quad;
    
    // Catch invalid usage of this method with a non-2D quad at compile time.
    static_assert(N==2, "Contains() method only available for 2D quads.");
    
    // Using the two triangles in this quad, sharing an edge cutting diagonally
    // across from TopLeft to BottomRight, we can test to see if the point is
    // within either triangle (in which case it is within the Quad).  Note that
    // this assumes the quad is CONVEX!
    
    // Try with first triangle
    std::array<const Point<N,T>*,3> triangle = {{
      &this->operator[](TopLeft),
      &this->operator[](BottomLeft),
      &this->operator[](BottomRight)
    }};
    bool isContained = IsPointWithinTriangle(point, triangle);
    
    if(!isContained) {
      // Try again with the other triangle (swap out the middle vertex)
      triangle[1] = &this->operator[](TopRight);
      isContained = IsPointWithinTriangle(point, triangle);
    }
    
    return isContained;
    
  } // Contains()

  template<QuadDimType N, typename T>
  bool Quadrilateral<N,T>::Contains(const Quadrilateral<2,T>& other) const
  {
    static_assert(N==2, "Quad 'contains' check only valid for 2D quads.");
    
    for(auto & point : other) {
      if(!this->Contains(point)) {
        return false;
      }
    }
    
    return true;
  } // Contains()
  
  template<QuadDimType N, typename T>
  bool Quadrilateral<N,T>::Intersects(const Point2<T>& rayFrom, const Point2<T>& rayTo, Point2<T>* outIntersectionPoint) const
  {
    using namespace Quad;

    const Quadrilateral<N,T>& one = *this;
    kmRay2 thisSegments[4];
    kmVec2 oneTL {one[TopLeft    ].x(), one[TopLeft    ].y()};
    kmVec2 oneTR {one[TopRight   ].x(), one[TopRight   ].y()};
    kmVec2 oneBL {one[BottomLeft ].x(), one[BottomLeft ].y()};
    kmVec2 oneBR {one[BottomRight].x(), one[BottomRight].y()};
    kmRay2FillWithEndpoints(&thisSegments[0], &oneTL, &oneTR);
    kmRay2FillWithEndpoints(&thisSegments[1], &oneTR, &oneBR);
    kmRay2FillWithEndpoints(&thisSegments[2], &oneBR, &oneBL);
    kmRay2FillWithEndpoints(&thisSegments[3], &oneBL, &oneTL);

    kmRay2 testRay;
    kmVec2 from {rayFrom.x(), rayFrom.y()};
    kmVec2 to   {rayTo.x(),   rayTo.y()  };
    kmRay2FillWithEndpoints(&testRay, &from, &to);

    for( int i=0; i<4; ++i) {
      kmVec2 interPoint;
      const kmBool segmentIntersects = kmSegment2WithSegmentIntersection(&thisSegments[i], &testRay, &interPoint);
      if ( segmentIntersects ) {
        if ( outIntersectionPoint ) { (*outIntersectionPoint) = Point2<T>(interPoint.x, interPoint.y); }
        return true;
      }
    }
    
    return false;
  }
  
  template<QuadDimType N, typename T>
  bool Quadrilateral<N,T>::Intersects(const Quadrilateral<2,T>& other) const
  {
    static_assert(N==2, "Quad intersection check only valid for 2D quads.");
    
    for(auto & point : other) {
      if(this->Contains(point)) {
        return true;
      }
    }
    
    for(auto & point : *this) {
      if(other.Contains(point)) {
        return true;
      }
    }
    
    // check if segments intersect to fix this case, where no corner is contained within the other quad
    //              __
    //             |  |
    //           __|__|__
    //          |  |  |  |
    //          |__|__|__|
    //             |  |
    //             |__|
    //
    // rsam: I think this should be enough in addition to the corner checks. There are some optimizations that can
    // be made (for example checking 3 sides should be enough, only corners from the smaller quad could be checked, etc),
    // but I don't foresee this to be a bottleneck atm, and I want to err on the safe side
    // Also, I'm relying on kazMath to not reinvent the wheel at this moment
    {
      using namespace Quad;

      const Quadrilateral<N,T>& one = *this;
      kmRay2 thisSegments[4];
      kmVec2 oneTL {one[TopLeft    ].x(), one[TopLeft    ].y()};
      kmVec2 oneTR {one[TopRight   ].x(), one[TopRight   ].y()};
      kmVec2 oneBL {one[BottomLeft ].x(), one[BottomLeft ].y()};
      kmVec2 oneBR {one[BottomRight].x(), one[BottomRight].y()};
      kmRay2FillWithEndpoints(&thisSegments[0], &oneTL, &oneTR);
      kmRay2FillWithEndpoints(&thisSegments[1], &oneTR, &oneBR);
      kmRay2FillWithEndpoints(&thisSegments[2], &oneBR, &oneBL);
      kmRay2FillWithEndpoints(&thisSegments[3], &oneBL, &oneTL);

      kmRay2 otherSegments[4];
      kmVec2 otherTL {other[TopLeft    ].x(), other[TopLeft    ].y()};
      kmVec2 otherTR {other[TopRight   ].x(), other[TopRight   ].y()};
      kmVec2 otherBL {other[BottomLeft ].x(), other[BottomLeft ].y()};
      kmVec2 otherBR {other[BottomRight].x(), other[BottomRight].y()};
      kmRay2FillWithEndpoints(&otherSegments[0], &otherTL, &otherTR);
      kmRay2FillWithEndpoints(&otherSegments[1], &otherTR, &otherBR);
      kmRay2FillWithEndpoints(&otherSegments[2], &otherBR, &otherBL);
      kmRay2FillWithEndpoints(&otherSegments[3], &otherBL, &otherTL);

      for( int i=0; i<4; ++i) {
        for( int j=0; j<4; ++j) {
          kmVec2 interPoint;
          const kmBool segmentIntersects = kmSegment2WithSegmentIntersection(&thisSegments[i], &otherSegments[j], &interPoint);
          if ( segmentIntersects ) {
            return true;
          }
        }
      }
    }
    
    return false;
    
  } // Intersects()
  
  template<QuadDimType N, typename T>
  bool IsNearlyEqual(const Quadrilateral<N,T> &quad1, const Quadrilateral<N,T> &quad2, const T eps)
  {
    for(s32 iCorner=0; iCorner<4; ++iCorner)
    {
      Quad::CornerName whichCorner = (Quad::CornerName)iCorner;
      if(!IsNearlyEqual(quad1[whichCorner], quad2[whichCorner], eps))
      {
        // Check fails as soon as one corner doesn't match
        return false;
      }
    }
    
    return true;
  }
  
  /*
   template<QuadDimType N, typename T>
  inline const std::vector<Point<N,T> >& Quadrilateral<N,T>::get_corners() const
  {
    return this->corners;
  }*/
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_QUAD_H_
