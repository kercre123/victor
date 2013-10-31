#ifndef _ANKICORETECH_COMMON_QUAD_H_
#define _ANKICORETECH_COMMON_QUAD_H_

#include <cmath>
#include <array>

#include "anki/common/basestation/exceptions.h"
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
    
  } // namespace Quad
  
  template<size_t N, typename T>
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
    
    // Scale around center:
    Quadrilateral<N,T>  operator* (const T scaleFactor) const;
    Quadrilateral<N,T>& operator*=(const T scaleFactor);
    
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
    
  protected:
    
    //Point<N,T> corners[4];
    //std::vector<Point<N,T> > corners;
    
  }; // class Quadrilateral<Type>
  
  typedef Quadrilateral<2,float> Quad2f;
  typedef Quadrilateral<3,float> Quad3f;
  
  
#pragma mark --- Quadrilateral Implementations ---
  
  template<size_t N, typename T>
  Quadrilateral<N,T>::Quadrilateral()
  {
    
  }
  
  template<size_t N, typename T>
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
  
  template<size_t N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Quadrilateral<N,T>& quad)
  : std::array<Point<N,T>,4>(quad)
  {
  
  }
  
  template<size_t N, typename T>
  inline Quadrilateral<N,T> Quadrilateral<N,T>::operator+
  (const Quadrilateral<N,T> &quad2) const
  {
    Quadrilateral<N,T> newQuad(*this);
    newQuad += quad2;
    return newQuad;
  }
  
  template<size_t N, typename T>
  inline Quadrilateral<N,T> Quadrilateral<N,T>::operator-
  (const Quadrilateral<N,T> &quad2) const
  {
    Quadrilateral<N,T> newQuad(*this);
    newQuad -= quad2;
    return newQuad;
  }
  
  template<size_t N, typename T>
  inline Quadrilateral<N,T>& Quadrilateral<N,T>::operator+= (const Quadrilateral<N,T> &quad2)
  {
    for(int i=0; i<4; ++i) {
      (*this)[i] += quad2[i];
    }
    return *this;
  }
  
  template<size_t N, typename T>
  inline Quadrilateral<N,T>& Quadrilateral<N,T>::operator-= (const Quadrilateral<N,T> &quad2)
  {
    for(int i=0; i<4; ++i) {
      (*this)[i] -= quad2[i];
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Quadrilateral<N,T>  Quadrilateral<N,T>::operator* (const T scaleFactor) const
  {
    Quadrilateral<N,T> scaledQuad(*this);
    scaledQuad *= scaleFactor;
    return scaledQuad;
  }
  
  template<size_t N, typename T>
  inline T Quadrilateral<N,T>::get_minX(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].x(),
               MIN((*this)[TopRight].x(),
                   MIN((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<size_t N, typename T>
  inline T Quadrilateral<N,T>::get_minY(void) const
  {
    using namespace Quad;
    return MIN((*this)[TopLeft].y(),
               MIN((*this)[TopRight].y(),
                   MIN((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<size_t N, typename T>
  inline T Quadrilateral<N,T>::get_maxX(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].x(),
               MAX((*this)[TopRight].x(),
                   MAX((*this)[BottomLeft].x(), (*this)[BottomRight].x())));
  }
  
  template<size_t N, typename T>
  inline T Quadrilateral<N,T>::get_maxY(void) const
  {
    using namespace Quad;
    return MAX((*this)[TopLeft].y(),
               MAX((*this)[TopRight].y(),
                   MAX((*this)[BottomLeft].y(), (*this)[BottomRight].y())));
  }
  
  template<size_t N, typename T>
  Point<N,T> Quadrilateral<N,T>::computeCentroid(void) const
  {
    using namespace Quad;
    
    Point<N,T> centroid((*this)[TopLeft]);
    centroid += (*this)[TopRight];
    centroid += (*this)[BottomLeft];
    centroid += (*this)[BottomRight];
    centroid *= 0.25f; // This might do weird things if T is not float/double

    return centroid;
  }
  
  template<size_t N, typename T>
  Quadrilateral<N,T>& Quadrilateral<N,T>::operator*=(const T scaleFactor)
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
  
  template<size_t N, typename T>
  inline const Point<N,T>& Quadrilateral<N,T>::operator[] (const Quad::CornerName whichCorner) const
  {
    //return this->corners[whichCorner];
    return std::array<Point<N,T>,4>::operator[](static_cast<int>(whichCorner));
  }

  
  template<size_t N, typename T>
  inline Point<N,T>& Quadrilateral<N,T>::operator[] (const Quad::CornerName whichCorner)
  {
    //return this->corners[whichCorner];
    return std::array<Point<N,T>,4>::operator[](static_cast<int>(whichCorner));
  }
  
  /*
  template<size_t N, typename T>
  inline const std::vector<Point<N,T> >& Quadrilateral<N,T>::get_corners() const
  {
    return this->corners;
  }*/
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_QUAD_H_
