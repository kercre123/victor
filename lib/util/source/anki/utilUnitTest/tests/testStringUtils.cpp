//
//  testStringUtils.cpp
//
//  Created by Stuart Eichert on 11/11/15.
//  Copyright (c) 2015 Anki. All rights reserved.
//

#include "util/helpers/includeGTest.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Util {

class StringUtilsTest : public ::testing::Test
{
};

TEST_F(StringUtilsTest, UTF8EmptyString)
{
  ASSERT_TRUE(IsValidUTF8(std::string()));
  ASSERT_TRUE(IsValidUTF8(nullptr, 0));
  ASSERT_FALSE(IsValidUTF8(nullptr, 1));
}

TEST_F(StringUtilsTest, UTF8AsciiString)
{
  char allAscii[0x80] = {'\0'};
  for (int i = 1 ; i < 0x80 ; i++) {
    allAscii[i-1] = (char) i;
  }
  ASSERT_TRUE(IsValidUTF8(std::string(allAscii)));
}

TEST_F(StringUtilsTest, UTF8AccentedCharsString)
{
  std::string accentedChars = "äüćklãñd";
  ASSERT_TRUE(IsValidUTF8(accentedChars));
}

TEST_F(StringUtilsTest, UTF8ChineseCharsString)
{
  std::string chineseChars = "我是美国人";
  ASSERT_TRUE(IsValidUTF8(chineseChars));
}

TEST_F(StringUtilsTest, UTF8BadByteStrings)
{
  // Values above 0x7f that do not signify the start of a multibyte sequence are invalid
  for (int i = 0x80 ; i <= 0xff ; i++) {
    if (i != 0b11000000 && i != 0b11100000 && i != 0b11110000) {
      uint8_t buf[1];
      buf[0] = (uint8_t) i;
      ASSERT_FALSE(IsValidUTF8(buf, 1));
    }
  }
}

TEST_F(StringUtilsTest, UTF8ShortStrings)
{
  uint8_t twobyte[1] = {0b11000000};
  uint8_t threebyte[2] = {0b11100000, 0x80};
  uint8_t fourbyte[3] = {0b11110000, 0x80, 0x80};

  ASSERT_FALSE(IsValidUTF8(twobyte, 1));

  for (int i = 1; i <= sizeof(threebyte); i++) {
    ASSERT_FALSE(IsValidUTF8(threebyte, i));
  }

  for (int i = 1; i <= sizeof(fourbyte); i++) {
    ASSERT_FALSE(IsValidUTF8(fourbyte, i));
  }
}

TEST_F(StringUtilsTest, UTF85And6ByteStrings)
{
  uint8_t fivebyte[5] = {0b11111000, 0x80, 0x80, 0x80, 0x80};
  uint8_t sixbyte[6] = {0b11111100, 0x80, 0x80, 0x80, 0x80, 0x80};
  ASSERT_FALSE(IsValidUTF8(fivebyte, sizeof(fivebyte)));
  ASSERT_FALSE(IsValidUTF8(sixbyte, sizeof(sixbyte)));
}

TEST_F(StringUtilsTest, UTF8MaxCodePoint)
{
  uint8_t maxCodePoint[4] = {0xf4, 0x8f, 0xbf, 0xbf}; // 0x10FFFF
  ASSERT_TRUE(IsValidUTF8(maxCodePoint, sizeof(maxCodePoint)));

  uint8_t overMaxCodePoint[4] = {0xf4, 0x90, 0x80, 0x80}; // 0x110000
  ASSERT_FALSE(IsValidUTF8(overMaxCodePoint, sizeof(overMaxCodePoint)));
}

TEST_F(StringUtilsTest, UTF8Surrogates)
{
  // 0x1f601 should be encoded as F0 9F 98 81 in UTF-8, however some conversions from
  // UTF-16 will convert the UTF-16 surrogate pair (D83D DE01) to UTF-8 instead
  // We will accept these, even though RFC 3629 declares them invalid
  uint8_t smileyWithSurrogates[6] = {0xed, 0xa0, 0xbd, 0xed, 0xb8, 0x81};
  ASSERT_TRUE(IsValidUTF8(smileyWithSurrogates, sizeof(smileyWithSurrogates)));

  // We will not accept a high surrogate without a low surrogate or vice versa
  ASSERT_FALSE(IsValidUTF8(smileyWithSurrogates, 3));
  ASSERT_FALSE(IsValidUTF8(smileyWithSurrogates + 3, 3));
}

TEST_F(StringUtilsTest, UTF8BOM)
{
  uint8_t bom[3] = {0xef, 0xbb, 0xbf};
  ASSERT_TRUE(IsValidUTF8(bom, sizeof(bom)));
}

TEST_F(StringUtilsTest, UTF8Emoji)
{
  uint8_t smiles[12] = {0xf0, 0x9f, 0x98, 0x80, 0xf0, 0x9f,
                        0x98, 0x80, 0xf0, 0x9f, 0x98, 0x80};
  ASSERT_TRUE(IsValidUTF8(smiles, sizeof(smiles)));
  ASSERT_FALSE(IsValidUTF8(smiles, sizeof(smiles) - 1));

  uint8_t heart[3] = {0xe2, 0x99, 0xa5};
  ASSERT_TRUE(IsValidUTF8(heart, sizeof(heart)));
}
  
TEST_F(StringUtilsTest, StringSplit)
{
  char c = '|';
  std::string ss = "hello|world|and|universe";
  std::vector<std::string> split = StringSplit(ss, c);
  ASSERT_EQ( split.size(), 4 );
  EXPECT_EQ( split[0], "hello" );
  EXPECT_EQ( split[1], "world" );
  EXPECT_EQ( split[2], "and" );
  EXPECT_EQ( split[3], "universe" );
  
  c = ',';
  ss = ",,";
  split = StringSplit(ss, c);
  ASSERT_EQ( split.size(), 3 );
  EXPECT_EQ( split[0], "" );
  EXPECT_EQ( split[1], "" );
  EXPECT_EQ( split[2], "" );
}
  
TEST_F(StringUtilsTest, StringJoin)
{
  std::vector<std::string> test = {{"one"}, {"two"}, {"three"}};
  char c = ',';
  std::string res = StringJoin(test, c);
  EXPECT_EQ( res, "one,two,three" );
  
  test = {{"one"}, {"two"}};
  c = '|';
  res = StringJoin(test, c);
  EXPECT_EQ( res, "one|two" );
  
  test = {{"one"}, {""}, {""}};
  c = ',';
  res = StringJoin(test, c);
  EXPECT_EQ( res, "one,," );
  
  test = {{""}, {""}, {""}};
  c = ',';
  res = StringJoin(test, c);
  EXPECT_EQ( res, ",," );
  
}

} // namespace Util
} // namespace Anki
