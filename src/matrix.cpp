#include "anki/math/matrix.h"

namespace Anki {
  
  void testMatrixInstantiation(void)
  {
    // Matrix:
    Matrix<float> A(3,3), B(3,4);
    A(0,0) = 1.f;
    A(1,1) = 2.f;
    A(2,2) = 3.f;
    
    B(1,2) = 4.f;
    B(2,3) = 5.f;
    B(3,1) = 6.f;
    
    Matrix<float> C( A*B );
    
    Matrix<float> D( C.getInverse() );
    
    Matrix<float> E( D.getTranpose() );
    
    // Small Matrix:
    Matrix_2x2f M;
    M(0,0) = 1.f;
    M(1,2) = 2.f;
    M(2,1) = 3.f;
    M(2,2) = 4.f;
  }
  
} // namespace Anki