/**
 *
 *  testMsgPack.cpp
 *  BaseStation
 *
 *  Created by Jarrod Hatfield on 11/13/13.
 *  Copyright (c) 2013 Anki. All rights reserved.
 *
 *  Desription: Tests the packing of data to be sent to/from car/basestation.
 *              UtilMsgPack/UtilMsgUnpack are the main funcitons used to pack/unpack this data.
 *
 **/
 
#include "util/helpers/includeGTest.h"
#include "util/utilMessaging/utilMessaging.h"
#include <float.h>
#include "util/random/randomGenerator.h"

using namespace std;

static const unsigned int MAX_ELEMENTS = UINT8_MAX;

/**
 *  Helper function for the testing of packing different types and their upper/lower bounds.
 **/
template<typename T>
void TestTypePackingHelper( char packType, T minValue, T maxValue )
{
  // Setup our test data.
  const T srcBuffer[] = { 0, -1, minValue, maxValue };
  const int bufferSize = sizeof(srcBuffer);
  const int numElements = ( sizeof(srcBuffer) / sizeof(T) );
  T* dstBuffer = new T[numElements];
  
  char* packString = new char[numElements+1];
  for ( int i = 0 ; i < numElements; ++i )
  {
    packString[i] = packType;
  }
  packString[numElements] = '\0';
  
  UtilMsgError error = UTILMSG_OK;
  
  memset(dstBuffer, 0, bufferSize);
  unsigned int bytesWritten = 0;
  error = SafeUtilMsgPack(dstBuffer, bufferSize, &bytesWritten, packString, srcBuffer[0], srcBuffer[1], srcBuffer[2], srcBuffer[3]);
  uint32_t expectedSize = SafeUtilGetMsgSize(packString, srcBuffer[0], srcBuffer[1], srcBuffer[2], srcBuffer[3]);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(bufferSize, bytesWritten);
  EXPECT_EQ(bytesWritten, expectedSize);
  
  // Check that the values were packed properly.
  for ( int i = 0; i < numElements; ++i )
  {
    EXPECT_EQ( dstBuffer[i], srcBuffer[i] );
  }
  
  memset(dstBuffer, 0, bufferSize);
  unsigned int bytesRead = 0;
  error = SafeUtilMsgUnpack(srcBuffer, bufferSize, &bytesRead, packString, &dstBuffer[0], &dstBuffer[1], &dstBuffer[2], &dstBuffer[3]);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(bufferSize, bytesRead);
  
  // Check that the values were unpacked properly.
  for ( int i = 0; i < numElements; ++i )
  {
    EXPECT_EQ( dstBuffer[i], srcBuffer[i] );
  }
  
  delete[] packString;
  delete[] dstBuffer;
}

/**
 *  Test packing of all supported types.
 **/
TEST(TestMsgPack, TestTypePacking)
{
  TestTypePackingHelper<char>( 'c', INT8_MIN, INT8_MAX );
  TestTypePackingHelper<short>( 'h', INT16_MIN, INT16_MAX );
  TestTypePackingHelper<int>( 'i', INT32_MIN, INT32_MAX );
  TestTypePackingHelper<long long int>( 'l', INT64_MIN, INT64_MAX );
  TestTypePackingHelper<float>( 'f', FLT_MIN, FLT_MAX );
  TestTypePackingHelper<double>( 'd', DBL_MIN, DBL_MAX );
}


/**
 *  Helper functions to test packing an array of specified size.
 *    - packs/unpacks an array of type T with the specified size.
 **/
