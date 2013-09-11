#include "gtest/gtest.h"

#include "anki/general.h"
#include "anki/math/matrix.h"
#include <iostream>

// For test debug printing
#define DEBUG_TEST_MATRIX


#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,FLOATING_POINT_COMPARISON_TOLERANCE)



using namespace std;
using namespace Anki;

TEST(TestMatrix, MatrixConstructorsAndAccessors)
{
  
  /////////////// Square matrix /////////////////////
  float initValueA = 0.f;
  Matrix<float> A(3,3,initValueA);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A (before): \n" << A << "\n";
#endif
 
  ASSERT_EQ(A.numRows(), 3);
  ASSERT_EQ(A.numCols(), 3);
  
  // Test accessors
  // Make sure matrix A is all zeros
  for (int r=0; r< A.numRows(); ++r) {
    for (int c=0; c<A.numCols(); ++c) {
      ASSERT_EQ(A(r,c), initValueA);
    }
  }
  
  // Assign some new values
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;

  // Test newly assigned values
  ASSERT_EQ(A(0,0), 1.f);
  ASSERT_EQ(A(1,1), 2.f);
  ASSERT_EQ(A(2,2), 3.f);
  
  // Make sure others are still zero
  ASSERT_EQ(A(0,1), initValueA);
  ASSERT_EQ(A(0,2), initValueA);
  ASSERT_EQ(A(1,0), initValueA);
  ASSERT_EQ(A(1,2), initValueA);
  ASSERT_EQ(A(2,0), initValueA);
  ASSERT_EQ(A(2,1), initValueA);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A (after): \n" << A << "\n";
#endif
  
  
  // Try to access invalid inded
  ASSERT_ANY_THROW(A(3,0));
  
  
  /////////////  Non-square matrix ///////////////////
  float initValueB = 3.14f;
  Matrix<float> B(3,4,initValueB);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix B (before): \n" << B << "\n";
#endif
  
  ASSERT_EQ(B.numRows(), 3);
  ASSERT_EQ(B.numCols(), 4);
  
  // Test accessors
  // Make sure matrix B initial values are correct
  for (int r=0; r< B.numRows(); ++r) {
    for (int c=0; c<B.numCols(); ++c) {
      ASSERT_EQ(B(r,c), initValueB);
    }
  }
  
  // Assign some new values
  B(0,1) = 1.23f;
  B(1,2) = 3.45f;
  B(2,3) = 6.78f;
  
  // Test newly assigned values
  ASSERT_EQ(B(0,1), 1.23f);
  ASSERT_EQ(B(1,2), 3.45f);
  ASSERT_EQ(B(2,3), 6.78f);
  
  // Make sure others are still zero
  ASSERT_EQ(B(0,0), initValueB);
  ASSERT_EQ(B(0,2), initValueB);
  ASSERT_EQ(B(0,3), initValueB);
  ASSERT_EQ(B(1,0), initValueB);
  ASSERT_EQ(B(1,1), initValueB);
  ASSERT_EQ(B(1,3), initValueB);
  ASSERT_EQ(B(2,0), initValueB);
  ASSERT_EQ(B(2,1), initValueB);
  

#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix B (after): \n" << B << "\n";
#endif
  
  /////////////// Empty matrix ///////////////
  ASSERT_ANY_THROW(Matrix<float> C(0,3,0));
  ASSERT_ANY_THROW(Matrix<float> C(1,0,0));
}


TEST(TestMatrix, SmallMatrixConstructorsAndAccessors)
{
  ///////// SmallMatrix //////////
  Matrix_2x2f M;

#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix M (before): \n" << M << "\n";
#endif
  
  // Test accessors
  // Make sure matrix B initial values are correct
  for (int r=0; r< M.numRows(); ++r) {
    for (int c=0; c<M.numCols(); ++c) {
      ASSERT_EQ(M(r,c), 0.f);
    }
  }
  
  // Assign new values
  M(0,0) = 1.f;
  M(0,1) = 2.f;
  
  // Test values
  ASSERT_EQ(M(0,0), 1.f);
  ASSERT_EQ(M(0,1), 2.f);
  
  ASSERT_EQ(M(1,0), 0.f);
  ASSERT_EQ(M(1,1), 0.f);
  
  
  // Assign more new values
  M(1,0) = 3.f;
  M(1,1) = 4.f;

  ASSERT_EQ(M(1,0), 3.f);
  ASSERT_EQ(M(1,1), 4.f);
  
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix M (after): \n" << M << "\n";
#endif
  
  // Try to access invalid index
  ASSERT_ANY_THROW(M(2,2));
  
  
  
  
  // Contstructor with initial values
  float N_init_array[] = {0, 1, 2,
                          3, 4, 5,
                          6, 7, 8};
  Matrix_3x3f N(N_init_array);
  
  ASSERT_EQ(N(0,0),0);  ASSERT_EQ(N(0,0),0);  
}

