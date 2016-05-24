/**
 * File: testCircularBuffer
 *
 * Author: Mark Wesley
 * Created: 10/19/15
 *
 * Description: Unit tests for CircularBuffer
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=CircularBuffer*
 **/


#include "util/container/circularBuffer.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"


TEST(CircularBuffer, LoopBufferPushBack)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);

  testBuff.push_back(103);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[0]);
  
  testBuff.push_back(102);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  EXPECT_EQ(testBuff[1], 102);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[1]);

  testBuff.push_back(105);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  EXPECT_EQ(testBuff[1], 102);
  EXPECT_EQ(testBuff[2], 105);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[2]);
  
  testBuff.push_back(106);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 4);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  EXPECT_EQ(testBuff[1], 102);
  EXPECT_EQ(testBuff[2], 105);
  EXPECT_EQ(testBuff[3], 106);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[3]);
  
  testBuff.push_back(107);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  EXPECT_EQ(testBuff[1], 102);
  EXPECT_EQ(testBuff[2], 105);
  EXPECT_EQ(testBuff[3], 106);
  EXPECT_EQ(testBuff[4], 107);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  // 1st entry to loop the buffer
  
  testBuff.push_back(108);
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff[0], 102);
  EXPECT_EQ(testBuff[1], 105);
  EXPECT_EQ(testBuff[2], 106);
  EXPECT_EQ(testBuff[3], 107);
  EXPECT_EQ(testBuff[4], 108);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  // loop the buffer several more times
  // first prime the buffer (so we can easily test on previous values in the next loop)
  for (int i= -9; i <= -6; ++i)
  {
    testBuff.push_back(i);
    
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.size(), 5);
    EXPECT_EQ(testBuff.capacity(), 5);
    
    EXPECT_EQ(testBuff[4], i  );
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
  }
  // now continue to add items verifying buffer order each time, enough times to ensure several loops
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_back(i);
    
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.size(), 5);
    EXPECT_EQ(testBuff.capacity(), 5);

    EXPECT_EQ(testBuff[0], i-4);
    EXPECT_EQ(testBuff[1], i-3);
    EXPECT_EQ(testBuff[2], i-2);
    EXPECT_EQ(testBuff[3], i-1);
    EXPECT_EQ(testBuff[4], i  );
    
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
  }
  
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
}

