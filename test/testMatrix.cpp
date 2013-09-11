#include "gtest/gtest.h"

#include "anki/general.h"
#include "anki/math/matrix.h"
#include <iostream>

// For test debug printing
#define DEBUG_TEST_MATRIX


using namespace std;
using namespace Anki;

TEST(TestMatrix, ConstructorsAndAccessors)
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
  
  
  
  // Small Matrix:
  Matrix_2x2f M;

#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix M (before): \n" << M << "\n";
#endif
  
  
  M(0,0) = 1.f;
  M(0,1) = 2.f;
  M(1,0) = 3.f;
  M(1,1) = 4.f;
  
#ifdef DEBUG_TEST_MATRIX
  cout << "SmallMatrix M (after): \n" << M << "\n";
#endif
}


// TODO:
// Access invalid element
// Make matrix too big
// Make zero size matrix