// Normal matrix multiplication
TEST(TestMatrix, MatrixMultiplication1)
{
  Matrix<float> A(3,3,0.f), B(3,3,0.f);
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;
  
  B(0,0) = 10.f;
  B(1,1) = 10.f;
  B(2,2) = 10.f;
  
  Matrix<float> C;   // TODO: Should matrix operator= throw if dimensions change?
  C = A*B;
  
  ASSERT_EQ(C.numRows(), 3);
  ASSERT_EQ(C.numCols(), 3);
  
  ASSERT_EQ(C(0,0), 10.f);
  ASSERT_EQ(C(1,1), 20.f);
  ASSERT_EQ(C(2,2), 30.f);
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix C = A*B: \n" << C << "\n";
#endif
}

// Multiplying 3x3 matrix by a 4x3 one.
// Should throw exception!
TEST(TestMatrix, MatrixMultiplication2)
{
  Matrix<float> A(3,3,0.f), B(4,3,0.f);
  Matrix<float> C;
  
  ASSERT_ANY_THROW(C = A*B);
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix C = A*B: \n" << C << "\n";
#endif
}



TEST(TestMatrix, SmallMatrixMultiplication)
{
  Matrix_3x3f A;
  Matrix_3x4f B;
  
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;
  
  B(0,0) = 10.f;
  B(1,1) = 10.f;
  B(2,2) = 10.f;

  Matrix_3x4f C;
  
  // Normal SmallMatrix multiplication
  C = A*B;
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix A: \n" << A << "\n";
  cout << "SmallMatrix B: \n" << B << "\n";
  cout << "SmallMatrix C = A*B: \n" << C << "\n";
#endif
  
  ASSERT_EQ(C.numRows(), 3);
  ASSERT_EQ(C.numCols(), 4);
  
  ASSERT_NEAR_EQ(C(0,0), 10.f);  ASSERT_NEAR_EQ(C(0,1), 0.f);    ASSERT_NEAR_EQ(C(0,2), 0.f);    ASSERT_NEAR_EQ(C(0,3), 0.f);
  ASSERT_NEAR_EQ(C(1,0), 0.f);   ASSERT_NEAR_EQ(C(1,1), 20.f);   ASSERT_NEAR_EQ(C(1,2), 0.f);    ASSERT_NEAR_EQ(C(1,3), 0.f);
  ASSERT_NEAR_EQ(C(2,0), 0.f);   ASSERT_NEAR_EQ(C(2,1), 0.f);    ASSERT_NEAR_EQ(C(2,2), 30.f);   ASSERT_NEAR_EQ(C(2,3), 0.f);
  
  
  // This results in compile time error if using OpenCV
  // Invalid multiplication
  //ASSERT_ANY_THROW(C = B*A);
}


TEST(TestMatrix, MatrixInverse)
{
  // Matrix
  Matrix<float> A(3,3,0.f);
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;
  
  Matrix<float> Ainv, I;
  
  Ainv = A.getInverse();
  I = A*Ainv;
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix Ainv: \n" << Ainv << "\n";
  cout << "I = A * Ainv: \n" << I << "\n";
#endif
  
  // Check that I is identity
  ASSERT_NEAR_EQ(I(0,0), 1.f); ASSERT_NEAR_EQ(I(0,1), 0.f); ASSERT_NEAR_EQ(I(0,2), 0.f);
  ASSERT_NEAR_EQ(I(1,0), 0.f); ASSERT_NEAR_EQ(I(1,1), 1.f); ASSERT_NEAR_EQ(I(1,2), 0.f);
  ASSERT_NEAR_EQ(I(2,0), 0.f); ASSERT_NEAR_EQ(I(2,1), 0.f); ASSERT_NEAR_EQ(I(2,2), 1.f);
  
  
  A.Invert();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "A inverted: \n" << A << "\n";
#endif
  
  // Check that A is now the same as Ainv
  for (int r=0; r< A.numRows(); ++r) {
    for (int c=0; c<A.numCols(); ++c) {
      ASSERT_EQ(A(r,c), Ainv(r,c));
    }
  }
  
  
  // Invert non-square matrix
  Matrix<float> B(3,4,0.f);
  
  ASSERT_ANY_THROW(B.getInverse());
  ASSERT_ANY_THROW(B.Invert());
}


