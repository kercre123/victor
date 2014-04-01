#include "anki/common/basestation/math/quad.h"

namespace Anki {
  
  namespace Quad {
    
    // prefix operator (++cname)
    CornerName& operator++(CornerName& cname) {
      cname = (cname < NumCorners) ? static_cast<CornerName>( static_cast<int>(cname) + 1 ) : NumCorners;
      return cname;
    }

    // postfix operator (cname++)
    CornerName operator++(CornerName& cname, int) {
      CornerName newCname = cname;
      ++newCname;
      return newCname;
    }
  }
  
  void test(void)
  {
    Quad2f q;
    
    q[Quad::TopLeft] = Point2f(.1f, .3f);
  }
  
} // namespace Anki