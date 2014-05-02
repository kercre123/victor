#ifndef _ANKICORETECH_COMMON_QUAD_H_
#define _ANKICORETECH_COMMON_QUAD_H_

#include <cmath>
#include <array>

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/linearAlgebra_impl.h"
#include "anki/common/basestation/math/point.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki {

  namespace Quad {
    
    enum CornerName {
      FirstCorner = 0,
      TopLeft     = 0,
      BottomLeft  = 1,
      TopRight    = 2,
      BottomRight = 3,
      NumCorners  = 4
    };
    
    // prefix operator (++cname)
    CornerName& operator++(CornerName& cname);

    // postfix operator (cname++)
    CornerName operator++(CornerName& cname, int);
    
    CornerName operator+(CornerName& cname, int value);
    
  } // namespace Quad
  
  typedef size_t QuadDimType;
  
  template<QuadDimType N, typename T>
  class Quadrilateral : public std::array<Point<N,T>, 4>
  {
  public:
    
    Quadrilateral();
    
    Quadrilateral(const Point<N,T> &cornerTopLeft,
                  const Point<N,T> &cornerBottomLeft,
                  const Point<N,T> &cornerTopRight,
                  const Point<N,T> &cornerBottomRight);
    
    Quadrilateral(const Quadrilateral<N,T>& quad);
    
    void Print() const;
    
    bool operator== (const Quadrilateral<N,T> &quad2) const;
    
    // Math operations
    Quadrilateral<N,T>  operator+ (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>  operator- (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>& operator+=(const Quadrilateral<N,T> &quad2);
    Quadrilateral<N,T>& operator-=(const Quadrilateral<N,T> &quad2);
    Quadrilateral<N,T>& operator*=(const T value);

    Quadrilateral<N,T>& operator+=(const Point<N,T> &point);
    Quadrilateral<N,T>& operator-=(const Point<N,T> &point);
    
    // Scale around centroid:
    Quadrilateral<N,T>  getScaled(const T scaleFactor) const;
    Quadrilateral<N,T>& scale(const T scaleFactor); // in place
    
    // Compute the centroid of the four points
    Point<N,T> computeCentroid(void) const;
    
    // Get min/max coordinates (e.g. for bounding box)
    T get_minX(void) const;
    T get_minY(void) const;
    T get_maxX(void) const;
    T get_maxY(void) const;
    
    // Force access by enumerated CornerNames:
    const Point<N,T>& operator[] (const Quad::CornerName whichCorner) const;
    Point<N,T>&       operator[] (const Quad::CornerName whichCorner);
    
    //const std::vector<Point<N,T> >& get_corners() const;
    
    using std::array<Point<N,T>, 4>::operator=;
    
    // WARNING:
    // The width and height of a floating point Rectangle is different than that of an integer rectangle.
    //template<typename OutType> Rectangle<OutType> ComputeBoundingRectangle() const;
    
    // Sorts the quad's corners, so they are clockwise around the centroid, with
    // respect to the given plane normal.
    // Warning: This may give weird results for non-convex quadrilaterals
    Quadrilateral<N,T> SortCornersClockwise(const Point<N,T>& aroundNormal) const; // in place

    // Special version for 2D quads: no normal needed, since they exist in a 2D
    // plane by definition.
    Quadrilateral<2,T> SortCornersClockwise(void) const;

  protected:
    
    //Point<N,T> corners[4];
    //std::vector<Point<N,T> > corners;
    
  }; // class Quadrilateral<Type>
  
  typedef Quadrilateral<2,float> Quad2f;
  typedef Quadrilateral<3,float> Quad3f;
  
  
#pragma mark --- Quadrilateral Implementations ---
  
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
  Quadrilateral<N,T>::Quadrilateral(const Quadrilateral<N,T>& quad)
  : std::array<Point<N,T>,4>(quad)
  {
  
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
  inline Quadrilateral<N,T>& Quadrilateral<N,T>::operator+= (const Quadrilateral<N,T> &quad2)
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
  Quadrilateral<N,T>  Quadrilateral<N,T>::getScaled(const T scaleFactor) const
  {
    Quadrilateral<N,T> scaledQuad(*this);
    scaledQuad *= scaleFactor;
    return scaledQuad;
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::get_minX(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].x(),
               MIN((*this)[TopRight].x(),
                   MIN((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::get_minY(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].y(),
               MIN((*this)[TopRight].y(),
                   MIN((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::get_maxX(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].x(),
               MAX((*this)[TopRight].x(),
                   MAX((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<QuadDimType N, typename T>
  inline T Quadrilateral<N,T>::get_maxY(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].y(),
               MAX((*this)[TopRight].y(),
                   MAX((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<QuadDimType N, typename T>
  Point<N,T> Quadrilateral<N,T>::computeCentroid(void) const
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
  Quadrilateral<N,T>& Quadrilateral<N,T>::scale(const T scaleFactor)
  {
    using namespace Quad;
    
    Point<N,T> centroid = this->computeCentroid();
    
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
  
  bool sortHelper(const std::pair<f32,QuadDimType>& pair1,
                  const std::pair<f32,QuadDimType>& pair2)
  {
    return pair1.first < pair2.first;
  }
  
  
  template<QuadDimType N, typename T>
  Quadrilateral<2,T> Quadrilateral<N,T>::SortCornersClockwise() const
  {
    // Catch invalid usage of this method with a non-2D quad at compile time.
    static_assert(N==2, "No normal should be supplied when sorting a 2D Quadrilateral's corners.");

    using namespace Quad;
    
    std::array<std::pair<f32,Quad::CornerName>,4> angleIndexPairs;
    
    Point<N,T> center = this->computeCentroid();
    
    for(CornerName i=FirstCorner; i<NumCorners; i++) {
      Point<N,T> corner((*this)[i]);
      corner -= center;
      
      angleIndexPairs[i].first  = atan2f(static_cast<f32>(corner.y()), static_cast<f32>(corner.x()));
      angleIndexPairs[i].second = i;
    }
    
    // Sort by angle, bringing corner index along for the ride
    std::sort(angleIndexPairs.begin(), angleIndexPairs.end(), sortHelper);
    
    // Re-order the corners using the sorted indices
    const Quadrilateral<2,T> sortedQuad((*this)[angleIndexPairs[0].second],
                                        (*this)[angleIndexPairs[1].second],
                                        (*this)[angleIndexPairs[2].second],
                                        (*this)[angleIndexPairs[3].second]);
    
    return sortedQuad;
    
  } // SortCornersClockWise(), Specialized for N==2
  
  
  template<QuadDimType N, typename T>
  Quadrilateral<N,T> Quadrilateral<N,T>::SortCornersClockwise(const Point<N,T>& aroundNormal) const
  {
    // Catch invalid usage of this method with a 2D quad at compile time.
    static_assert(N != 2, "A normal must be supplied when sorting an ND Quadrilateral's corners.");
    
    using namespace Quad;
    
    Point<N,T> unitNormal(aroundNormal);
    unitNormal.makeUnitLength();

    Point<N,T> center = this->computeCentroid();
    SmallSquareMatrix<N,T> P = GetProjectionOperator(unitNormal);
    center = P * center;

    // Need to establish an arbitrary coordinate system in the plane.
    // Use the first corner as the first coordinate axis.
    Point<N,T> unitU(P * (*this)[Quad::FirstCorner]);
    unitU -= center;
    unitU.makeUnitLength();
    
    // The cross product of the first coordinate axis vector with the normal
    // to define the (orthogonal) other coordinate axis vector
    Point<N,T> unitV( cross(unitU, unitNormal) );
    
    // By definition, the first corner's angle is 0.  Sort the other three
    // relative to it, using the plane's coordinate system we just established
    std::array<std::pair<f32,CornerName>,3> angleIndexPairs;
    for(s32 i=0; i<3; i++)
    {
      CornerName index = static_cast<CornerName>(i+1);
      Point<N,T> corner( P * (*this)[index] );
      corner -= center;
      
      const T u = dot(corner, unitU);
      const T v = dot(corner, unitV);
      
      angleIndexPairs[i].first = atan2f(static_cast<f32>(v), static_cast<f32>(u));
      angleIndexPairs[i].second = index;
    }
    
    // Sort by angle, taking the corner index along for the ride
    std::sort(angleIndexPairs.begin(), angleIndexPairs.end(), sortHelper);
    
    const Quadrilateral<N,T> sortedQuad((*this)[Quad::FirstCorner],
                                        (*this)[angleIndexPairs[0].second],
                                        (*this)[angleIndexPairs[1].second],
                                        (*this)[angleIndexPairs[2].second]);
              
    return sortedQuad;
  }
  
  /*
  template<QuadDimType N, typename T>
  inline const std::vector<Point<N,T> >& Quadrilateral<N,T>::get_corners() const
  {
    return this->corners;
  }*/
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_QUAD_H_
