/**
 * File: testNumericCast
 *
 * Author: Mark Wesley
 * Created: 11/18/14
 *
 * Description: Unit tests for numeric_cast<> and related code
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/helpers/arrayHelpers.h"


// Enable this to generate code and tables for most of the tests
#define ENABLE_NUMERIC_CAST_CODE_GEN  0


static const int64_t kInt64Lowest  = std::numeric_limits<int64_t>::lowest();
static const float   kFloatLowest  = std::numeric_limits<float>::lowest();
static const float   kFloatMax     = std::numeric_limits<float>::max();
static const double  kDoubleLowest = std::numeric_limits<double>::lowest();
static const double  kDoubleMax    = std::numeric_limits<double>::max();


template <typename ToType>
void TestEqual(ToType expectedResult, ToType actualResult)
{
  EXPECT_EQ(expectedResult, actualResult);
}


void TestEqualDouble(double expectedResult, double actualResult)
{
  // Due to floating-point precision loss, especially around conversion to/from strings, we must compare using a relative epsilon value
  
  using namespace Anki::Util;
  
  const double diff = expectedResult - actualResult;
  // scale epsilon by smaller of the 2 absolute values - that way if both values are similarly large we compare against a larger epsilon
  const double smallestAbsValue = Min(Abs(expectedResult), Abs(actualResult));
  const double relativeEpsilon = 0.000001 * Max(1.0, smallestAbsValue);
  const bool diffGTEpsilon = (Abs(diff) > relativeEpsilon);
  
  EXPECT_FALSE(diffGTEpsilon);
}


template <>
void TestEqual(float expectedResult, float actualResult)
{
  TestEqualDouble(expectedResult, actualResult);
}


template <>
void TestEqual(double expectedResult, double actualResult)
{
  TestEqualDouble(expectedResult, actualResult);
}

  
template <typename ToType, typename FromType>
void TestNumericCastAgaintExpected(FromType fromValue, ToType expectedResult, bool expectedIsValid)
{
  EXPECT_EQ(expectedIsValid, Anki::Util::IsValidNumericCast<ToType>(fromValue));
  
  TestEqual(expectedResult, Anki::Util::numeric_cast_clamped<ToType>(fromValue));
  
  if (expectedIsValid)
  {
    TestEqual(expectedResult, Anki::Util::numeric_cast<ToType>(fromValue));
  }
}


template <typename FromType>
static const char* GetTypeName()
{
  assert(0);
  return "UNKNOWN";
}


#define GET_TYPENAME_SPECIALIZATION(type) template <>                     \
                                          const char* GetTypeName<type>() \
                                          {                               \
                                            return #type;                 \
                                          }

GET_TYPENAME_SPECIALIZATION(uint8_t);
GET_TYPENAME_SPECIALIZATION(uint16_t);
GET_TYPENAME_SPECIALIZATION(uint32_t);
GET_TYPENAME_SPECIALIZATION(uint64_t);

GET_TYPENAME_SPECIALIZATION(int8_t);
GET_TYPENAME_SPECIALIZATION(int16_t);
GET_TYPENAME_SPECIALIZATION(int32_t);
GET_TYPENAME_SPECIALIZATION(int64_t);

GET_TYPENAME_SPECIALIZATION(float);
GET_TYPENAME_SPECIALIZATION(double);


template <typename FromType>
static void PrintValue(FromType value, bool isForCodeGen=true)
{
  typedef std::numeric_limits<FromType> FromTypeLimits;
  assert(FromTypeLimits::is_integer);
  
  if (isForCodeGen && (value < 0) && (int64_t(value) == kInt64Lowest))
  {
    // workaround for compiler not liking the smallest i64 value if written literally
    printf("kInt64Lowest");
    return;
  }
  
  if (FromTypeLimits::is_signed)
  {
    printf("%lld", (int64_t)value);
  }
  else
  {
    printf("%llu", (uint64_t)value);
  }
  // any int literals larger than biggest int64 need to specify ull
  if (isForCodeGen && (value > 0) && (uint64_t(value) > uint64_t(std::numeric_limits<int64_t>::max())))
  {
    printf("ull");
  }
}


template <>
void PrintValue(float value, bool isForCodeGen)
{
  if (isForCodeGen && (value == kFloatLowest))
  {
    printf("kFloatLowest");
  }
  else if (isForCodeGen && (value == kFloatMax))
  {
    printf("kFloatMax");
  }
  else if (Anki::Util::Abs(value) > 10000000000000000000.0f)
  {
    printf("%e", value);
  }
  else
  {
    printf("%ff", value);
  }
}


template <>
void PrintValue(double value, bool isForCodeGen)
{
  if (isForCodeGen && (value == kDoubleLowest))
  {
    printf("kDoubleLowest");
  }
  else if (isForCodeGen && (value == kDoubleMax))
  {
    printf("kDoubleMax");
  }
  else if (Anki::Util::Abs(value) > 10000000000000000000.0)
  {
    printf("%e", value);
  }
  else
  {
    printf("%f", value);
  }
}


template <typename FromType, typename ToType>
void TestNumericCastValues(const FromType* fromValues, const ToType* expectedToValues, const bool* expectedValidValues, size_t numValues)
{
  assert(fromValues[0          ] == std::numeric_limits<FromType>::lowest());
  assert(fromValues[numValues-1] == std::numeric_limits<FromType>::max());
  
  FromType fromValue = 0;

  for (size_t vI = 0; vI < numValues; ++vI)
  {
    assert((vI == 0) || (fromValues[vI] > fromValue));
    fromValue = fromValues[vI];
    TestNumericCastAgaintExpected<ToType>(fromValue, expectedToValues[vI], expectedValidValues[vI]);
  }
  
  printf("Tested: %s to %s: %zu values, range: ", GetTypeName<FromType>(), GetTypeName<ToType>(), numValues);
  PrintValue(fromValues[0], false);
  printf("..");
  PrintValue(fromValue, false);
  printf("\n");
}


void TestNumericCastFrom_int8_t()
{
  // Test every possible value of an int8 being cast into every other numeric type
  
  typedef int8_t FromType;
  typedef std::numeric_limits<FromType> FromTypeLimits;
  
  FromType i = FromTypeLimits::lowest();
  FromType lastI = i;
  int numValues = 0;
  do
  {
    lastI = i;
    
    // -> Signed Ints
    TestNumericCastAgaintExpected<int8_t >(i, i, true);
    TestNumericCastAgaintExpected<int16_t>(i, i, true);
    TestNumericCastAgaintExpected<int32_t>(i, i, true);
    TestNumericCastAgaintExpected<int64_t>(i, i, true);
    
    // -> fp types
    TestNumericCastAgaintExpected<float >(i, i, true);
    TestNumericCastAgaintExpected<double>(i, i, true);
    
    // -> Unsigned Ints
    if (i < 0)
    {
      TestNumericCastAgaintExpected<uint8_t >(i, 0, false);
      TestNumericCastAgaintExpected<uint16_t>(i, 0, false);
      TestNumericCastAgaintExpected<uint32_t>(i, 0, false);
      TestNumericCastAgaintExpected<uint64_t>(i, 0, false);
    }
    else
    {
      TestNumericCastAgaintExpected<uint8_t >(i, i, true);
      TestNumericCastAgaintExpected<uint16_t>(i, i, true);
      TestNumericCastAgaintExpected<uint32_t>(i, i, true);
      TestNumericCastAgaintExpected<uint64_t>(i, i, true);
    }
    ++numValues;
  }
  while (i++ != FromTypeLimits::max());
  
  assert(lastI == FromTypeLimits::max());
  printf("Tested: int8 to all: %d values, range: %d..%d\n", numValues, FromTypeLimits::lowest(), lastI);
}


void TestNumericCastFrom_uint8_t()
{
  // Test every possible value of a uint8 being cast into every other numeric type
  
  typedef uint8_t FromType;
  typedef std::numeric_limits<FromType> FromTypeLimits;
  
  FromType i = FromTypeLimits::lowest();
  FromType lastI = i;
  int numValues = 0;
  do
  {
    lastI = i;
    
    // -> Signed Ints
    if (i < 128)
    {
      TestNumericCastAgaintExpected<int8_t >(i, i, true);
    }
    else
    {
      TestNumericCastAgaintExpected<int8_t >(i, 127, false);
    }
    
    TestNumericCastAgaintExpected<int16_t>(i, i, true);
    TestNumericCastAgaintExpected<int32_t>(i, i, true);
    TestNumericCastAgaintExpected<int64_t>(i, i, true);
    
    // -> fp types
    TestNumericCastAgaintExpected<float >(i, i, true);
    TestNumericCastAgaintExpected<double>(i, i, true);
    
    // -> Unsigned Ints
    TestNumericCastAgaintExpected<uint8_t >(i, i, true);
    TestNumericCastAgaintExpected<uint16_t>(i, i, true);
    TestNumericCastAgaintExpected<uint32_t>(i, i, true);
    TestNumericCastAgaintExpected<uint64_t>(i, i, true);
    ++numValues;
  }
  while (i++ != FromTypeLimits::max());
  
  assert(lastI == FromTypeLimits::max());
  printf("Tested: uint8 to all: %d values, range %u..%u\n", numValues, FromTypeLimits::lowest(), lastI);
}


// Tables etc. for other tests are generated from code and places in a separate file
#include "testNumericCastGeneratedCode.h"


// --gtest_filter=TestMath.TestNumericCast
TEST(TestMath, TestNumericCast)
{
  #if ENABLE_NUMERIC_CAST_CODE_GEN
  {
    void CodeGenTestNumericCastAll();
    CodeGenTestNumericCastAll();
  }
  #endif // ENABLE_NUMERIC_CAST_CODE_GEN
  
  TestNumericCastFrom_int8_t();
  TestNumericCastFrom_int16_t();
  TestNumericCastFrom_int32_t();
  TestNumericCastFrom_int64_t();
  
  TestNumericCastFrom_uint8_t();
  TestNumericCastFrom_uint16_t();
  TestNumericCastFrom_uint32_t();
  TestNumericCastFrom_uint64_t();
  
  TestNumericCastFrom_float();
  TestNumericCastFrom_double();
}


#if ENABLE_NUMERIC_CAST_CODE_GEN
template <typename ToType, typename FromType>
void CodeGenTestNumericCastTo(const FromType* fromValues, size_t numValues)
{
  using namespace Anki::Util;
  
  typedef std::numeric_limits<FromType> FromTypeLimits;
  const char* toTypeName = GetTypeName<ToType>();
  FromType fromValue = 0;
  
  printf("  // to: %s\n", toTypeName);
  printf("  {\n");
  printf("    %s expectedValues_%s[] = { ", toTypeName, toTypeName);
  for (size_t vI = 0; vI < numValues; ++vI)
  {
    assert((vI == 0) || (fromValues[vI] > fromValue));
    fromValue = fromValues[vI];
    
    const ToType expectedVal = numeric_cast_clamped<ToType>(fromValue);
    
    if (vI != 0)
    {
      printf(", ");
    }
    if ((vI % 5) == 0)
    {
      printf("\n          ");
    }
    PrintValue(expectedVal);
  }
  printf("    };\n");
  printf("\n");
  printf("    bool expectedValid_%s[] = { ", toTypeName );
  for (size_t vI = 0; vI < numValues; ++vI)
  {
    assert((vI == 0) || (fromValues[vI] > fromValue));
    fromValue = fromValues[vI];
    
    const bool expectedVal = IsValidNumericCast<ToType>(fromValue);
    
    if (vI != 0)
    {
      printf(", ");
    }
    if ((vI % 5) == 0)
    {
      printf("\n          ");
    }
    printf("%s", expectedVal ? "true" : "false");
  }
  
  printf("    };\n");
  printf("\n");
  printf("    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_%s), \"Array Sizes Differ!\");\n", toTypeName);
  printf("    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_%s),  \"Array Sizes Differ!\");\n", toTypeName);
  printf("\n");
  printf("    TestNumericCastValues(fromValues, expectedValues_%s, expectedValid_%s, Anki::Util::array_size(fromValues));\n", toTypeName, toTypeName);
  printf("  }\n");
}


template <typename FromType>
void CodeGenTestNumericCastFrom(const FromType* fromValues, size_t numValues)
{
  typedef std::numeric_limits<FromType> FromTypeLimits;
  
  assert(fromValues[0          ] == FromTypeLimits::lowest());
  assert(fromValues[numValues-1] == FromTypeLimits::max());
  
  printf("void TestNumericCastFrom_%s()\n", GetTypeName<FromType>());
  printf("{\n");
  printf("  const %s fromValues[] = { ", GetTypeName<FromType>());
  for (size_t vI = 0; vI < numValues; ++vI)
  {
    if (vI != 0)
    {
      printf(", ");
    }
    if ((vI % 5) == 0)
    {
      printf("\n          ");
    }
    
    PrintValue(fromValues[vI]);
  }
  printf(" };\n\n");
  
  CodeGenTestNumericCastTo<uint8_t >(fromValues, numValues);
  CodeGenTestNumericCastTo<uint16_t>(fromValues, numValues);
  CodeGenTestNumericCastTo<uint32_t>(fromValues, numValues);
  CodeGenTestNumericCastTo<uint64_t>(fromValues, numValues);
  
  CodeGenTestNumericCastTo<int8_t >(fromValues, numValues);
  CodeGenTestNumericCastTo<int16_t>(fromValues, numValues);
  CodeGenTestNumericCastTo<int32_t>(fromValues, numValues);
  CodeGenTestNumericCastTo<int64_t>(fromValues, numValues);
  
  CodeGenTestNumericCastTo<float >(fromValues, numValues);
  CodeGenTestNumericCastTo<double>(fromValues, numValues);
  
  printf("}\n");
  printf("\n\n");
}


void CodeGenTestNumericCastAll()
{
  printf("// Auto-generated code from testNumericCast for generating tables of expected casts for each type->type for a representative set of values\n");
  printf("// See CodeGenTestNumericCastAll(), enabled via '#define ENABLE_NUMERIC_CAST_CODE_GEN 1'\n");
  printf("\n\n");
  
  // Use values around each signed/unsigned boundary (i.e. around +/- 2^(8n-1) and 2^(8n) for n=0,1,2,4,8)
  // 2^7  = 128
  // 2^8  = 256
  // 2^15 = 32768
  // 2^16 = 65536
  // 2^31 = 2147483648
  // 2^32 = 4294967296
  // 2^63 = 9223372036854775808
  // 2^64 = 18446744073709551616
  
  // int16 ->
  {
    typedef int16_t FromType;

    FromType values[] = {
                 -32768,     -32767,     -32766,   // -2^15
       -257,       -256,       -255,       -254,   // -2^8
       -129,       -128,       -127,       -126,   // -2^7
         -1,          0,          1,               //  0
        126,        127,        128,        129,   //  2^7
        254,        255,        256,        257,   //  2^8
      32766,      32767 };                         //  2^16
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  // int32 ->
  {
    typedef int32_t FromType;
    // Note: have to write "(-9223372036854775807)-1" instead of -9223372036854775808 to appease the compiler...
    FromType values[] = {
                 -2147483648,-2147483647,-2147483646,   // -2^31
          -65537,     -65536,     -65535,     -65534,   // -2^16
          -32769,     -32768,     -32767,     -32766,   // -2^15
            -257,       -256,       -255,       -254,   // -2^8
            -129,       -128,       -127,       -126,   // -2^7
              -1,          0,          1,               //  0
             126,        127,        128,        129,   //  2^7
             254,        255,        256,        257,   //  2^8
           32766,      32767,      32768,      32769,   //  2^15
           65534,      65535,      65536,      65537,   //  2^16
      2147483646, 2147483647 }; //  2^31
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  // int64 ->
  {
    typedef int64_t FromType;

    FromType values[] = {
                                 kInt64Lowest,kInt64Lowest+1,kInt64Lowest+2,   // -2^63
                    -2147483649, -2147483648,  -2147483647,  -2147483646,   // -2^31
                         -65537,      -65536,       -65535,       -65534,   // -2^16
                         -32769,      -32768,       -32767,       -32766,   // -2^15
                           -257,        -256,         -255,         -254,   // -2^8
                           -129,        -128,         -127,         -126,   // -2^7
                             -1,           0,            1,                 //  0
                            126,         127,          128,          129,   //  2^7
                            254,         255,          256,          257,   //  2^8
                          32766,       32767,        32768,        32769,   //  2^15
                          65534,       65535,        65536,        65537,   //  2^16
                     2147483646,  2147483647,   2147483648,   2147483649,   //  2^31
                     4294967294,  4294967295,   4294967296,   4294967297,   //  2^32
            9223372036854775806,  9223372036854775807 };                    //  2^63
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  {
    typedef uint16_t FromType;
    
    FromType values[] = {
                      0,          1,               //  0
        126,        127,        128,        129,   //  2^7
        254,        255,        256,        257,   //  2^8
      32766,      32767,      32768,      32769,   //  2^15
      65534,      65535 };                         //  2^16
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  {
    typedef uint32_t FromType;

    FromType values[] = {
                           0,          1,               //  0
             126,        127,        128,        129,   //  2^7
             254,        255,        256,        257,   //  2^8
           32766,      32767,      32768,      32769,   //  2^15
           65534,      65535,      65536,      65537,   //  2^16
      2147483646, 2147483647, 2147483648, 2147483649,   //  2^31
      4294967294, 4294967295 };                         //  2^32
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  {
    typedef uint64_t FromType;

    FromType values[] = {
                           0,          1,               //  0
             126,        127,        128,        129,   //  2^7
             254,        255,        256,        257,   //  2^8
           32766,      32767,      32768,      32769,   //  2^15
           65534,      65535,      65536,      65537,   //  2^16
      2147483646, 2147483647, 2147483648, 2147483649,   //  2^31
      4294967294, 4294967295, 4294967296, 4294967297,   //  2^32
      9223372036854775806, 9223372036854775807, 9223372036854775808ull, 9223372036854775809ull, //  2^63
      18446744073709551614ull, 18446744073709551615ull };               //  2^64
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  {
    typedef float FromType;
    typedef std::numeric_limits<FromType> FromTypeLimits;
    
    const float minPossibleValue = FromTypeLimits::lowest();
    const float maxPossibleValue = FromTypeLimits::max();
    FromType values[] = { minPossibleValue, -1.e38, -1.e29, -1.e20, -1.e11,
     -65537.0f,     -65536.0f,     -65535.0f,     -65534.0f,   // -2^16
     -32769.0f,     -32768.0f,     -32767.0f,     -32766.0f,   // -2^15
       -257.0f,       -256.0f,       -255.0f,       -254.0f,   // -2^8
       -129.0f,       -128.0f,       -127.0f,       -126.0f,   // -2^7
       -1.5f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 1.5f,            //  0
        126.0f,        127.0f,        128.0f,        129.0f,   //  2^7
        254.0f,        255.0f,        256.0f,        257.0f,   //  2^8
      32766.0f,      32767.0f,      32768.0f,      32769.0f,   //  2^15
      65534.0f,      65535.0f,      65536.0f,      65537.0f,   //  2^16
      1.e11, 1.e20, 1.e29, 1.e38, maxPossibleValue };
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
  
  {
    typedef double FromType;
    typedef std::numeric_limits<FromType> FromTypeLimits;
    const FromType minPossibleValue = FromTypeLimits::lowest();
    const FromType maxPossibleValue = FromTypeLimits::max();
    
    FromType values[] = { minPossibleValue, -1.e300, -1.e200, -1.e100, -1.e40, -1.e39, -1.e38, -1.e29, -1.e20, -1.e11,
      -65537.0,     -65536.0,     -65535.0,     -65534.0, // -2^16
      -32769.0,     -32768.0,     -32767.0,     -32766.0, // -2^15
      -257.0,       -256.0,       -255.0,       -254.0,   // -2^8
      -129.0,       -128.0,       -127.0,       -126.0,   // -2^7
      -1.5, -1.0, -0.5, 0.0, 0.5, 1.0, 1.5,               //  0
      126.0,        127.0,        128.0,        129.0,    //  2^7
      254.0,        255.0,        256.0,        257.0,    //  2^8
      32766.0,      32767.0,      32768.0,      32769.0,  //  2^15
      65534.0,      65535.0,      65536.0,      65537.0,  //  2^16
      1.e11, 1.e20, 1.e29, 1.e38, 1.e39, 1.e40, 1.e100, 1.e200, 1.e300, maxPossibleValue };
    
    CodeGenTestNumericCastFrom(values, Anki::Util::array_size(values));
  }
}
#endif // ENABLE_NUMERIC_CAST_CODE_GEN