TEST(CircularBuffer, LoopBufferPushBackArray)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  int testArray[] = { 101, 102, 103, 104, 105, 106, 107, 108 };
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);
  
  // Test inserting arrays smaller then the buffer size
  testBuff.push_back( testArray, 1 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[0]);
  
  testBuff.push_back( &testArray[1], 1 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[1]);
  
  testBuff.clear();
  testBuff.push_back( &testArray[0], 2 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[1]);
  
  testBuff.push_back( 109 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], 109);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[2]);

  // Test inserting arrays the same size as the buffer
  testBuff.clear();
  testBuff.push_back( &testArray[0], 4 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 4);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  EXPECT_EQ(testBuff[3], testArray[3]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[3]);

  testBuff.push_back( &testArray[0], 1 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  EXPECT_EQ(testBuff[3], testArray[3]);
  EXPECT_EQ(testBuff[4], testArray[0]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_back( &testArray[7], 1 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[1]);
  EXPECT_EQ(testBuff[1], testArray[2]);
  EXPECT_EQ(testBuff[2], testArray[3]);
  EXPECT_EQ(testBuff[3], testArray[0]);
  EXPECT_EQ(testBuff[4], testArray[7]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_back( 109 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[2]);
  EXPECT_EQ(testBuff[1], testArray[3]);
  EXPECT_EQ(testBuff[2], testArray[0]);
  EXPECT_EQ(testBuff[3], testArray[7]);
  EXPECT_EQ(testBuff[4], 109);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  // Buffer.capacity == array.size
  testBuff.clear();
  testBuff.push_back( &testArray[0], 5 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  EXPECT_EQ(testBuff[3], testArray[3]);
  EXPECT_EQ(testBuff[4], testArray[4]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  // Wrap buffer
  testBuff.clear();
  testBuff.push_back( &testArray[0], 3 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[2]);
  // Test Wrapping buffer
  testBuff.push_back( &testArray[3], 5 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[3]);
  EXPECT_EQ(testBuff[1], testArray[4]);
  EXPECT_EQ(testBuff[2], testArray[5]);
  EXPECT_EQ(testBuff[3], testArray[6]);
  EXPECT_EQ(testBuff[4], testArray[7]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  // Keep Wrapping
  testBuff.push_back( &testArray[0], 3 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[6]);
  EXPECT_EQ(testBuff[1], testArray[7]);
  EXPECT_EQ(testBuff[2], testArray[0]);
  EXPECT_EQ(testBuff[3], testArray[1]);
  EXPECT_EQ(testBuff[4], testArray[2]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  
  // Test Large Data
  // Generate test data
  const size_t testArray2Size = 30;
  int testArray2[testArray2Size];
  for (size_t idx = 0; idx < testArray2Size; ++idx ) {
    testArray2[idx] = (int)idx + 100;
  }
  
  // Test completely filling the buffer 6x (30/5 = 6)
  testBuff.clear();
  size_t loopSize = testBuff.capacity();
  for (size_t idx = 0; idx<testArray2Size; idx += loopSize) {
    testBuff.push_back(&testArray2[idx], loopSize);
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.capacity(), 5);
    EXPECT_EQ(testBuff.front(), testBuff[0]);

    // Test all buffers between each push
    for (size_t testDataOffset = 0; testDataOffset < loopSize; ++testDataOffset) {
      EXPECT_EQ(testBuff[testDataOffset], testArray2[idx + testDataOffset]);
    }
  }
  
  testBuff.clear();
  
  // Test completely filling the buffer 6x while poping data out in between iterations
  for (size_t idx = 0; idx<testArray2Size; idx += loopSize) {
    testBuff.push_back(&testArray2[idx], loopSize);
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.size(), 5);
    EXPECT_EQ(testBuff.capacity(), 5);
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
    
    // Test all buffers
    for (size_t testDataOffset = 0; testDataOffset < loopSize; ++testDataOffset) {
      EXPECT_EQ(testBuff[testDataOffset], testArray2[idx + testDataOffset]);
    }
    
    
    // Pull out entries
    size_t outArray2Size = 3;
    int outArray2[outArray2Size];
    
    testBuff.front(outArray2, outArray2Size);
    testBuff.pop_front(outArray2Size);
    
    // Test remaing buffer entities
    for (size_t testDataOffset = 0; testDataOffset < loopSize - outArray2Size; ++testDataOffset) {
      EXPECT_EQ(testBuff[testDataOffset], testArray2[ outArray2Size + idx + testDataOffset]);
    }
  }

  // Test buffer where the middle is empty
  testBuff.clear();
  testBuff.push_back(testArray, 3);
  testBuff.pop_front(3);
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);

  testBuff.push_back(testArray, 3);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  
}

TEST(CircularBuffer, LoopBufferNormVsArray)
{
  Anki::Util::CircularBuffer<int> testBufferNorm(100);
  Anki::Util::CircularBuffer<int> testBufferArray(100);

  // Test data
  const size_t testArraySize = 300;
  int testArray[testArraySize];
  for (int idx = 0; idx < testArraySize; ++idx) {
    testArray[idx] = idx + 100;
  }
  
  // Test loading vectors
  int pushCount = 50;
  for (int idx = 0; idx < pushCount; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(testArray, pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  // Pop some data
  int poppedDataNorm[100];
  int poppedDataArray[100];
  int popCount = 12;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  // Push more
  pushCount = 13;
  int offset = 50;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  // Pop more
  popCount = 50;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  // Push alot
  pushCount = 90;
  offset = 100;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  // Push alot, again
  pushCount = 90;
  offset = 200;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  // Pop
  popCount = 20;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  popCount = 20;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  popCount = 20;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  // Push single
  pushCount = 1;
  offset = 200;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  offset = 290;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );

  offset = 295;
  for (int idx = offset; idx < pushCount + offset; ++idx) {
    testBufferNorm.push_back(testArray[idx]);
  }
  testBufferArray.push_back(&testArray[offset], pushCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  
  // Pop Single
  popCount = 1;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
  popCount = 1;
  for (int idx = 0; idx < popCount; ++idx) {
    poppedDataNorm[idx] = testBufferNorm.front();
    testBufferNorm.pop_front();
  }
  testBufferArray.front(&poppedDataArray[0], popCount);
  testBufferArray.pop_front(popCount);
  
  EXPECT_EQ( testBufferNorm, testBufferArray );
  EXPECT_EQ( memcmp(&poppedDataNorm[0], &poppedDataArray[0], popCount), 0 );
  
}


TEST(CircularBuffer, LoopBufferFrontArray)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  int testArray[] = { 101, 102, 103, 104, 105, 106, 107, 108 };
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);
  
  testBuff.push_back( &testArray[0], 5 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[0]);
  EXPECT_EQ(testBuff[1], testArray[1]);
  EXPECT_EQ(testBuff[2], testArray[2]);
  EXPECT_EQ(testBuff[3], testArray[3]);
  EXPECT_EQ(testBuff[4], testArray[4]);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_back( &testArray[5], 3 );
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], testArray[3]);
  EXPECT_EQ(testBuff[1], testArray[4]);
  EXPECT_EQ(testBuff[2], testArray[5]);
  EXPECT_EQ(testBuff[3], testArray[6]);
  EXPECT_EQ(testBuff[4], testArray[7]);
  
  int outArray[] = {0, 0, 0, 0, 0 };
  size_t resultCount = testBuff.front( outArray, 3 );
  EXPECT_EQ(resultCount, 3);
  EXPECT_EQ(testArray[3], outArray[0]);
  EXPECT_EQ(testArray[4], outArray[1]);
  EXPECT_EQ(testArray[5], outArray[2]);
  
  resultCount = testBuff.front( outArray, 5 );
  EXPECT_EQ(resultCount, 5);
  EXPECT_EQ(testArray[3], outArray[0]);
  EXPECT_EQ(testArray[4], outArray[1]);
  EXPECT_EQ(testArray[5], outArray[2]);
  EXPECT_EQ(testArray[6], outArray[3]);
  EXPECT_EQ(testArray[7], outArray[4]);
  
  testBuff.pop_front( 3 );
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff[0], testArray[6]);
  EXPECT_EQ(testBuff[1], testArray[7]);
  
  testBuff.pop_front( 1 );
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff[0], testArray[7]);

}


