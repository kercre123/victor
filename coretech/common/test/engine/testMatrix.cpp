#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header

#include "coretech/common/shared/math/matrix_impl.h"
#include "util/math/math.h"
#include <iostream>

// For test debug printing
#define DEBUG_TEST_MATRIX


#define ASSERT_NEAR_EQ(a,b) ASSERT_NEAR(a,b,Util::FLOATING_POINT_COMPARISON_TOLERANCE)

using namespace std;
using namespace Anki;


GTEST_TEST(TestMatrix, MatrixConstructorsAndAccessors)
{
  
  /////////////// Square matrix /////////////////////
  float initValueA = 0.f;
  Matrix<float> A(3,3,initValueA);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A (before): \n" << A << "\n";
#endif
 
  ASSERT_EQ(A.GetNumRows(), 3);
  ASSERT_EQ(A.GetNumCols(), 3);
  
  // Test accessors
  // Make sure matrix A is all zeros
  for (int r=0; r< A.GetNumRows(); ++r) {
    for (int c=0; c<A.GetNumCols(); ++c) {
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
  ASSERT_DEATH(A(3,0),"");
  
  
  /////////////  Non-square matrix ///////////////////
  float initValueB = 3.14f;
  Matrix<float> B(3,4,initValueB);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix B (before): \n" << B << "\n";
#endif
  
  ASSERT_EQ(B.GetNumRows(), 3);
  ASSERT_EQ(B.GetNumCols(), 4);
  
  // Test accessors
  // Make sure matrix B initial values are correct
  for (int r=0; r< B.GetNumRows(); ++r) {
    for (int c=0; c<B.GetNumCols(); ++c) {
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
  ASSERT_DEATH(Matrix<float> C(0,3,(float*)NULL),"");
  ASSERT_DEATH(Matrix<float> C(1,0,(float*)NULL),"");
}

GTEST_TEST(TestMatrix, MatrixAssignFromStdVector)
{
  std::vector<float> vec(6);
  for(int i=0; i<6; ++i) {
    vec[i] = float(i+1);
  }
  
  Matrix<float> M(2,3,vec);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix M built from std::vector: \n" << M << "\n";
#endif
  
  for(int i=0,k=0; i<M.GetNumRows(); ++i) {
    for(int j=0; j<M.GetNumCols(); ++j, ++k) {
      ASSERT_EQ(M(i,j), vec[k]);
    }
  }
}

GTEST_TEST(TestMatrix, SmallMatrixConstructorsAndAccessors)
{
  ///////// SmallMatrix //////////
  Matrix_2x2f M;

#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix M (before): \n" << M << "\n";
#endif
  
  // Test accessors
  // Make sure matrix B initial values are correct
  for (int r=0; r< M.GetNumRows(); ++r) {
    for (int c=0; c<M.GetNumCols(); ++c) {
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
  ASSERT_DEATH(M(2,2),"");
  
  
  // Contstructor with initial value array
  float N_init_array[] = {0, 1, 2, 3,
                          4, 5, 6, 7,
                          8, 9,10,11};
  
  {
    Matrix_3x4f N(N_init_array);
    
    EXPECT_EQ(N(0,0),0);  EXPECT_EQ(N(0,1),1);  EXPECT_EQ(N(0,2),2);  EXPECT_EQ(N(0,3),3);
    EXPECT_EQ(N(1,0),4);  EXPECT_EQ(N(1,1),5);  EXPECT_EQ(N(1,2),6);  EXPECT_EQ(N(1,3),7);
    EXPECT_EQ(N(2,0),8);  EXPECT_EQ(N(2,1),9);  EXPECT_EQ(N(2,2),10); EXPECT_EQ(N(2,3),11);
  }
  
  
  // Constructor with initializer list
  {
    Matrix_3x4f N({0,1,2,3, 4,5,6,7, 8,9,10,11});

    EXPECT_EQ(N(0,0),0);  EXPECT_EQ(N(0,1),1);  EXPECT_EQ(N(0,2),2);  EXPECT_EQ(N(0,3),3);
    EXPECT_EQ(N(1,0),4);  EXPECT_EQ(N(1,1),5);  EXPECT_EQ(N(1,2),6);  EXPECT_EQ(N(1,3),7);
    EXPECT_EQ(N(2,0),8);  EXPECT_EQ(N(2,1),9);  EXPECT_EQ(N(2,2),10); EXPECT_EQ(N(2,3),11);
  }
  
  // TODO: This doesn't fail anymore because each column (a Point3f) can now be initialized by a scalar. Fix?
  /*
  // Constructor with incorrect number of values in initializer list should fail
  // Note the ^ just matches the beginning of the failure message.
  EXPECT_DEATH( Matrix_3x3f Q({1,2,3}), "^Assertion failed");
   */
  
}

// Normal matrix multiplication
GTEST_TEST(TestMatrix, MatrixMultiplication1)
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
  
  ASSERT_EQ(C.GetNumRows(), 3);
  ASSERT_EQ(C.GetNumCols(), 3);
  
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
GTEST_TEST(TestMatrix, MatrixMultiplication2)
{
  Matrix<float> A(3,3,0.f), B(4,3,0.f);
  Matrix<float> C;
  
  ASSERT_DEATH(C = A*B,"");
  
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix C = A*B: \n" << C << "\n";
#endif
}

// Multiply 5x3 matrix A by 3x4 matrix B to get 5x4 matrix C
GTEST_TEST(TestMatrix, MatrixMatrixMultiplication)
{
  double A_data[15] = {
    0.814723686393179,   0.097540404999410,   0.157613081677548,
    0.905791937075619,   0.278498218867048,   0.970592781760616,
    0.126986816293506,   0.546881519204984,   0.957166948242946,
    0.913375856139019,   0.957506835434298,   0.485375648722841,
    0.632359246225410,   0.964888535199277,   0.800280468888800};
  
  double B_data[15] = {
    0.141886338627215,   0.792207329559554,   0.035711678574190,   0.678735154857773,
    0.421761282626275,   0.959492426392903,   0.849129305868777,   0.757740130578333,
    0.915735525189067,   0.655740699156587,   0.933993247757551,   0.743132468124916};
  
  double C_data[20] = {
    0.301068825302290,   0.842372668166146,   0.259129120851649,   0.744019285067097,
    1.134785558258487,   1.621249132682141,   1.175355454270838,   1.547101116864088,
    1.125182923445183,   1.252981886286981,   1.362895503797235,   1.211886326850504,
    0.977910591651454,   1.960584171941806,   1.299002878061871,   1.706180061474638,
    1.229521019817462,   1.951539345802075,   1.589354296533507,   1.755053615605484};
  
  Matrix<double> A(5,3,A_data);
  Matrix<double> B(3,4,B_data);
  Matrix<double> C_expected(5,4,C_data);
  
  // Instantiate with result of A*B
  Matrix<double> C1(A*B);
  EXPECT_TRUE( IsNearlyEqual(C1, C_expected) );
  
  // Assign result of A*B
  Matrix<double> C2 = A * B;
  EXPECT_TRUE( IsNearlyEqual(C2, C_expected) );
  
  // Multiply in place
  Matrix<double> C3(A); C3 *= B;
  EXPECT_TRUE( IsNearlyEqual(C3, C_expected) );

#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix C_true == A*B: \n" << C_expected << "\n";

  cout << "Matrix C1 = A*B: \n" << C1 << "\n";
  cout << "Matrix C2(A*B): \n" << C2 << "\n";
  cout << "Matrix C3 = A; C3 *= B: \n" << C3 << "\n";
#endif
  
} // MatrixMatrixMultiplication Test




GTEST_TEST(TestMatrix, SmallMatrixAddition)
{
  Matrix_3x3f A;
  
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;
  
  Matrix_3x3f B{{
    10.f, 1.f, 1.f,
    1.f, 10.f, 1.f,
    1.f, 1.f, 10.f
  }};

  // Normal SmallMatrix multiplication
  Matrix_3x3f C = A+B;
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix A: \n" << A << "\n";
  cout << "SmallMatrix B: \n" << B << "\n";
  cout << "SmallMatrix C = A+B: \n" << C << "\n";
#endif
  
  ASSERT_NEAR_EQ(C(0,0), 11.f);  ASSERT_NEAR_EQ(C(0,1), 1.f);    ASSERT_NEAR_EQ(C(0,2), 1.f);
  ASSERT_NEAR_EQ(C(1,0), 1.f);   ASSERT_NEAR_EQ(C(1,1), 12.f);   ASSERT_NEAR_EQ(C(1,2), 1.f);
  ASSERT_NEAR_EQ(C(2,0), 1.f);   ASSERT_NEAR_EQ(C(2,1), 1.f);    ASSERT_NEAR_EQ(C(2,2), 13.f);
  
  C += A;

  ASSERT_NEAR_EQ(C(0,0), 12.f);  ASSERT_NEAR_EQ(C(0,1), 1.f);    ASSERT_NEAR_EQ(C(0,2), 1.f);
  ASSERT_NEAR_EQ(C(1,0), 1.f);   ASSERT_NEAR_EQ(C(1,1), 14.f);   ASSERT_NEAR_EQ(C(1,2), 1.f);
  ASSERT_NEAR_EQ(C(2,0), 1.f);   ASSERT_NEAR_EQ(C(2,1), 1.f);    ASSERT_NEAR_EQ(C(2,2), 16.f);

  
  Matrix_3x3f D = C - A;

  ASSERT_NEAR_EQ(D(0,0), 11.f);  ASSERT_NEAR_EQ(D(0,1), 1.f);    ASSERT_NEAR_EQ(D(0,2), 1.f);
  ASSERT_NEAR_EQ(D(1,0), 1.f);   ASSERT_NEAR_EQ(D(1,1), 12.f);   ASSERT_NEAR_EQ(D(1,2), 1.f);
  ASSERT_NEAR_EQ(D(2,0), 1.f);   ASSERT_NEAR_EQ(D(2,1), 1.f);    ASSERT_NEAR_EQ(D(2,2), 13.f);

  D -= B;

  ASSERT_NEAR_EQ(D(0,0), 1.f);  ASSERT_NEAR_EQ(D(0,1), 0.f);   ASSERT_NEAR_EQ(D(0,2), 0.f);
  ASSERT_NEAR_EQ(D(1,0), 0.f);  ASSERT_NEAR_EQ(D(1,1), 2.f);   ASSERT_NEAR_EQ(D(1,2), 0.f);
  ASSERT_NEAR_EQ(D(2,0), 0.f);  ASSERT_NEAR_EQ(D(2,1), 0.f);   ASSERT_NEAR_EQ(D(2,2), 3.f);
  
}


GTEST_TEST(TestMatrix, SmallMatrixMultiplication)
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
  
  ASSERT_EQ(C.GetNumRows(), 3);
  ASSERT_EQ(C.GetNumCols(), 4);
  
  ASSERT_NEAR_EQ(C(0,0), 10.f);  ASSERT_NEAR_EQ(C(0,1), 0.f);    ASSERT_NEAR_EQ(C(0,2), 0.f);    ASSERT_NEAR_EQ(C(0,3), 0.f);
  ASSERT_NEAR_EQ(C(1,0), 0.f);   ASSERT_NEAR_EQ(C(1,1), 20.f);   ASSERT_NEAR_EQ(C(1,2), 0.f);    ASSERT_NEAR_EQ(C(1,3), 0.f);
  ASSERT_NEAR_EQ(C(2,0), 0.f);   ASSERT_NEAR_EQ(C(2,1), 0.f);    ASSERT_NEAR_EQ(C(2,2), 30.f);   ASSERT_NEAR_EQ(C(2,3), 0.f);
  
  
  // This results in compile time error if using OpenCV
  // Invalid multiplication
  // ASSERT_DEATH(C = B*A,"");
}


GTEST_TEST(TestMatrix, MatrixInverse)
{
  // Matrix
  Matrix<float> A(3,3,0.f);
  A(0,0) = 1.f;
  A(1,1) = 2.f;
  A(2,2) = 3.f;
  
  Matrix<float> A_orig, Ainv, I;
  A.CopyTo(A_orig);
  
  A.GetInverse(Ainv);
  
  // Verify A is left unchanged
  ASSERT_TRUE(IsNearlyEqual(A, A_orig));
  
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
  for (int r=0; r< A.GetNumRows(); ++r) {
    for (int c=0; c<A.GetNumCols(); ++c) {
      ASSERT_EQ(A(r,c), Ainv(r,c));
    }
  }
  
  
  // Invert non-square matrix
  Matrix<float> B(3,4,0.f), Binv;
  
  ASSERT_DEATH(B.GetInverse(Binv),"");
  ASSERT_DEATH(B.Invert(),"");
}


GTEST_TEST(TestMatrix, SmallMatrixInverse)
{
  float initValsA[] = {50, 0, 10,
                       0, 100, 0,
                       0, 0, 150};
  Matrix_3x3f A(initValsA);
  Matrix_3x3f Ainv, I;
  const Matrix_3x3f A_orig(initValsA);
  
  Ainv = A.GetInverse();
  
  // Verify A has not changed
  ASSERT_TRUE(IsNearlyEqual(A, A_orig));
  
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
  for (int r=0; r< A.GetNumRows(); ++r) {
    for (int c=0; c<A.GetNumCols(); ++c) {
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
//  ASSERT_DEATH(B.GetInverse(),"");
//  ASSERT_DEATH(B.Invert(),"");

  
}


GTEST_TEST(TestMatrix, MatrixTranspose)
{
  Matrix<float> A(3,3,0.f);
  A(0,0) = 1.f;    A(0,1) = 2.f;     A(0,2) = 3.f;
  A(1,0) = 4.f;    A(1,1) = 5.f;     A(1,2) = 6.f;
  A(2,0) = 7.f;    A(2,1) = 8.f;     A(2,2) = 9.f;
  
  Matrix<float> A_orig, A_t;
  A.CopyTo(A_orig);
  A.GetTranspose(A_t);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix A: \n" << A << "\n";
  cout << "Matrix A_transpose: \n" << A_t << "\n";
#endif

  // Verify A is left unchanged
  ASSERT_TRUE(IsNearlyEqual(A_orig, A));
  
  // Check that I is identity
  ASSERT_NEAR_EQ(A_t(0,0), 1.f); ASSERT_NEAR_EQ(A_t(0,1), 4.f); ASSERT_NEAR_EQ(A_t(0,2), 7.f);
  ASSERT_NEAR_EQ(A_t(1,0), 2.f); ASSERT_NEAR_EQ(A_t(1,1), 5.f); ASSERT_NEAR_EQ(A_t(1,2), 8.f);
  ASSERT_NEAR_EQ(A_t(2,0), 3.f); ASSERT_NEAR_EQ(A_t(2,1), 6.f); ASSERT_NEAR_EQ(A_t(2,2), 9.f);
  
  A.Transpose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "A transposed: \n" << A << "\n";
#endif
  
  // Check that A is now the same as A_t
  for (int r=0; r< A.GetNumRows(); ++r) {
    for (int c=0; c<A.GetNumCols(); ++c) {
      ASSERT_EQ(A(r,c), A_t(r,c));
    }
  }
  
  
  // Transpose non-square matrix
  Matrix<float> B(3,4);
  B(0,0) = 1.f;   B(0,1) = 2.f;   B(0,2) = 3.f;   B(0,3) = 4.f;
  B(1,0) = 5.f;   B(1,1) = 6.f;   B(1,2) = 7.f;   B(1,3) = 8.f;
  B(2,0) = 9.f;   B(2,1) = 10.f;  B(2,2) = 11.f;  B(2,3) = 12.f;
  
  Matrix<float> B_t;
  B.GetTranspose(B_t);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "Matrix B: \n" << B << "\n";
  cout << "Matrix B_transpose: \n" << B_t << "\n";
#endif
  
  ASSERT_EQ(B_t.GetNumRows(), B.GetNumCols());
  ASSERT_EQ(B_t.GetNumCols(), B.GetNumRows());
  
  ASSERT_NEAR_EQ(B_t(0,0), 1.f); ASSERT_NEAR_EQ(B_t(0,1), 5.f); ASSERT_NEAR_EQ(B_t(0,2), 9.f);
  ASSERT_NEAR_EQ(B_t(1,0), 2.f); ASSERT_NEAR_EQ(B_t(1,1), 6.f); ASSERT_NEAR_EQ(B_t(1,2), 10.f);
  ASSERT_NEAR_EQ(B_t(2,0), 3.f); ASSERT_NEAR_EQ(B_t(2,1), 7.f); ASSERT_NEAR_EQ(B_t(2,2), 11.f);
  ASSERT_NEAR_EQ(B_t(3,0), 4.f); ASSERT_NEAR_EQ(B_t(3,1), 8.f); ASSERT_NEAR_EQ(B_t(3,2), 12.f);
  
  B.Transpose();
  
  // Check that B is now the same as B_t
  for (int r=0; r< B.GetNumRows(); ++r) {
    for (int c=0; c<B.GetNumCols(); ++c) {
      ASSERT_EQ(B(r,c), B_t(r,c));
    }
  }
}



GTEST_TEST(TestMatrix, SmallMatrixTranspose)
{
  // Tranpose square matrix (note this actually tests SmallSquareMatrix)
  float initValsA[] = {1, 2, 3,
                       4, 5, 6,
                       7, 8, 9};
  const Matrix_3x3f A(initValsA);
  const Matrix_3x3f A_orig(initValsA);
  
  Matrix_3x3f A_t = A.GetTranspose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix A: \n" << A << "\n";
  cout << "SmallMatrix A_transpose: \n" << A_t << "\n";
#endif
  
  // Check that I is identity
  ASSERT_NEAR_EQ(A_t(0,0), 1.f); ASSERT_NEAR_EQ(A_t(0,1), 4.f); ASSERT_NEAR_EQ(A_t(0,2), 7.f);
  ASSERT_NEAR_EQ(A_t(1,0), 2.f); ASSERT_NEAR_EQ(A_t(1,1), 5.f); ASSERT_NEAR_EQ(A_t(1,2), 8.f);
  ASSERT_NEAR_EQ(A_t(2,0), 3.f); ASSERT_NEAR_EQ(A_t(2,1), 6.f); ASSERT_NEAR_EQ(A_t(2,2), 9.f);
  
  // Check that A is left unchanged
  ASSERT_TRUE(IsNearlyEqual(A, A_orig));
  
  // Transpose non-square matrix
  float initValsB[] = {1, 2, 3, 4,
                       5, 6, 7, 8,
                       9, 10, 11, 12};
  const Matrix_3x4f B(initValsB);
  const Matrix_3x4f B_orig(initValsB);
  
  SmallMatrix<4,3,float> B_t = B.GetTranspose();
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix B: \n" << B << "\n";
  cout << "SmallMatrix B_transpose: \n" << B_t << "\n";
#endif
  
  ASSERT_EQ(B_t.GetNumRows(), B.GetNumCols());
  ASSERT_EQ(B_t.GetNumCols(), B.GetNumRows());
  
  // Check that A is left unchanged
  ASSERT_TRUE(IsNearlyEqual(B, B_orig));
  
  ASSERT_NEAR_EQ(B_t(0,0), 1.f); ASSERT_NEAR_EQ(B_t(0,1), 5.f); ASSERT_NEAR_EQ(B_t(0,2), 9.f);
  ASSERT_NEAR_EQ(B_t(1,0), 2.f); ASSERT_NEAR_EQ(B_t(1,1), 6.f); ASSERT_NEAR_EQ(B_t(1,2), 10.f);
  ASSERT_NEAR_EQ(B_t(2,0), 3.f); ASSERT_NEAR_EQ(B_t(2,1), 7.f); ASSERT_NEAR_EQ(B_t(2,2), 11.f);
  ASSERT_NEAR_EQ(B_t(3,0), 4.f); ASSERT_NEAR_EQ(B_t(3,1), 8.f); ASSERT_NEAR_EQ(B_t(3,2), 12.f);

}

GTEST_TEST(TestMatrix, SmallSquareMatrixMultiplicationByPoint)
{
  float initValsM[] = {1, 2, 3,
                       4, 5, 6,
                       7, 8, 9};
  
  SmallSquareMatrix<3,float> M(initValsM);
  Point3f p(10,11,12);
  
  Point3f q_true(68, 167, 266);
  
  Point3f q1(M*p);
  
  ASSERT_EQ(q_true, q1);
  
  Point3f q2 = M*p;
  
  ASSERT_EQ(q_true, q2);
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallSquareMatrix M: \n" << M << "\n";
  cout << "Point p: " << p << "\n";
  cout << "Point q_true: " << q_true << "\n";
  cout << "Point q1: " << q1 << "\n";
  cout << "Point q2: " << q2 << "\n";
#endif
  
} // TestMatrix:SmallSquareMatrixMultiplicationByPoint()

