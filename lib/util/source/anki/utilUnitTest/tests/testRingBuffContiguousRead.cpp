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
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );
  
  size_t numAdded = testBuff.AddData( data.data(), 1 );
  EXPECT_EQ( numAdded, 1 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 1 );
  EXPECT_EQ( testBuff.GetNumUsed(), 1 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 9 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 1 );
  
  numAdded = testBuff.AddData( data.data() + 1, 9 );
  EXPECT_EQ( numAdded, 9 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  
  // can't add any more
  numAdded = testBuff.AddData( data.data(), 1 );
  EXPECT_EQ( numAdded, 0 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  
  testBuff.Reset();
  EXPECT_TRUE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 0 );
  EXPECT_EQ( testBuff.GetNumUsed(), 0 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 10 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );

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

// helper to check that calling ReadData on the max contiguous size doesn't read outside owned memory
// (assumes buff was made with negative integer padding)
bool ContiguousHasNegativeElements( const RingBuffContiguousRead<int>& buff )
{
  const unsigned int readSize = (int)buff.GetContiguousSize();
  if( readSize == 0 ) {
    return false;
  }
  const int* data = buff.ReadData( readSize );
  if( data != nullptr ) {
    for( int i=0; i<readSize; ++i ) {
      if( data[i] < 0 ) {
        return true;
      }
    }
    return false;
  } else {
    return true;
  }
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  size_t numAdded = testBuff.AddData( writeData, 10 );
  EXPECT_EQ( numAdded, 10 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  readData = testBuff.ReadData( 5 );
  bool same = CompareData( readData, writeData, 5 );
  EXPECT_TRUE( same );
  // reading doesn't affect cursor placement
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  // moving the cursor affects cursor placement
  bool advanceAllowed = testBuff.AdvanceCursor( 20 );
  EXPECT_FALSE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 5 );
  EXPECT_EQ( testBuff.GetNumUsed(), 5 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 5 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 5 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  numAdded = testBuff.AddData( writeData, 5 );
  EXPECT_EQ( numAdded, 5 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 7 ); // never less than 5 when there are >= 5 elems
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 2 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  size_t numAdded = testBuff.AddData( writeData, 8 );
  EXPECT_EQ( numAdded, 8 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 8 );
  EXPECT_EQ( testBuff.GetNumUsed(), 8 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 2 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 8 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  readData = testBuff.ReadData( 5 );
  bool same = CompareData( readData, writeData, 5 );
  EXPECT_TRUE( same );
  // reading doesn't affect cursor placement
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 8 );
  EXPECT_EQ( testBuff.GetNumUsed(), 8 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 2 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 8 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  bool advanceAllowed = testBuff.AdvanceCursor( 5 );
  EXPECT_TRUE( advanceAllowed );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 3 );
  EXPECT_EQ( testBuff.GetNumUsed(), 3 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 7 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 3 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  // this write wraps around
  numAdded = testBuff.AddData( writeData, 7 );
  EXPECT_EQ( numAdded, 7 );
  EXPECT_FALSE( testBuff.IsEmpty() );
  EXPECT_EQ( testBuff.Capacity(), 10 );
  EXPECT_EQ( testBuff.Size(), 10 );
  EXPECT_EQ( testBuff.GetNumUsed(), 10 );
  EXPECT_EQ( testBuff.GetNumAvailable(), 0 );
  EXPECT_EQ( testBuff.GetContiguousSize(), 10 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 7 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 2 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
  
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
  EXPECT_EQ( testBuff.GetContiguousSize(), 0 );
  EXPECT_FALSE( ContiguousHasNegativeElements( testBuff ) );
  
}

