/**
 * File: quad.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Defines a general N-dimensional Quadrilateral class for holding
 *              four points in arbitrary positions.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_COMMON_QUAD_H_
#define _ANKICORETECH_COMMON_QUAD_H_

#include "coretech/common/shared/math/rect.h"

namespace Anki {

  using QuadDimType = size_t;
  
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
  
  template<QuadDimType N, typename T>
  class Quadrilateral : public std::array<Point<N,T>, 4>
  {
  public:
    
    Quadrilateral();
    
    Quadrilateral(const Point<N,T> &cornerTopLeft,
                  const Point<N,T> &cornerBottomLeft,
                  const Point<N,T> &cornerTopRight,
                  const Point<N,T> &cornerBottomRight);
    
    Quadrilateral(std::initializer_list<Point<N,T> >& points);
    
    Quadrilateral(const Quadrilateral<N,T>& quad);

    Quadrilateral(const Rectangle<T>& rect);
    
    // Construct from quad living in one dimension higher. Last dimension
    // simply gets dropped. For example, this allows construction of a 2D quad
    // from a 3D quad, by simply using the (x,y) coordinates and ignoring z.
    Quadrilateral(const Quadrilateral<N+1,T>& quad);
    
    void Print() const;
    
    bool operator== (const Quadrilateral<N,T> &quad2) const;
    
    // Math operations
    Quadrilateral<N,T>  operator+ (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>  operator- (const Quadrilateral<N,T> &quad2) const;
    Quadrilateral<N,T>& operator+=(const Quadrilateral<N,T> &quad2);
    Quadrilateral<N,T>& operator-=(const Quadrilateral<N,T> &quad2);

    Quadrilateral<N,T>& operator*=(const T value);
    Quadrilateral<N,T>& operator+=(const T value);

    Quadrilateral<N,T>& operator+=(const Point<N,T> &point);
    Quadrilateral<N,T>& operator-=(const Point<N,T> &point);
    
    // Scale around centroid:
    Quadrilateral<N,T>  GetScaled(const T scaleFactor) const;
    Quadrilateral<N,T>& Scale(const T scaleFactor); // in place
    
    // Compute the centroid of the four points
    Point<N,T> ComputeCentroid(void) const;
    
    // Compute the area of a 2D quadrilateral
    T ComputeArea(void) const;
    
    // Get min/max coordinates (e.g. for bounding box)
    T GetMinX(void) const;
    T GetMinY(void) const;
    T GetMaxX(void) const;
    T GetMaxY(void) const;
    
    const Point<N,T>& GetTopLeft() const;
    const Point<N,T>& GetBottomLeft() const;
    const Point<N,T>& GetTopRight() const;
    const Point<N,T>& GetBottomRight() const;
    
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
    
    // Determine if a 2D point is contained within a 2D quad.
    // NOTE: This method is ONLY available for 2D quads!
    // ASSUMPTIONS:
    //  - the quad is *convex*
    //  - the corners are ordered such that TopLeft/BottomRight are opposite
    //    one antoher and BottomLeft/TopRight are opposite one another
    bool Contains(const Point<2,T>& point) const;
    
    // Returns true if this quad contains all the corners from the other quad (fully contains the other convex quad)
    bool Contains(const Quadrilateral<2,T>& other) const;

    // Returns true if the two quads intersect.
    // Only valid for 2D quads. Assumes quads are convex and checks whether
    // any corners of one quad are inside the other one, as well as whether any segments collide
    bool Intersects(const Quadrilateral<2,T>& other) const;
    bool Intersects(const Point2<T>& rayFrom, const Point2<T>& rayTo, Point2<T>* outIntersectionPoint=nullptr) const;
    
  protected:
    
    //Point<N,T> corners[4];
    //std::vector<Point<N,T> > corners;
    
  }; // class Quadrilateral<Type>
  
  typedef Quadrilateral<2,float> Quad2f;
  typedef Quadrilateral<3,float> Quad3f;
  
  // Find the minimum bounding quad for a set of points. Note that this will
  // actually be a rotated rectangle (i.e. not a completely general
  // quadrilateral. It also only works for N==2 for now.
  // TODO: Take in more generic STL containers of points beyond just std::vector?
  // TODO: Make a 3D version too, with a projector?
  template<typename T>
  Quadrilateral<2,T> GetBoundingQuad(std::vector<Point<2,T> >& points);

  template<QuadDimType N, typename T>
  bool IsNearlyEqual(const Quadrilateral<N,T> &quad1, const Quadrilateral<N,T> &quad2, const T eps);
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_QUAD_H_
