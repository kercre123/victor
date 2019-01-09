#include "coretech/common/engine/math/quad_impl.h"

namespace Anki {

  namespace Quad {

    // prefix operator (++cname)
    CornerName& operator++(CornerName& cname) {
      cname = (cname < NumCorners) ? static_cast<CornerName>( static_cast<int>(cname) + 1 ) : NumCorners;
      return cname;
    }

    // postfix operator (cname++)
    CornerName operator++(CornerName& cname, int) {
      CornerName cnameToReturn = cname; // make a copy to return before we increment
      ++cname;
      return cnameToReturn;
    }

    CornerName operator+(CornerName& cname, int value)
    {
      const int newValue = static_cast<int>(cname) + value;

      const CornerName newCname = (newValue < static_cast<int>(NumCorners) ?
                                   static_cast<CornerName>(newValue) :
                                   NumCorners);

      return newCname;
    }

  }

  void test(void)
  {
    Quad2f q;

    q[Quad::TopLeft] = Point2f(.1f, .3f);

    q.SortCornersClockwise();

    //Point2f n;
    //q.SortCornersClockwise(n); // Should fail: triggers static assert because N==2

    Quad3f q3;
    Point3f n3;
    //q3.SortCornersClockwise(); // Should fail: triggers static assert because N!=2
    q3.SortCornersClockwise(n3);
  }

} // namespace Anki