TEST(CircularBuffer, LoopBufferPushFront)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);
  
  testBuff.push_front(103);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 103);
  
  testBuff.push_front(102);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 102);
  EXPECT_EQ(testBuff[1], 103);
  
  testBuff.push_front(105);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 105);
  EXPECT_EQ(testBuff[1], 102);
  EXPECT_EQ(testBuff[2], 103);
  
  testBuff.push_front(106);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 4);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 106);
  EXPECT_EQ(testBuff[1], 105);
  EXPECT_EQ(testBuff[2], 102);
  EXPECT_EQ(testBuff[3], 103);
  
  testBuff.push_front(107);
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 107);
  EXPECT_EQ(testBuff[1], 106);
  EXPECT_EQ(testBuff[2], 105);
  EXPECT_EQ(testBuff[3], 102);
  EXPECT_EQ(testBuff[4], 103);
  
  // 1st entry to loop the buffer
  
  testBuff.push_front(108);
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff[0], 108);
  EXPECT_EQ(testBuff[1], 107);
  EXPECT_EQ(testBuff[2], 106);
  EXPECT_EQ(testBuff[3], 105);
  EXPECT_EQ(testBuff[4], 102);
  
  // loop the buffer several more times
  // first prime the buffer (so we can easily test on previous values in the next loop)
  for (int i= -9; i <= -6; ++i)
  {
    testBuff.push_front(i);
    
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.size(), 5);
    EXPECT_EQ(testBuff.capacity(), 5);
    
    EXPECT_EQ(testBuff[0], i  );
    
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
  }
  // now continue to add items verifying buffer order each time, enough times to ensure several loops
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_front(i);
    
    EXPECT_FALSE(testBuff.empty());
    EXPECT_EQ(testBuff.size(), 5);
    EXPECT_EQ(testBuff.capacity(), 5);
    
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
    
    EXPECT_EQ(testBuff[0], i);
    EXPECT_EQ(testBuff[1], i-1);
    EXPECT_EQ(testBuff[2], i-2);
    EXPECT_EQ(testBuff[3], i-3);
    EXPECT_EQ(testBuff[4], i-4);
    
    EXPECT_EQ(testBuff.front(), testBuff[0]);
    EXPECT_EQ(testBuff.back(),  testBuff[4]);
  }
  
  EXPECT_EQ(testBuff[0], 24);
  EXPECT_EQ(testBuff[1], 23);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 21);
  EXPECT_EQ(testBuff[4], 20);
}


