#ifndef _ANKICORETECH_COMMON_QUAD_H_
#define _ANKICORETECH_COMMON_QUAD_H_

#include <cmath>
#include <vector>

#include "anki/common/basestation/exceptions.h"
#include "anki/common/basestation/math/point.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki {
  
  template<size_t N, typename T>
  class Quadrilateral : public std::vector<Point<N,T> >
  {
  public:
    
    enum CornerName {
      TopLeft     = 0,
      BottomLeft  = 1,
      TopRight    = 2,
      BottomRight = 3
    };
    
    Quadrilateral();
    
    Quadrilateral(const Point<N,T> &cornerTopLeft,
                  const Point<N,T> &cornerBottomLeft,
                  const Point<N,T> &cornerTopRight,
                  const Point<N,T> &cornerBottomRight);
    
    Quadrilateral(const Quadrilateral<N,T>& quad);
    
    void Print() const;
    
    bool operator== (const Quadrilateral<N,T> &quad2) const;
    
    Quadrilateral<N,T>  operator+ (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>  operator- (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>& operator+=(const Quadrilateral<N,T> &quad2);
    Quadrilateral<N,T>& operator-=(const Quadrilateral<N,T> &quad2);
    
    // Force access by enumerated CornerNames:
    const Point<N,T>& operator[] (const CornerName whichCorner) const;
    Point<N,T>&       operator[] (const CornerName whichCorner);
    
    //const std::vector<Point<N,T> >& get_corners() const;
    
    using std::vector<Point<N,T> >::operator=;
    
  protected:
    
    //Point<N,T> corners[4];
    //std::vector<Point<N,T> > corners;
    
  }; // class Quadrilateral<Type>
  
  typedef Quadrilateral<2,float> Quad2f;
  typedef Quadrilateral<3,float> Quad3f;
  
  
#pragma mark --- Quadrilateral Implementations ---
  
  template<size_t N, typename T>
  Quadrilateral<N,T>::Quadrilateral()
  : std::vector<Point<N,T> >(4) //corners(4)
  {
    
  }
  
  template<size_t N, typename T>
  Quadrilateral<N,T>::Quadrilateral(const Point<N,T> &cornerTopLeft,
                                      const Point<N,T> &cornerBottomLeft,
                                      const Point<N,T> &cornerTopRight,
                                      const Point<N,T> &cornerBottomRight)
  : std::vector<Point<N,T> >(4) //corners(4)
  {
    (*this)[TopLeft]     = cornerTopLeft;
    (*this)[TopRight]    = cornerTopRight;
    (*this)[BottomLeft]  = cornerBottomLeft;
    (*this)[BottomRight] = cornerBottomRight;
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
  inline const Point<N,T>& Quadrilateral<N,T>::operator[] (const CornerName whichCorner) const
  {
    //return this->corners[whichCorner];
    return std::vector<Point<N,T> >::operator[](static_cast<int>(whichCorner));
  }

  
  template<size_t N, typename T>
  inline Point<N,T>& Quadrilateral<N,T>::operator[] (const CornerName whichCorner)
  {
    //return this->corners[whichCorner];
    return std::vector<Point<N,T> >::operator[](static_cast<int>(whichCorner));
  }
  
  /*
  template<size_t N, typename T>
  inline const std::vector<Point<N,T> >& Quadrilateral<N,T>::get_corners() const
  {
    return this->corners;
  }*/
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_QUAD_H_
