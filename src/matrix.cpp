#include "anki/math/matrix.h"

namespace Anki {
  
  void testMatrixInstantiation(void)
  {
    Matrix<float> A(3,3), B(3,4);
    A(0,0) = 1.f;
    A(1,1) = 2.f;
    A(2,2) = 3.f;
    
    B(1,2) = 4.f;
    B(2,3) = 5.f;
    B(3,1) = 6.f;
    
    Matrix<float> C( A*B );
    
    Matrix<float> D( C.Inverse() );
    
    Matrix<float> E( D.Tranpose() );
  }
  
} // namespace Anki