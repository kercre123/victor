/**
 * File: testSharedCircularBuffer
 *
 * Author: Ron Barry
 * Created: 4/23/2019
 *
 * Description: Unit tests for SharedCircularBuffer
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=SharedCircularBuffer*
 **/


#include "util/container/sharedCircularBuffer.h"
#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"

TEST(SharedCircularBuffer, ReapWhatYouSow)
{
  Anki::Util::SharedCircularBuffer<uint32_t, 20> writer("ReapWhatYouSow", true);
  Anki::Util::SharedCircularBuffer<uint32_t, 20> reader("ReapWhatYouSow", false);

  EXPECT_EQ(reader.GetWritePtr(), nullptr);

  uint32_t* obj_ptr;
  
  //Make sure that reading ahead of the writer results in a squelch.
  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_PLEASE_WAIT);
  
  obj_ptr = writer.GetWritePtr();
  *obj_ptr = 1;
  writer.Advance();
  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_OKAY);
  EXPECT_EQ(*obj_ptr, 1);

  // Write-one, read-one, write-one, read-one...
  for (uint32_t i=0; i<100; ++i) {
    EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_PLEASE_WAIT);
    obj_ptr = writer.GetWritePtr();
    *obj_ptr = i;
    writer.Advance();
    EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_OKAY);
    EXPECT_EQ(*obj_ptr, i);
  }

  // Write a bunch so the reader is behind by a bit.
  const uint32_t diff=5;
  for (uint32_t i=0; i<diff; ++i) {
    obj_ptr = writer.GetWritePtr();
    *obj_ptr = i;
    writer.Advance();
  }
  // Write-one, read-one, write-one, read-one. Reads should be `diff` less than writes.
  for (uint32_t i=0; i<50; ++i) {
    EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_OKAY);
    EXPECT_EQ(*obj_ptr, i);
    obj_ptr = writer.GetWritePtr();
    *obj_ptr = i+diff;
    writer.Advance();
  }
  // Make sure we still get the last `diff` reads
  for (uint32_t i=50; i<50+diff; ++i) {
    EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_OKAY);
    EXPECT_EQ(*obj_ptr, i);
  }

  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_PLEASE_WAIT);
}

TEST(SharedCircularBuffer, LagWarnings)
{
  const int buffer_size = 20;

  Anki::Util::SharedCircularBuffer<uint32_t, buffer_size> writer("LagWarnings", true);
  Anki::Util::SharedCircularBuffer<uint32_t, buffer_size> reader("LagWarnings", false);

  uint32_t* obj_ptr;

  for (uint32_t i=0; i<buffer_size/2; ++i) {
    obj_ptr = writer.GetWritePtr();
    *obj_ptr = i;
    writer.Advance();
  }

  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_WARNING);
  EXPECT_EQ(*obj_ptr, 0);
  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_OKAY);
  EXPECT_EQ(*obj_ptr, 1);
}  

TEST(SharedCircularBuffer, LagBehind)
{
  const int buffer_size = 20;

  Anki::Util::SharedCircularBuffer<uint32_t, buffer_size> writer("LagBehind", true);
  Anki::Util::SharedCircularBuffer<uint32_t, buffer_size> reader("LagBehind", false);

  uint32_t* obj_ptr;

  uint32_t i;
  for (i=0; i<(3*buffer_size)/4; ++i) {
    obj_ptr = writer.GetWritePtr();
    *obj_ptr = i;
    writer.Advance();
  }

  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_BEHIND);
  EXPECT_EQ(*obj_ptr, i-1);
  EXPECT_EQ(reader.GetNext(&obj_ptr), Anki::Util::GET_STATE_PLEASE_WAIT);
  EXPECT_EQ(obj_ptr, nullptr);
}  