TEST(TestMatrix, SmallMatrixInverse)
{
  float initValsA[] = {50, 0, 10,
                       0, 100, 0,
                       0, 0, 150};
  Matrix_3x3f A(initValsA);
  Matrix_3x3f Ainv, I;
  
  Ainv = A.getInverse();
  I = A*Ainv;
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix Ainv: \n" << Ainv << "\n";
  cout << "I = A * Ainv: \n" << I << "\n";
#endif
  
  // Check that I is identity
  ASSERT_NEAR_EQ(I(0,0), 1.f); ASSERT_NEAR_EQ(I(0,1), 0.f); ASSERT_NEAR_EQ(I(0,2), 0.f);
  ASSERT_NEAR_EQ(I(1,0), 0.f); ASSERT_NEAR_EQ(I(1,1), 1.f); ASSERT_NEAR_EQ(I(1,2), 0.f);
  ASSERT_NEAR_EQ(I(2,0), 0.f); ASSERT_NEAR_EQ(I(2,1), 0.f); ASSERT_NEAR_EQ(I(2,2), 1.f);
  
  
  
  A.Invert();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "A inverted: \n" << A << "\n";
#endif
  
  // Check that A is now the same as Ainv
  for (int r=0; r< A.numRows(); ++r) {
    for (int c=0; c<A.numCols(); ++c) {
      ASSERT_EQ(A(r,c), Ainv(r,c));
    }
  }
  

  
  // This results in compile time error if using OpenCV
  
//  // Invert non-square matrix
//  float initValsB[] = {1, 0, 0, 0,
//                       0, 1, 0, 0,
//                       0, 0, 1, 0};
//  Matrix_3x4f B(initValsB);
//  
//  ASSERT_ANY_THROW(B.getInverse());
//  ASSERT_ANY_THROW(B.Invert());

  
}