template<typename T>
void TestArrayPackingHelper( int size, const char* packString, T* srcBuffer )
{
  // When we pack array size, we pack it as a char and therefore can't have more than max char.
  ASSERT_LE(size, MAX_ELEMENTS);
  
  static const char BAD_VALUE = 0xDE; // Simple way to check for uninitialized values.
  static const int DST_BUFFER_SIZE = ( ( MAX_ELEMENTS * sizeof(T) ) + 2 ); // Add some padding for the size char and for buffer overrun detection.
  char dstBuffer[DST_BUFFER_SIZE];
  
  const int expectedBufferSize = ( 1 + ( size * sizeof(T) ) );
  
  // Testing UtilMsgPack
  {
    memset(dstBuffer, BAD_VALUE, DST_BUFFER_SIZE);
    unsigned int bytesWritten = 0;
    const UtilMsgError error = SafeUtilMsgPack(dstBuffer, DST_BUFFER_SIZE, &bytesWritten, packString, size, srcBuffer);
    uint32_t expectedSize = SafeUtilGetMsgSize(packString, size, srcBuffer);
    EXPECT_EQ(UTILMSG_OK, error);
    EXPECT_EQ(expectedBufferSize, bytesWritten);
    EXPECT_EQ(expectedSize, bytesWritten);
    
    EXPECT_EQ((char)size, dstBuffer[0]); // Check that the size was packed correctly.
    
    // Check that all of the elements were packed correctly.
    T* TArray = (T*)(dstBuffer + sizeof(char));
    for ( int i = 0; i < size; ++i )
    {
      EXPECT_EQ(srcBuffer[i], TArray[i]);
    }
    
    const int lastIndex = ( 1 + (size*sizeof(T)) );
    EXPECT_EQ(BAD_VALUE, dstBuffer[lastIndex]); // Check that we didn't overrun the buffer.
  }
  
  // Testing UtilMsgUnpack
  {
    const int ArraySize = ( size + 1 );
    unsigned char unpackedSize = 0;
    T* unpackedBuffer = new T[ArraySize];
    
    memset(unpackedBuffer, BAD_VALUE, (ArraySize*sizeof(T)) );
    unsigned int bytesRead = 0;
    const UtilMsgError error = SafeUtilMsgUnpack(dstBuffer, DST_BUFFER_SIZE, &bytesRead, packString, &unpackedSize, unpackedBuffer);
    EXPECT_EQ(UTILMSG_OK, error);
    EXPECT_EQ(expectedBufferSize, bytesRead);
    
    EXPECT_EQ(size, unpackedSize);
    
    for ( int i = 0; i < size; ++i )
    {
      EXPECT_EQ(srcBuffer[i], unpackedBuffer[i]);
    }
    
    const T* lastElement = &unpackedBuffer[ArraySize-1];
    EXPECT_EQ(BAD_VALUE, *(char*)lastElement); // Check that we didn't overrun the buffer.
    
    delete[] unpackedBuffer;
  }
}

template<typename T>
void TestArrayTypePackingHelper( const char* packString, T* srcArray )
{
  TestArrayPackingHelper<T>( 0, packString, srcArray );
  TestArrayPackingHelper<T>( 1, packString, srcArray );
  TestArrayPackingHelper<T>( MAX_ELEMENTS, packString, srcArray );
}

/**
 *  Simply fill the byteArray with randomized bytes.
 **/
void RandomizeByteArray( void* byteArray, unsigned int size )
{
  Anki::Util::RandomGenerator rand(12345);
  
  // Fill the buffer with random bytes.
  char* charArray = (char*)byteArray;
  for ( int i = 0; i < size; ++i )
  {
    charArray[i] = rand.RandIntInRange( 0, UINT8_MAX );
  }
}


/**
 *  Test packing arrays of varying size and type.
 **/
TEST(TestMsgPack, TestArrayPacking)
{
  // Test array packing of different int types.
  {
    // Creat a buffer to hold our random test data.
    // Create one large enough to hold all of our values and just reuse it.
    const unsigned int INT_BUFFER_SIZE = ( MAX_ELEMENTS * sizeof( long long int ) );
    unsigned char intBuffer[INT_BUFFER_SIZE];

    RandomizeByteArray( intBuffer, INT_BUFFER_SIZE );
    
    TestArrayTypePackingHelper<char>( "ac", (char*)intBuffer );
    TestArrayTypePackingHelper<short>( "ah", (short*)intBuffer );
    TestArrayTypePackingHelper<int>( "ai", (int*)intBuffer );
    TestArrayTypePackingHelper<long long int>( "al", (long long int*)intBuffer );
  }
  
  
  // Since gtest seems to automatically fail tests when a floating value is Nan, we cannot simply use an array of random bytes.
  // We'll need to create an array of random legitimate float values instead.
  {
    Anki::Util::RandomGenerator rand(12345);

    float floatBuffer[MAX_ELEMENTS];
    double doubleBuffer[MAX_ELEMENTS];
    for ( int i = 0; i < MAX_ELEMENTS; ++i )
    {
      floatBuffer[i] = rand.RandDblInRange( FLT_MIN, FLT_MAX );
      doubleBuffer[i] = rand.RandDblInRange( DBL_MIN, DBL_MAX );
    }
    
    TestArrayTypePackingHelper<float>( "af", floatBuffer );
    TestArrayTypePackingHelper<double>( "ad", doubleBuffer );
  }
}


