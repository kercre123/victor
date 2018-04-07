/**
 * File: stringutils.cpp
 *
 * Author: seichert
 * Created: 2/16/2018
 *
 * Description: Routines for working with strings
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "stringutils.h"
#include <algorithm>
#include <cctype>

static uint8_t hex_char_to_byte(char input) {
  if (input >= '0' && input <= '9') {
    return input - '0';
  }
  if (input >= 'A' && input <= 'F') {
    return input - 'A' + 10;
  }
  if (input >= 'a' && input <= 'f') {
    return input - 'a' + 10;
  }
  return 0;
}

inline bool caseInsensitveCharCompare(char a, char b) {
  return (std::toupper(a) == std::toupper(b));
}

bool AreCaseInsensitiveStringsEqual(const std::string& s1, const std::string& s2)
{
  return ((s1.size() == s2.size())
          && std::equal(s1.begin(), s1.end(), s2.begin(), caseInsensitveCharCompare));
}

static char nibbleToHexChar(uint8_t nib) {
  return "0123456789ABCDEF"[nib];
}

static char nibbleToHexCharLowerCase(uint8_t nib) {
  return "0123456789abcdef"[nib];
}

std::string byteVectorToHexString(const std::vector<uint8_t>& byteVector,
                                  const int numSpaces,
                                  const bool lowercase)
{
  std::string hexString;
  bool first = true;
  for (auto b : byteVector) {
    if (first) {
      first = false;
    } else {
      for (int i = 0 ; i < numSpaces ; i++) {
        hexString.push_back(' ');
      }
    }
    uint8_t high = (b >> 4) & 0xf;
    uint8_t low = b & 0xf;
    char hexDigitHigh = lowercase ? nibbleToHexCharLowerCase(high) : nibbleToHexChar(high);
    char hexDigitLow = lowercase ? nibbleToHexCharLowerCase(low) : nibbleToHexChar(low);
    hexString.push_back(hexDigitHigh);
    hexString.push_back(hexDigitLow);
  }

  return hexString;
}

bool IsHexString(const std::string& hexString) {
  for (auto const& c : hexString) {
    if (!std::isxdigit(c)) {
      return false;
    }
  }
  return true;
}

std::string hexStringToAsciiString(const std::string& hexString)
{
  std::string asciiString;
  for (auto it = hexString.cbegin() ; it != hexString.cend(); it++) {
    char c = (hex_char_to_byte(*it)) << 4;
    it++;
    c |= hex_char_to_byte(*it);
    asciiString.push_back(c);
  }
  return asciiString;
}