TEST(TestMatrix, MatrixTranspose)
{
  Matrix<float> A(3,3,0.f);
  A(0,0) = 1.f;    A(0,1) = 2.f;     A(0,2) = 3.f;
  A(1,0) = 4.f;    A(1,1) = 5.f;     A(1,2) = 6.f;
  A(2,0) = 7.f;    A(2,1) = 8.f;     A(2,2) = 9.f;
  
  Matrix<float> A_t;
  A_t = A.getTranpose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix A_transpose: \n" << A_t << "\n";
#endif

  // Check that I is identity
  ASSERT_NEAR_EQ(A_t(0,0), 1.f); ASSERT_NEAR_EQ(A_t(0,1), 4.f); ASSERT_NEAR_EQ(A_t(0,2), 7.f);
  ASSERT_NEAR_EQ(A_t(1,0), 2.f); ASSERT_NEAR_EQ(A_t(1,1), 5.f); ASSERT_NEAR_EQ(A_t(1,2), 8.f);
  ASSERT_NEAR_EQ(A_t(2,0), 3.f); ASSERT_NEAR_EQ(A_t(2,1), 6.f); ASSERT_NEAR_EQ(A_t(2,2), 9.f);
  
  A.Transpose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "A transposed: \n" << A << "\n";
#endif
  
  // Check that A is now the same as A_t
  for (int r=0; r< A.numRows(); ++r) {
    for (int c=0; c<A.numCols(); ++c) {
      ASSERT_EQ(A(r,c), A_t(r,c));
    }
  }
  
  
  // Transpose non-square matrix
  Matrix<float> B(3,4);
  B(0,0) = 1.f;   B(0,1) = 2.f;   B(0,2) = 3.f;   B(0,3) = 4.f;
  B(1,0) = 5.f;   B(1,1) = 6.f;   B(1,2) = 7.f;   B(1,3) = 8.f;
  B(2,0) = 9.f;   B(2,1) = 10.f;  B(2,2) = 11.f;  B(2,3) = 12.f;
  
  Matrix<float> B_t = B.getTranpose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix B_transpose: \n" << B_t << "\n";
#endif
  
  ASSERT_EQ(B_t.numRows(), B.numCols());
  ASSERT_EQ(B_t.numCols(), B.numRows());
  
  ASSERT_NEAR_EQ(B_t(0,0), 1.f); ASSERT_NEAR_EQ(B_t(0,1), 5.f); ASSERT_NEAR_EQ(B_t(0,2), 9.f);
  ASSERT_NEAR_EQ(B_t(1,0), 2.f); ASSERT_NEAR_EQ(B_t(1,1), 6.f); ASSERT_NEAR_EQ(B_t(1,2), 10.f);
  ASSERT_NEAR_EQ(B_t(2,0), 3.f); ASSERT_NEAR_EQ(B_t(2,1), 7.f); ASSERT_NEAR_EQ(B_t(2,2), 11.f);
  ASSERT_NEAR_EQ(B_t(3,0), 4.f); ASSERT_NEAR_EQ(B_t(3,1), 8.f); ASSERT_NEAR_EQ(B_t(3,2), 12.f);
  
  B.Transpose();
  
  // Check that B is now the same as B_t
  for (int r=0; r< B.numRows(); ++r) {
    for (int c=0; c<B.numCols(); ++c) {
      ASSERT_EQ(B(r,c), B_t(r,c));
    }
  }
}



TEST(TestMatrix, SmallMatrixTranspose)
{
  float initValsA[] = {1, 2, 3,
                       4, 5, 6,
                       7, 8, 9};
  Matrix_3x3f A(initValsA);
  
  Matrix_3x3f A_t;
  A_t = A.getTranspose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix A: \n" << A << "\n";
  cout << "SmallMatrix A_transpose: \n" << A_t << "\n";
#endif
  
  // Check that I is identity
  ASSERT_NEAR_EQ(A_t(0,0), 1.f); ASSERT_NEAR_EQ(A_t(0,1), 4.f); ASSERT_NEAR_EQ(A_t(0,2), 7.f);
  ASSERT_NEAR_EQ(A_t(1,0), 2.f); ASSERT_NEAR_EQ(A_t(1,1), 5.f); ASSERT_NEAR_EQ(A_t(1,2), 8.f);
  ASSERT_NEAR_EQ(A_t(2,0), 3.f); ASSERT_NEAR_EQ(A_t(2,1), 6.f); ASSERT_NEAR_EQ(A_t(2,2), 9.f);
  
  
  
  // Transpose non-square matrix
  float initValsB[] = {1, 2, 3, 4,
                       5, 6, 7, 8,
                       9, 10, 11, 12};
  Matrix_3x4f B(initValsB);
  
  SmallMatrix<float,4,3> B_t = B.getTranspose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix B: \n" << B << "\n";
  cout << "SmallMatrix B_transpose: \n" << B_t << "\n";
#endif
  
  ASSERT_EQ(B_t.numRows(), B.numCols());
  ASSERT_EQ(B_t.numCols(), B.numRows());
  
  ASSERT_NEAR_EQ(B_t(0,0), 1.f); ASSERT_NEAR_EQ(B_t(0,1), 5.f); ASSERT_NEAR_EQ(B_t(0,2), 9.f);
  ASSERT_NEAR_EQ(B_t(1,0), 2.f); ASSERT_NEAR_EQ(B_t(1,1), 6.f); ASSERT_NEAR_EQ(B_t(1,2), 10.f);
  ASSERT_NEAR_EQ(B_t(2,0), 3.f); ASSERT_NEAR_EQ(B_t(2,1), 7.f); ASSERT_NEAR_EQ(B_t(2,2), 11.f);
  ASSERT_NEAR_EQ(B_t(3,0), 4.f); ASSERT_NEAR_EQ(B_t(3,1), 8.f); ASSERT_NEAR_EQ(B_t(3,2), 12.f);

}