/**
 *  Simple helper to test for the expected values.
 *  We need to do this for every supported type.
 **/
template<typename T>
void TestRandomPackingHelper( char* &inBuffer, T expected, unsigned char expectedSize, T* expectedArray )
{
  // Test expected random value.
  EXPECT_EQ(expected, *reinterpret_cast<T*>(inBuffer));
  inBuffer += sizeof(T);
  
  // Test expected array size.
  EXPECT_EQ(expectedSize, *reinterpret_cast<unsigned char*>(inBuffer));
  inBuffer += sizeof(char);
  
  // Test expected array values.
  for ( int i = 0; i < expectedSize; ++i )
  {
    EXPECT_EQ(expectedArray[i], *reinterpret_cast<T*>(inBuffer));
    inBuffer += sizeof(T);
  }
}

/**
 *  Test packing of all supported types in a single pack/unpack using randomly generated data.
 **/
TEST(TestMsgPack, TestRandomData)
{
  Anki::Util::RandomGenerator rand(12345);

  // Let's now test a random set of mixed types.
  const char rChar = rand.RandIntInRange( INT8_MIN, INT8_MAX );
  const short rShort = rand.RandIntInRange( INT16_MIN, INT16_MAX );
  const int rInt = rand.RandIntInRange( INT32_MIN, INT32_MAX );
  const long long int rLong = (long long int)rand.RandIntInRange( INT32_MIN, INT32_MAX );
  const float rFloat = rand.RandDblInRange( FLT_MIN, FLT_MAX );
  const double rDouble = rand.RandDblInRange( DBL_MIN, DBL_MAX );
  
  // Create a random array of chars
  const unsigned char cArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  char* cArray = new char[cArraySize];
  for ( int i = 0; i < cArraySize; ++i )
  {
    cArray[i] = rChar;
  }
  
  // Create a random array of shorts
  const unsigned char hArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  short* hArray = new short[hArraySize];
  for ( int i = 0; i < hArraySize; ++i )
  {
    hArray[i] = rShort;
  }
  
  // Create a random array of ints
  const unsigned char iArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  int* iArray = new int[iArraySize];
  for ( int i = 0; i < iArraySize; ++i )
  {
    iArray[i] = rInt;
  }
  
  // Create a random array of longs
  const unsigned char lArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  long long int* lArray = new long long int[lArraySize];
  for ( int i = 0; i < lArraySize; ++i )
  {
    lArray[i] = rLong;
  }
  
  // Create a random array of shorts
  const unsigned char fArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  float* fArray = new float[fArraySize];
  for ( int i = 0; i < fArraySize; ++i )
  {
    fArray[i] = rFloat;
  }
  
  // Create a random array of shorts
  const unsigned char dArraySize = rand.RandIntInRange( 0, UINT8_MAX );
  double* dArray = new double[dArraySize];
  for ( int i = 0; i < dArraySize; ++i )
  {
    dArray[i] = rDouble;
  }
  
  // Add up how many bytes we need to allocate for our source/destination buffers.
  // 1 byte for each array size + 1 sizeof(T) for the random value + X sizeof(T) for the array.
  unsigned int numBytes = 0;
  numBytes += 1 + ( ( 1 + cArraySize ) * sizeof( char ) );
  numBytes += 1 + ( ( 1 + hArraySize ) * sizeof( short ) );
  numBytes += 1 + ( ( 1 + iArraySize ) * sizeof( int ) );
  numBytes += 1 + ( ( 1 + lArraySize ) * sizeof( long long int ) );
  numBytes += 1 + ( ( 1 + fArraySize ) * sizeof( float ) );
  numBytes += 1 + ( ( 1 + dArraySize ) * sizeof( double ) );
  
  const char* packString = "cachahiailalfafdad";
  char* dstBuffer = new char[numBytes];
  
  // Test the packing of all of this random data.
  {
    memset( dstBuffer, 0, numBytes );
    unsigned int bytesWritten = 0;
    const UtilMsgError error = SafeUtilMsgPack( dstBuffer, numBytes, &bytesWritten, packString, rChar, cArraySize, cArray,
                                        rShort, hArraySize, hArray,
                                        rInt, iArraySize, iArray,
                                        rLong, lArraySize, lArray,
                                        rFloat, fArraySize, fArray,
                                        rDouble, dArraySize, dArray );
    uint32_t expectedSize = SafeUtilGetMsgSize( packString, rChar, cArraySize, cArray,
                                               rShort, hArraySize, hArray,
                                               rInt, iArraySize, iArray,
                                               rLong, lArraySize, lArray,
                                               rFloat, fArraySize, fArray,
                                               rDouble, dArraySize, dArray );

    EXPECT_EQ(bytesWritten, expectedSize);
    char* index = dstBuffer;
    TestRandomPackingHelper<char>( index, rChar, cArraySize, cArray );
    TestRandomPackingHelper<short>( index, rShort, hArraySize, hArray );
    TestRandomPackingHelper<int>( index, rInt, iArraySize, iArray );
    TestRandomPackingHelper<long long int>( index, rLong, lArraySize, lArray );
    TestRandomPackingHelper<float>( index, rFloat, fArraySize, fArray );
    TestRandomPackingHelper<double>( index, rDouble, dArraySize, dArray );
    
    EXPECT_EQ(UTILMSG_OK, error);
    EXPECT_EQ(numBytes, bytesWritten);
  }
  
  // Test the unpacking of all of this random data.
  {
    char cExpected;
    char* cExpectedArray = new char[cArraySize];
    unsigned char cExpectedSize;
    
    short hExpected;
    short* hExpectedArray = new short[hArraySize];
    unsigned char hExpectedSize;
    
    int iExpected;
    int* iExpectedArray = new int[iArraySize];
    unsigned char iExpectedSize;
    
    long long int lExpected;
    long long int* lExpectedArray = new long long int[lArraySize];
    unsigned char lExpectedSize;
    
    float fExpected;
    float* fExpectedArray = new float[fArraySize];
    unsigned char fExpectedSize;
    
    double dExpected;
    double* dExpectedArray = new double[dArraySize];
    unsigned char dExpectedSize;
    
    unsigned int bytesRead = 0;
    const UtilMsgError error = SafeUtilMsgUnpack( dstBuffer, numBytes, &bytesRead, packString, &cExpected, &cExpectedSize, cExpectedArray,
                                          &hExpected, &hExpectedSize, hExpectedArray,
                                          &iExpected, &iExpectedSize, iExpectedArray,
                                          &lExpected, &lExpectedSize, lExpectedArray,
                                          &fExpected, &fExpectedSize, fExpectedArray,
                                          &dExpected, &dExpectedSize, dExpectedArray );
    
    EXPECT_EQ(UTILMSG_OK, error);
    EXPECT_EQ(numBytes, bytesRead);
    
    EXPECT_EQ(rChar, cExpected);
    EXPECT_EQ(cArraySize, cExpectedSize);
    for ( int i = 0; i < cArraySize; ++i )
    {
      EXPECT_EQ(cArray[i], cExpectedArray[i]);
    }
    
    EXPECT_EQ(rShort, hExpected);
    EXPECT_EQ(hArraySize, hExpectedSize);
    for ( int i = 0; i < hArraySize; ++i )
    {
      EXPECT_EQ(hArray[i], hExpectedArray[i]);
    }
    
    EXPECT_EQ(rInt, iExpected);
    EXPECT_EQ(iArraySize, iExpectedSize);
    for ( int i = 0; i < iArraySize; ++i )
    {
      EXPECT_EQ(iArray[i], iExpectedArray[i]);
    }
    
    EXPECT_EQ(rLong, lExpected);
    EXPECT_EQ(lArraySize, lExpectedSize);
    for ( int i = 0; i < lArraySize; ++i )
    {
      EXPECT_EQ(lArray[i], lExpectedArray[i]);
    }
    
    EXPECT_EQ(rFloat, fExpected);
    EXPECT_EQ(fArraySize, fExpectedSize);
    for ( int i = 0; i < fArraySize; ++i )
    {
      EXPECT_EQ(fArray[i], fExpectedArray[i]);
    }
    
    EXPECT_EQ(rDouble, dExpected);
    EXPECT_EQ(dArraySize, dExpectedSize);
    for ( int i = 0; i < dArraySize; ++i )
    {
      EXPECT_EQ(dArray[i], dExpectedArray[i]);
    }
    
    delete[] cExpectedArray;
    delete[] hExpectedArray;
    delete[] iExpectedArray;
    delete[] lExpectedArray;
    delete[] fExpectedArray;
    delete[] dExpectedArray;
  }
  
  delete[] cArray;
  delete[] hArray;
  delete[] iArray;
  delete[] lArray;
  delete[] fArray;
  delete[] dArray;
  
  delete[] dstBuffer;
}

