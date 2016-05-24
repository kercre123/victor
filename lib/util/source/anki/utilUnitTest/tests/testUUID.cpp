/**
 * File: testUUID
 *
 * Author: chapados
 * Created: 8/11/13
 *
 * Description: test for UUID.
 * --gtest_filter=*TestUUID*
 * GTEST_FILTER="*TestUUID*"
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "util/UUID/UUID.h"
#include "util/helpers/includeGTest.h"

class TestUUID : public ::testing::Test {
};

TEST_F(TestUUID, UUIDCreateEmpty)
{
  UUIDBytesRef emptyUUIDBytes = UUIDEmpty();
  const unsigned char* bytes = emptyUUIDBytes->bytes;
  
  bool isEmpty = true;
  for (int i = 0; i < sizeof(UUIDBytes); ++i) {
    isEmpty = ( bytes[i] == 0 );
    if ( !isEmpty )
      break;
  }
  
  ASSERT_TRUE(isEmpty) << "UUIEmpty() should return empty UUIDBytes";
}

TEST_F(TestUUID, UUIDCompare)
{
  UUIDBytes expectedUUID = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15}};
  
  UUIDBytes uuid0_10 = {{0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15}};
  UUIDBytes uuid15_0 = {{0x0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x0}};
  UUIDBytes uuid5_0 = {{0x0, 0x01, 0x02, 0x03, 0x04, 0x0, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x0}};
  
  ASSERT_EQ(UUIDCompare(&expectedUUID, &uuid0_10), expectedUUID.bytes[0] - uuid0_10.bytes[0]) << "Should differ by 10 at bytes 0";
  ASSERT_EQ(UUIDCompare(&expectedUUID, &uuid15_0), expectedUUID.bytes[15] - uuid15_0.bytes[15]) << "Should differ by 15 at byte 15";
  ASSERT_EQ(UUIDCompare(&expectedUUID, &uuid5_0), expectedUUID.bytes[5] - uuid5_0.bytes[5]) << "Should differ by 5 at byte 5";
}

TEST_F(TestUUID, UUIDCopy)
{
  UUIDBytes sourceBytes = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15}};
 
  UUIDBytesRef sourceUUID = &sourceBytes;
  
  UUIDBytes copyBytes;
  UUIDBytesRef UUIDcopy = &copyBytes;
  UUIDCopy(UUIDcopy, sourceUUID);
 
  ASSERT_TRUE(UUIDCompare(sourceUUID, UUIDcopy) == 0) << "Copy should be identical to source";
  
  // Should not be able to copy into empty UUID
  // BRC: Should UUIDCopy() assert if the destination == UUIDEmpty()
  // or if destination is a const UUIDBytesRef?
  UUIDBytesRef empty = UUIDEmpty();
  UUIDCopy(empty, sourceUUID);
  ASSERT_EQ(empty, UUIDEmpty()) << "UUIDEmpty() returns the same read-only pointer";
  ASSERT_EQ(UUIDCompare(UUIDEmpty(), empty), 0) << "UUIDCopy() cannot modify an empty UUID";
}


TEST_F(TestUUID, UUIDFromString)
{
  UUIDBytes expectedUUID = { {0x87, 0x5e, 0x95, 0x53, 0x17, 0xd0, 0x44, 0xbc, 0x8c, 0x49, 0xda, 0x77, 0x74, 0xda, 0x76, 0xb7} };
  const char * uuidString = "875E9553-17D0-44BC-8C49-DA7774DA76B7";
  
  UUIDBytes uuid;
  int success = -1;
  success = UUIDBytesFromString(&uuid, uuidString);
  
  ASSERT_EQ(success, 0) << "Failed parsing UUID string";
  ASSERT_TRUE(UUIDCompare(&uuid, &expectedUUID) == 0) << "UUID bytes should be equal";

  // If string is shorter by 1 char:
  // Last byte should be different, but bytes parsed should be the same.
  const char * invalidUUIDString1 = "875E9553-17D0-44BC-8C49-DA7774DA76B";
  UUIDBytes invalidUUID1;
  success = 0;
  success = UUIDBytesFromString(&invalidUUID1, invalidUUIDString1);
  
  ASSERT_EQ(success, -1) << "Strings 1 differ by 1 char, parsing should fail";
  ASSERT_EQ(UUIDCompare(&invalidUUID1, &expectedUUID), invalidUUID1.bytes[15]-expectedUUID.bytes[15]);
  
  // If string is shorted by 2 char:
  // Count of parsed characters should be different.
  const char * invalidUUIDString2 = "875E9553-17D0-44BC-8C49-DA7774DA76";
  UUIDBytes invalidUUID2;
  success = 0;
  success = UUIDBytesFromString(&invalidUUID2, invalidUUIDString2);
  ASSERT_EQ(success, -1) << "Should have failed parsing UUID";
}

TEST_F(TestUUID, StringFromUUID)
{
  UUIDBytes uuidBytes = { {0x87, 0x5e, 0x95, 0x53, 0x17, 0xd0, 0x44, 0xbc, 0x8c, 0x49, 0xda, 0x77, 0x74, 0xda, 0x76, 0xb7} };
  UUIDBytesRef uuid = &uuidBytes;
  const char * expectedUUIDString = "875E9553-17D0-44BC-8C49-DA7774DA76B7";
  
  const char * uuidString = StringFromUUIDBytes(uuid);
  ASSERT_EQ(strncmp(uuidString, expectedUUIDString, strlen(expectedUUIDString)), 0);
  
  // UUIDBytes only has 15 bytes:
  // We print an error message, but a string is still returned,
  // and the caller has no indication that there was an error!
  // We should probably return NULL if the conversion fails.
  UUIDBytes invalidUUIDBytes15 = { {0x87, 0x5e, 0x95, 0x53, 0x17, 0xd0, 0x44, 0xbc, 0x8c, 0x49, 0xda, 0x77, 0x74, 0xda, 0x76} };
  const char * invalidUUIDString15 = StringFromUUIDBytes(&invalidUUIDBytes15);
  ASSERT_NE(strncmp(invalidUUIDString15, expectedUUIDString, strlen(expectedUUIDString)), 0);
}
