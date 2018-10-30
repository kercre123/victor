/**
 * File: testRingBuffContiguousRead.cpp
 *
 * Author: ross
 * Created: Oct 19 2018
 *
 * Description: Unit tests for RingBuffContiguousRead
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=RingBuffContiguousRead.*
 **/


#define private public
#include "util/container/ringBuffContiguousRead.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"

#include <numeric>


using namespace Anki::Util;

TEST(RingBuffContiguousRead, EmptyFull)
{
  std::vector<int> data(10);
  std::iota( std::begin(data), std::end(data), 0 ); // Fill with 0, 1, ..., 9
  
  RingBuffContiguousRead<int> testBuff{ 10, 5 };
  
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  
  size_t numAdded = testBuff.AddData( data.data(), 1 );
  EXPECT_EQ( numAdded, 1 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 1 );
  EXPECT_EQ( testBuff.GetNumUsed(), 1 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 9 );
  
  numAdded = testBuff.AddData( data.data() + 1, 9 );
  EXPECT_EQ( numAdded, 9 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  // can't add any more
  numAdded = testBuff.AddData( data.data(), 1 );
  EXPECT_EQ( numAdded, 0 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  testBuff.Reset();
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );

}

bool CompareData( const int* a, const int* b, int len )
{
  bool success = true;
  for( int i=0; i<len; ++i ) {
    if( a[i] != b[i] ) {
      std::cout << i << "th element differs. a=" << a[i] << ", b=" << b[i] << std::endl;
      success = false;
    }
  }
  return success;
}

TEST(RingBuffContiguousRead, WrappedRead)
{
  // this test: writes never wrap around the size of the buffer. one read will wrap
  
  // pad the actual data with garbage that doesnt match the interior
  std::vector<int> data = {-10, -20, -30, -40, -50, 0,1,2,3,4,5,6,7,8,9, -60, -70, -80, -90, -100};
  int* writeData = data.data() + 5;
  
  const int* readData;
  
  RingBuffContiguousRead<int> testBuff{ 10, 5 };
  
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  
  size_t numAdded = testBuff.AddData( writeData, 10 );
  EXPECT_EQ( numAdded, 10 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  readData = testBuff.ReadData( 5 );
  bool same = CompareData( readData, writeData, 5 );
  EXPECT_TRUE( same );
  // reading doesn't affect cursor placement
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  // moving the cursor affects cursor placement
  bool advanceAllowed = testBuff.AdvanceCursor( 20 );
  EXPECT_FALSE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 5 );
  EXPECT_EQ( testBuff.GetNumUsed(), 5 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 5 );
  
  numAdded = testBuff.AddData( writeData, 5 );
  EXPECT_EQ( numAdded, 5 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  readData = testBuff.ReadData( 3 );
  std::vector<int> expected = {5,6,7};
  same = CompareData( readData, expected.data(), 3 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 3 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 7 );
  EXPECT_EQ( testBuff.GetNumUsed(), 7 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 3 );
  
  // this would normally wrap around, but the read should be contiguous
  readData = testBuff.ReadData( 5 );
  expected = {8,9,0,1,2};
  same = CompareData( readData, expected.data(), 5 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 2 );
  EXPECT_EQ( testBuff.GetNumUsed(), 2 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 8 );
  
  
  // eat up the rest of the data
  readData = testBuff.ReadData( 2 );
  expected = {3,4};
  same = CompareData( readData, expected.data(), 2 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 2 );
  EXPECT_TRUE( advanceAllowed );
  
  // should be empty now
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  
}

TEST(RingBuffContiguousRead, WrappedWriteAndRead)
{
  // this test: one write wraps around the size of the buffer. one read will also wrap
  
  // pad the actual data with garbage that doesnt match the interior
  std::vector<int> data = {-10, -20, -30, -40, -50, 0,1,2,3,4,5,6,7,8,9, -60, -70, -80, -90, -100};
  int* writeData = data.data() + 5;
  
  const int* readData;
  
  RingBuffContiguousRead<int> testBuff{ 10, 5 };
  
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  
  size_t numAdded = testBuff.AddData( writeData, 8 );
  EXPECT_EQ( numAdded, 8 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 8 );
  EXPECT_EQ( testBuff.GetNumUsed(), 8 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 2 );
  
  readData = testBuff.ReadData( 5 );
  bool same = CompareData( readData, writeData, 5 );
  EXPECT_TRUE( same );
  // reading doesn't affect cursor placement
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 8 );
  EXPECT_EQ( testBuff.GetNumUsed(), 8 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 2 );
  bool advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 3 );
  EXPECT_EQ( testBuff.GetNumUsed(), 3 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 7 );
  
  // this write wraps around
  numAdded = testBuff.AddData( writeData, 7 );
  EXPECT_EQ( numAdded, 7 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  
  // [0,1,2,3,4,5,6,7,0,1,.... 2,3,4,5,6]
  readData = testBuff.ReadData( 3 );
  std::vector<int> expected = {5,6,7};
  same = CompareData( readData, expected.data(), 3 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 3 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 7 );
  EXPECT_EQ( testBuff.GetNumUsed(), 7 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 3 );
  
  // this would normally wrap around, but the read should be contiguous
  readData = testBuff.ReadData( 5 );
  same = CompareData( readData, writeData, 5 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 2 );
  EXPECT_EQ( testBuff.GetNumUsed(), 2 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 8 );
  
  
  // eat up the rest of the data
  readData = testBuff.ReadData( 2 );
  expected = {5,6};
  same = CompareData( readData, expected.data(), 2 );
  EXPECT_TRUE( same );
  advanceAllowed = testBuff.AdvanceCursor( 2 );
  EXPECT_TRUE( advanceAllowed );
  
  // should be empty now
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  
}