/*
 *  Simple test to make sure the number of bytes written/read is being reported accurately.
 *  The other unit tests also incorporate checking for bytes written/read so I'm keeping this simple.
 */
TEST(TestMsgPack, TestPackingSize)
{
  char cValue = 0;
  short hValue = 0;
  int iValue = 0;
  long long int lValue = 0;
  float fValue = 0.0f;
  double dValue = 0.0;
  
  const int BUFFER_SIZE = 1024;
  char dstBuffer[1024];
  
  const int bytesExpected = sizeof(char) + sizeof(short) + sizeof(int) + sizeof(long long int) + sizeof(float) + sizeof(double);
  UtilMsgError error = UTILMSG_OK;
  
  unsigned int bytesWritten = 0;
  uint32_t expectedSize = 0;
  error = SafeUtilMsgPack(dstBuffer, BUFFER_SIZE, &bytesWritten, "chilfd", cValue, hValue, iValue, lValue, fValue, dValue);
  expectedSize = SafeUtilGetMsgSize("chilfd", cValue, hValue, iValue, lValue, fValue, dValue);
  EXPECT_EQ(expectedSize, bytesWritten);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(bytesExpected, bytesWritten);
  
  unsigned int bytesRead = 0;
  error = SafeUtilMsgUnpack(dstBuffer, BUFFER_SIZE, &bytesRead, "chilfd", &cValue, &hValue, &iValue, &lValue, &fValue, &dValue);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(bytesExpected, bytesRead);
  
  // Check for a zero sized array.
  error = SafeUtilMsgPack(dstBuffer, BUFFER_SIZE, &bytesWritten, "ac", 0, cValue);
  expectedSize = SafeUtilGetMsgSize("ac", 0, cValue);
  EXPECT_EQ(expectedSize, bytesWritten);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(1, bytesWritten);
  
  unsigned char size = 0;
  error = SafeUtilMsgUnpack(dstBuffer, BUFFER_SIZE, &bytesRead, "ac", &size, &cValue);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(1, bytesRead);
  EXPECT_EQ(0, size);
}


