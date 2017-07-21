/**
 * File: testMath
 *
 * Author: Mark Wesley
 * Created: 08/11/15
 *
 * Description: Unit tests for math.h
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/math/math.h"


// --gtest_filter=TestMath.TestClampedAdditionU8
TEST(TestMath, TestClampedAdditionU8)
{
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(  0), uint8_t(  0)),   0);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(127), uint8_t(127)), 254);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(127), uint8_t(128)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(128), uint8_t(128)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(129), uint8_t(129)), 255);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(253), uint8_t(  1)), 254);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(253), uint8_t(  2)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(253), uint8_t(  3)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(253), uint8_t(  4)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(  1), uint8_t(253)), 254);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(  2), uint8_t(253)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(  3), uint8_t(253)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(  4), uint8_t(253)), 255);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(155), uint8_t(155)), 255);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint8_t(255), uint8_t(255)), 255);
}


// --gtest_filter=TestMath.TestClampedAdditionU16
TEST(TestMath, TestClampedAdditionU16)
{
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(    0), uint16_t(    0)),     0);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(32767), uint16_t(32767)), 65534);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(32767), uint16_t(32768)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(32768), uint16_t(32768)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(32769), uint16_t(32769)), 65535);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(65533), uint16_t(    1)), 65534);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(65533), uint16_t(    2)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(65533), uint16_t(    3)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(65533), uint16_t(    4)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(    1), uint16_t(65533)), 65534);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(    2), uint16_t(65533)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(    3), uint16_t(65533)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(    4), uint16_t(65533)), 65535);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(44444), uint16_t(44444)), 65535);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint16_t(65535), uint16_t(65535)), 65535);
}


// --gtest_filter=TestMath.TestClampedAdditionU32
TEST(TestMath, TestClampedAdditionU32)
{
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(         0u), uint32_t(         0u)),          0u);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(2147483647u), uint32_t(2147483647u)), 4294967294u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(2147483647u), uint32_t(2147483648u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(2147483648u), uint32_t(2147483648u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(2147483649u), uint32_t(2147483649u)), 4294967295u);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(4294967293u), uint32_t(         1u)), 4294967294u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(4294967293u), uint32_t(         2u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(4294967293u), uint32_t(         3u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(4294967293u), uint32_t(         4u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(         1u), uint32_t(4294967293u)), 4294967294u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(         2u), uint32_t(4294967293u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(         3u), uint32_t(4294967293u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(         4u), uint32_t(4294967293u)), 4294967295u);

  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(3333333333u), uint32_t(3333333333u)), 4294967295u);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint32_t(4294967295u), uint32_t(4294967295u)), 4294967295u);
}


// --gtest_filter=TestMath.TestClampedAdditionU64
TEST(TestMath, TestClampedAdditionU64)
{
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(                   0ull), uint64_t(                   0ull)),                    0ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t( 9223372036854775807ull), uint64_t( 9223372036854775807ull)), 18446744073709551614ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t( 9223372036854775807ull), uint64_t( 9223372036854775808ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t( 9223372036854775808ull), uint64_t( 9223372036854775808ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t( 9223372036854775809ull), uint64_t( 9223372036854775809ull)), 18446744073709551615ull);

  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(18446744073709551613ull), uint64_t(                   1ull)), 18446744073709551614ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(18446744073709551613ull), uint64_t(                   2ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(18446744073709551613ull), uint64_t(                   3ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(18446744073709551613ull), uint64_t(                   3ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(                   1ull), uint64_t(18446744073709551613ull)), 18446744073709551614ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(                   2ull), uint64_t(18446744073709551613ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(                   3ull), uint64_t(18446744073709551613ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(                   4ull), uint64_t(18446744073709551613ull)), 18446744073709551615ull);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(11111111111111111111ull), uint64_t(11111111111111111111ull)), 18446744073709551615ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(uint64_t(18446744073709551615ull), uint64_t(18446744073709551615ull)), 18446744073709551615ull);
}


// --gtest_filter=TestMath.TestClampedAdditionI8
TEST(TestMath, TestClampedAdditionI8)
{
  using ValueType = int8_t;
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  0), ValueType(  0)),   0);
  
  // +ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( 63), ValueType( 63)), 126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( 63), ValueType( 64)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( 64), ValueType( 63)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( 64), ValueType( 64)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(125)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(126)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(126), ValueType(126)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(127), ValueType(127)), 127);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(  1)), 126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(  2)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(  3)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(125), ValueType(  4)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  1), ValueType(125)), 126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  2), ValueType(125)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  3), ValueType(125)), 127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  4), ValueType(125)), 127);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(127), ValueType(127)), 127);
  
  // -ve

  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -63), ValueType( -63)), -126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -63), ValueType( -64)), -127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -64), ValueType( -63)), -127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -64), ValueType( -64)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -64), ValueType( -65)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -65), ValueType( -64)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -65), ValueType( -65)), -128);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-125), ValueType(  -1)), -126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-125), ValueType(  -2)), -127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-125), ValueType(  -3)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-125), ValueType(  -4)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  -1), ValueType(-125)), -126);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  -2), ValueType(-125)), -127);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  -3), ValueType(-125)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  -4), ValueType(-125)), -128);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-127), ValueType(-127)), -128);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-128), ValueType(-128)), -128);
}


// --gtest_filter=TestMath.TestClampedAdditionI16
TEST(TestMath, TestClampedAdditionI16)
{
  using ValueType = int16_t;
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  0), ValueType(  0)),   0);
  
  // +ve

  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(16383), ValueType( 16383)), 32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(16383), ValueType( 16384)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(16384), ValueType( 16383)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(16384), ValueType( 16384)), 32767);

  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(32765), ValueType(    1)), 32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(32765), ValueType(    2)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(32765), ValueType(    3)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(32765), ValueType(    4)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    1), ValueType(32765)), 32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    2), ValueType(32765)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    3), ValueType(32765)), 32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    4), ValueType(32765)), 32767);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(32767), ValueType(32767)), 32767);
  
  // -ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16383), ValueType( -16383)), -32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16383), ValueType( -16384)), -32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16384), ValueType( -16383)), -32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16384), ValueType( -16384)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16384), ValueType( -16385)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16385), ValueType( -16384)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -16385), ValueType( -16385)), -32768);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32765), ValueType(    -1)), -32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32765), ValueType(    -2)), -32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32765), ValueType(    -3)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32765), ValueType(    -4)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -1), ValueType(-32765)), -32766);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -2), ValueType(-32765)), -32767);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -3), ValueType(-32765)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -4), ValueType(-32765)), -32768);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32767), ValueType(-32767)), -32768);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-32768), ValueType(-32768)), -32768);
}


// --gtest_filter=TestMath.TestClampedAdditionI32
TEST(TestMath, TestClampedAdditionI32)
{
  using ValueType = int32_t;
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  0), ValueType(  0)),   0);
  
  // +ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(1073741823), ValueType( 1073741823)), 2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(1073741823), ValueType( 1073741824)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(1073741824), ValueType( 1073741823)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(1073741824), ValueType( 1073741824)), 2147483647);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(2147483645), ValueType(         1)), 2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(2147483645), ValueType(         2)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(2147483645), ValueType(         3)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(2147483645), ValueType(         4)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         1), ValueType(2147483645)), 2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         2), ValueType(2147483645)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         3), ValueType(2147483645)), 2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         4), ValueType(2147483645)), 2147483647);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(2147483647), ValueType(2147483647)), 2147483647);
  
  // -ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741823), ValueType( -1073741823)), -2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741823), ValueType( -1073741824)), -2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741824), ValueType( -1073741823)), -2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741824), ValueType( -1073741824)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741824), ValueType( -1073741825)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741825), ValueType( -1073741824)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -1073741825), ValueType( -1073741825)), -2147483648);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483645), ValueType(         -1)), -2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483645), ValueType(         -2)), -2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483645), ValueType(         -3)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483645), ValueType(         -4)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         -1), ValueType(-2147483645)), -2147483646);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         -2), ValueType(-2147483645)), -2147483647);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         -3), ValueType(-2147483645)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(         -4), ValueType(-2147483645)), -2147483648);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483647), ValueType(-2147483647)), -2147483648);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-2147483648), ValueType(-2147483648)), -2147483648);
}


// --gtest_filter=TestMath.TestClampedAdditionI64
TEST(TestMath, TestClampedAdditionI64)
{
  using ValueType = int64_t;
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  0ull), ValueType(  0ull)),   0ull);
  
  // +ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(4611686018427387903ull), ValueType( 4611686018427387903ull)), 9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(4611686018427387903ull), ValueType( 4611686018427387904ull)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(4611686018427387904ull), ValueType( 4611686018427387903ull)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(4611686018427387904ull), ValueType( 4611686018427387904ull)), 9223372036854775807ull);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(9223372036854775805ull), ValueType(    1)), 9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(9223372036854775805ull), ValueType(    2)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(9223372036854775805ull), ValueType(    3)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(9223372036854775805ull), ValueType(    4)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    1), ValueType(9223372036854775805ull)), 9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    2), ValueType(9223372036854775805ull)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    3), ValueType(9223372036854775805ull)), 9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    4), ValueType(9223372036854775805ull)), 9223372036854775807ull);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(9223372036854775807), ValueType(9223372036854775807)), 9223372036854775807);
  
  // -ve
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387903ull), ValueType( -4611686018427387903ull)), -9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387903ull), ValueType( -4611686018427387904ull)), -9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387904ull), ValueType( -4611686018427387903ull)), -9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387904ull), ValueType( -4611686018427387904ull)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387904ull), ValueType( -4611686018427387905ull)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387905ull), ValueType( -4611686018427387904ull)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( -4611686018427387905ull), ValueType( -4611686018427387905ull)), -9223372036854775808ull);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775805ull), ValueType(    -1)), -9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775805ull), ValueType(    -2)), -9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775805ull), ValueType(    -3)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775805ull), ValueType(    -4)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -1), ValueType(-9223372036854775805ull)), -9223372036854775806ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -2), ValueType(-9223372036854775805ull)), -9223372036854775807ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -3), ValueType(-9223372036854775805ull)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    -4), ValueType(-9223372036854775805ull)), -9223372036854775808ull);
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775807ull), ValueType(-9223372036854775807ull)), -9223372036854775808ull);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-9223372036854775808ull), ValueType(-9223372036854775808ull)), -9223372036854775808ull);
}


// --gtest_filter=TestMath.TestClampedAdditionFloat
TEST(TestMath, TestClampedAdditionFloat)
{
  using ValueType = float;

  // Zero
  
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType(    0.0f), ValueType(    0.0f)),  0.0f);
  
  // Opposite Sign
  
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType( FLT_MAX), ValueType(-FLT_MAX)),  0.0f);
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType(   10.0f), ValueType(   -3.0f)),  7.0f);
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType(   -4.0f), ValueType(    2.0f)), -2.0f);
  
  // Same Sign no overflow
  
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType( 1.0f), ValueType( 1.0f)),  2.0f);
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType(-1.0f), ValueType(-1.0f)), -2.0f);

  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType( 2e38), ValueType( 1e38)),  3e38);
  EXPECT_FLOAT_EQ(Anki::Util::ClampedAddition(ValueType(-2e38), ValueType(-1e38)), -3e38);

  // Same Sign clamped
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(    3e38), ValueType(    1e38)),  FLT_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( FLT_MAX), ValueType( FLT_MAX)),  FLT_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(   -3e38), ValueType(   -1e38)), -FLT_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-FLT_MAX), ValueType(-FLT_MAX)), -FLT_MAX);
}


// --gtest_filter=TestMath.TestClampedAdditionDouble
TEST(TestMath, TestClampedAdditionDouble)
{
  using ValueType = double;
  
  // Zero
  
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(     0.0), ValueType(     0.0)),  0.0);
  
  // Opposite Sign
  
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType( FLT_MAX), ValueType(-FLT_MAX)),  0.0);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType( DBL_MAX), ValueType(-DBL_MAX)),  0.0);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(    10.0), ValueType(    -3.0)),  7.0);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(    -4.0), ValueType(     2.0)), -2.0);
  
  // Same Sign no overflow
  
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType( 1.0), ValueType( 1.0)),  2.0);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(-1.0), ValueType(-1.0)), -2.0);
  
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType( FLT_MAX), ValueType( FLT_MAX)),  6.8056469327705772e+38);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(-FLT_MAX), ValueType(-FLT_MAX)), -6.8056469327705772e+38);
  
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(   1e308), ValueType( 0.5e308)),  1.5e+308);
  EXPECT_DOUBLE_EQ(Anki::Util::ClampedAddition(ValueType(  -1e308), ValueType(-0.5e308)), -1.5e+308);
  
  // Same Sign clamped
  
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(   1e308), ValueType(   1e308)),  DBL_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType( DBL_MAX), ValueType( DBL_MAX)),  DBL_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(  -1e308), ValueType(  -1e308)), -DBL_MAX);
  EXPECT_EQ(Anki::Util::ClampedAddition(ValueType(-DBL_MAX), ValueType(-DBL_MAX)), -DBL_MAX);
}


// --gtest_filter=TestMath.TestWrapToMax
TEST(TestMath, TestWrapToMax)
{
  const float kEpsilon = 0.000001f;

  EXPECT_NEAR( Anki::Util::WrapToMax( -4.69f, 2.34f), 2.33f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -4.68f, 2.34f), 0.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -4.67f, 2.34f), 0.01f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -4.66f, 2.34f), 0.02f, kEpsilon );
  
  EXPECT_NEAR( Anki::Util::WrapToMax( -2.35f, 2.34f), 2.33f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -2.34f, 2.34f), 0.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -2.33f, 2.34f), 0.01f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -2.32f, 2.34f), 0.02f, kEpsilon );

  EXPECT_NEAR( Anki::Util::WrapToMax( -1.34f, 2.34f), 1.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -0.34f, 2.34f), 2.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -0.02f, 2.34f), 2.32f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax( -0.01f, 2.34f), 2.33f, kEpsilon );
  
  EXPECT_NEAR( Anki::Util::WrapToMax(  0.00f, 2.34f), 0.00f, kEpsilon );
  
  EXPECT_NEAR( Anki::Util::WrapToMax(  0.01f, 2.34f), 0.01f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  0.02f, 2.34f), 0.02f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  0.34f, 2.34f), 0.34f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  1.34f, 2.34f), 1.34f, kEpsilon );
  
  EXPECT_NEAR( Anki::Util::WrapToMax(  2.32f, 2.34f), 2.32f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  2.33f, 2.34f), 2.33f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  2.34f, 2.34f), 0.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  2.35f, 2.34f), 0.01f, kEpsilon );
  
  EXPECT_NEAR( Anki::Util::WrapToMax(  4.66f, 2.34f), 2.32f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  4.67f, 2.34f), 2.33f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  4.68f, 2.34f), 0.00f, kEpsilon );
  EXPECT_NEAR( Anki::Util::WrapToMax(  4.69f, 2.34f), 0.01f, kEpsilon );
}

TEST(TestMath, FloatZeroComparison)
{
  const float zeroEquiv = 1e-8f;
  
  // Test > 0
  EXPECT_TRUE( Anki::Util::IsFltGTZero(1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltGTZero(10.f));
  EXPECT_FALSE( Anki::Util::IsFltGTZero(0.f));
  EXPECT_FALSE( Anki::Util::IsFltGTZero(-1e-3f));
  EXPECT_FALSE( Anki::Util::IsFltGTZero(-zeroEquiv));
  EXPECT_FALSE( Anki::Util::IsFltGTZero(zeroEquiv));
  
  // Test >= 0
  EXPECT_TRUE( Anki::Util::IsFltGEZero(1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltGEZero(10.f));
  EXPECT_TRUE( Anki::Util::IsFltGEZero(0.f));
  EXPECT_FALSE( Anki::Util::IsFltGEZero(-1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltGEZero(-zeroEquiv));
  EXPECT_TRUE( Anki::Util::IsFltGEZero(zeroEquiv));
  
  // Test < 0
  EXPECT_TRUE( Anki::Util::IsFltLTZero(-1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltLTZero(-10.f));
  EXPECT_FALSE( Anki::Util::IsFltLTZero(0.f));
  EXPECT_FALSE( Anki::Util::IsFltLTZero(1e-3f));
  EXPECT_FALSE( Anki::Util::IsFltLTZero(-zeroEquiv));
  EXPECT_FALSE( Anki::Util::IsFltLTZero(zeroEquiv));
  
  // Test <= 0
  EXPECT_TRUE( Anki::Util::IsFltLEZero(-1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltLEZero(-10.f));
  EXPECT_TRUE( Anki::Util::IsFltLEZero(0.f));
  EXPECT_FALSE( Anki::Util::IsFltLEZero(1e-3f));
  EXPECT_TRUE( Anki::Util::IsFltLEZero(zeroEquiv));
  EXPECT_TRUE( Anki::Util::IsFltLEZero(-zeroEquiv));
  
  // Test "near" 0
  EXPECT_TRUE( Anki::Util::IsNearZero(0.f));
  EXPECT_TRUE( Anki::Util::IsNearZero(zeroEquiv));
  EXPECT_TRUE( Anki::Util::IsNearZero(-zeroEquiv));
  EXPECT_FALSE( Anki::Util::IsNearZero(.1f));
}