TEST(CircularBuffer, PushPopMix)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  testBuff.push_front(123);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 123);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[0]);
  
  testBuff.push_back(124);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 123);
  EXPECT_EQ(testBuff[1], 124);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[1]);
  
  testBuff.push_front(122);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 122);
  EXPECT_EQ(testBuff[1], 123);
  EXPECT_EQ(testBuff[2], 124);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[2]);
  
  testBuff.pop_back();
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 2);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 122);
  EXPECT_EQ(testBuff[1], 123);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[1]);
  
  testBuff.pop_front();
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 1);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 123);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[0]);
  
  testBuff.pop_front();
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 5);

  testBuff.push_front(10);
  testBuff.push_back(11);
  testBuff.push_front(9);
  testBuff.push_back(12);
  testBuff.push_front(8);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  8);
  EXPECT_EQ(testBuff[1],  9);
  EXPECT_EQ(testBuff[2], 10);
  EXPECT_EQ(testBuff[3], 11);
  EXPECT_EQ(testBuff[4], 12);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_front(7);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  7);
  EXPECT_EQ(testBuff[1],  8);
  EXPECT_EQ(testBuff[2],  9);
  EXPECT_EQ(testBuff[3], 10);
  EXPECT_EQ(testBuff[4], 11);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_front(6);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  6);
  EXPECT_EQ(testBuff[1],  7);
  EXPECT_EQ(testBuff[2],  8);
  EXPECT_EQ(testBuff[3],  9);
  EXPECT_EQ(testBuff[4], 10);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.push_back(81);
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  7);
  EXPECT_EQ(testBuff[1],  8);
  EXPECT_EQ(testBuff[2],  9);
  EXPECT_EQ(testBuff[3], 10);
  EXPECT_EQ(testBuff[4], 81);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[4]);
  
  testBuff.pop_front();
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 4);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  8);
  EXPECT_EQ(testBuff[1],  9);
  EXPECT_EQ(testBuff[2], 10);
  EXPECT_EQ(testBuff[3], 81);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[3]);
  
  testBuff.pop_back();
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 3);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0],  8);
  EXPECT_EQ(testBuff[1],  9);
  EXPECT_EQ(testBuff[2], 10);
  EXPECT_EQ(testBuff.front(), testBuff[0]);
  EXPECT_EQ(testBuff.back(),  testBuff[2]);
}


// ========== Move and Copy Tests ==========