template <typename T>
void EXPECT_EQ_ALLOW_NANS(T a, T b)
{
  if (isnan(a) && isnan(b))
  {
    // Both values are NaN - normal EXPECT_EQ test would fail
  }
  else
  {
    EXPECT_EQ(a, b);
  }
}


/*
 *  Test to ensure that our buffer overrun detection is working correctly.
 */
TEST(TestMsgPack, TestSafetyChecking)
{
  const int BUFFER_SIZE = 1024;
  char srcBuffer[BUFFER_SIZE];
  char dstBuffer[BUFFER_SIZE];
  
  RandomizeByteArray( srcBuffer, BUFFER_SIZE );
  
  unsigned int bytesPacked = 0;
  unsigned int bytesRead = 0;
  unsigned char expectedSize;
  UtilMsgError error = UTILMSG_OK;
  
  // NULL buffer ...
  error = SafeUtilMsgPack(NULL, BUFFER_SIZE, &bytesPacked, "c");
  EXPECT_EQ(UTILMSG_ZEROBUFFER, error);
  EXPECT_EQ(0, bytesPacked);
  error = SafeUtilMsgUnpack(NULL, BUFFER_SIZE, &bytesRead, "c");
  EXPECT_EQ(UTILMSG_ZEROBUFFER, error);
  EXPECT_EQ(0, bytesRead);
  
  // Zero length buffer ...
  error = SafeUtilMsgPack(dstBuffer, 0, &bytesPacked, "c");
  EXPECT_EQ(UTILMSG_ZEROBUFFER, error);
  EXPECT_EQ(0, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 0, &bytesRead, "c");
  EXPECT_EQ(UTILMSG_ZEROBUFFER, error);
  EXPECT_EQ(0, bytesRead);
  
  // Negative array size ...
  error = SafeUtilMsgPack(dstBuffer, BUFFER_SIZE, &bytesPacked, "ac", -1, srcBuffer);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, BUFFER_SIZE, &bytesRead, "ac", &expectedSize, srcBuffer);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(0, expectedSize);
  
  // Too large array size ...
  error = SafeUtilMsgPack(dstBuffer, BUFFER_SIZE, &bytesPacked, "ac", 256, srcBuffer);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(256, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, BUFFER_SIZE, &bytesRead, "ac", &expectedSize, srcBuffer);
  EXPECT_EQ(UTILMSG_OK, error);
  EXPECT_EQ(255, expectedSize);
  
  // Buffer overrun: Writing/Reading array size ...
  error = SafeUtilMsgPack(dstBuffer, 1, &bytesPacked, "cac", srcBuffer[0], 1, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 1, &bytesRead, "cac", &srcBuffer[0], &expectedSize, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesRead);
  
  // Buffer overrun: Writing/Reading char ...
  error = SafeUtilMsgPack(dstBuffer, 1, &bytesPacked, "acc", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 1, &bytesRead, "cc", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesRead);
  EXPECT_EQ(dstBuffer[0], srcBuffer[0]);
  
  // Buffer overrun: Writing/Reading short ...
  error = SafeUtilMsgPack(dstBuffer, 2, &bytesPacked, "ahc", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 2, &bytesRead, "hh", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(2, bytesRead);
  EXPECT_EQ(*(short*)dstBuffer, *(short*)srcBuffer);
  
  // Buffer overrun: Writing/Reading int ...
  error = SafeUtilMsgPack(dstBuffer, 4, &bytesPacked, "aic", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 4, &bytesRead, "ii", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(4, bytesRead);
  EXPECT_EQ(*(int*)dstBuffer, *(int*)srcBuffer);
  
  // Buffer overrun: Writing/Reading long ...
  error = SafeUtilMsgPack(dstBuffer, 8, &bytesPacked, "alc", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 8, &bytesRead, "ll", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(8, bytesRead);
  EXPECT_EQ(*(long long int*)dstBuffer, *(long long int*)srcBuffer);
  
  // Buffer overrun: Writing/Reading float ...
  error = SafeUtilMsgPack(dstBuffer, 4, &bytesPacked, "afc", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 4, &bytesRead, "ff", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(4, bytesRead);
  EXPECT_EQ_ALLOW_NANS(*(float*)dstBuffer, *(float*)srcBuffer);
  
  // Buffer overrun: Writing/Reading double ...
  error = SafeUtilMsgPack(dstBuffer, 8, &bytesPacked, "adc", BUFFER_SIZE, srcBuffer, srcBuffer[0]);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(1, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, 8, &bytesRead, "dd", srcBuffer, srcBuffer);
  EXPECT_EQ(UTILMSG_BUFFEROVRN, error);
  EXPECT_EQ(8, bytesRead);
  EXPECT_EQ_ALLOW_NANS(*(double*)dstBuffer, *(double*)srcBuffer);
  
  error = SafeUtilMsgPack(dstBuffer, BUFFER_SIZE, &bytesPacked, "z", srcBuffer);
  EXPECT_EQ(UTILMSG_INVALIDARG, error);
  EXPECT_EQ(0, bytesPacked);
  error = SafeUtilMsgUnpack(dstBuffer, BUFFER_SIZE, &bytesRead, "az", &expectedSize, srcBuffer);
  EXPECT_EQ(UTILMSG_INVALIDARG, error);
  EXPECT_EQ(1, bytesRead);
}