TEST(CircularBuffer, MoveConstructor)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_back(i);
  }
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  Anki::Util::CircularBuffer<int> testBuff2( std::move(testBuff) ); // Move Constructor
  
  // testBuff should now be empty and have no storage allocated, testBuff2 should be equivalent to old testBuff
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 0);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
  
  // check we can reserve testBuff again and add to it without it affecting testBuff2
  
  testBuff.Reset(5);
  for (int i= 1; i <= 5; ++i)
  {
    testBuff.push_back(i);
  }
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 1);
  EXPECT_EQ(testBuff[1], 2);
  EXPECT_EQ(testBuff[2], 3);
  EXPECT_EQ(testBuff[3], 4);
  EXPECT_EQ(testBuff[4], 5);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
}


TEST(CircularBuffer, MoveAssignment)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_back(i);
  }
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  Anki::Util::CircularBuffer<int> testBuff2(0);
  testBuff2 = std::move(testBuff); // Move Assignment
  
  // testBuff should now be empty and have no storage allocated, testBuff2 should be equivalent to old testBuff
  
  EXPECT_TRUE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 0);
  EXPECT_EQ(testBuff.capacity(), 0);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
  
  // check we can reserve testBuff again and add to it without it affecting testBuff2
  
  testBuff.Reset(5);
  for (int i= 1; i <= 5; ++i)
  {
    testBuff.push_back(i);
  }
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 1);
  EXPECT_EQ(testBuff[1], 2);
  EXPECT_EQ(testBuff[2], 3);
  EXPECT_EQ(testBuff[3], 4);
  EXPECT_EQ(testBuff[4], 5);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
}


TEST(CircularBuffer, CopyConstructor)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_back(i);
  }
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  Anki::Util::CircularBuffer<int> testBuff2( testBuff ); // CopyConstructor
  
  // testBuff and testBuff2 should now be equivalent (although independent if modified in the future)
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
  
  // check we can modify testBuff without it affecting testBuff2
  
  for (int i= 1; i <= 5; ++i)
  {
    testBuff.push_back(i);
  }
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 1);
  EXPECT_EQ(testBuff[1], 2);
  EXPECT_EQ(testBuff[2], 3);
  EXPECT_EQ(testBuff[3], 4);
  EXPECT_EQ(testBuff[4], 5);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
}


TEST(CircularBuffer, CopyAssignment)
{
  Anki::Util::CircularBuffer<int> testBuff(5);
  
  for (int i= -5; i <= 24; ++i)
  {
    testBuff.push_back(i);
  }
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  Anki::Util::CircularBuffer<int> testBuff2(0);
  testBuff2 = testBuff; // Copy Assignment
  
  // testBuff and testBuff2 should now be equivalent (although independent if modified in the future)
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 20);
  EXPECT_EQ(testBuff[1], 21);
  EXPECT_EQ(testBuff[2], 22);
  EXPECT_EQ(testBuff[3], 23);
  EXPECT_EQ(testBuff[4], 24);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
  
  // check we can modify testBuff without it affecting testBuff2
  
  for (int i= 1; i <= 5; ++i)
  {
    testBuff.push_back(i);
  }
  
  EXPECT_FALSE(testBuff.empty());
  EXPECT_EQ(testBuff.size(), 5);
  EXPECT_EQ(testBuff.capacity(), 5);
  EXPECT_EQ(testBuff[0], 1);
  EXPECT_EQ(testBuff[1], 2);
  EXPECT_EQ(testBuff[2], 3);
  EXPECT_EQ(testBuff[3], 4);
  EXPECT_EQ(testBuff[4], 5);
  
  EXPECT_FALSE(testBuff2.empty());
  EXPECT_EQ(testBuff2.size(), 5);
  EXPECT_EQ(testBuff2.capacity(), 5);
  EXPECT_EQ(testBuff2[0], 20);
  EXPECT_EQ(testBuff2[1], 21);
  EXPECT_EQ(testBuff2[2], 22);
  EXPECT_EQ(testBuff2[3], 23);
  EXPECT_EQ(testBuff2[4], 24);
}

