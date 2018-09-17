/**
 * File: stringUtils
 *
 * Author: seichert
 * Created: 07/14/14
 *
 * Description: Utilities for strings
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __StringUtils_H__
#define __StringUtils_H__

#include <map>
#include <string>
#include <vector>

#define ANKI_STRINGIFY_HELPER(x) #x
#define ANKI_STRINGIFY(x) ANKI_STRINGIFY_HELPER(x)

namespace Anki {
namespace Util {
  
bool StringCaseInsensitiveEquals(const std::string& s1, const std::string& s2);

std::string StringToLower(const std::string& source);

std::string StringToUpper(const std::string& source);

std::string StringToLowerUTF8(const std::string& s);

std::string StringToUpperUTF8(const std::string& s);

std::string StringFromContentsOfFile(const std::string &filename);

std::string StringMapToJson(const std::map<std::string,std::string> &stringMap);

std::string StringMapToPrettyJson(const std::map<std::string,std::string> &stringMap);
  
// Return the json string as a map of <string, string>. This will fail if the json has non string values or is nested
std::map<std::string,std::string> JsonToStringMap(const std::string &jsonString);

// Return the json string array as a vector of string
std::vector<std::string> JsonToStringVector(const std::string& jsonString);

// Read the Json File and convert to a vector of string
std::vector<std::string> JsonFileToStringVector(const std::string& path);

// Returns a string representation of the byte vector
std::string ConvertFromByteVectorToString(const std::vector<uint8_t> &bytes);
  
// Returns a vector representation of the string
void ConvertFromStringToVector(std::vector<uint8_t> &bytes, const std::string &stringValue);

bool StringStartsWith(const std::string& fullString, const std::string& prefix);

bool StringEndsWith(const std::string& fullString, const std::string& ending);
  
// trim whitespace from beginning/end/both ends
void StringTrimWhitespaceFromStart(std::string& s);
void StringTrimWhitespaceFromEnd(std::string& s);
void StringTrimWhitespace(std::string& s);

// Checks if a byte array is valid UTF-8
// Warning:  This is homegrown code for use by PlayerAdvertisement::GetAdvertisingString.
// It has not been broadly tested.  For wider use, unit tests are strongly recommended.
bool IsValidUTF8(const uint8_t* b, size_t length);

bool IsValidUTF8(const std::string& s);

std::string TruncateUTF8String(const std::string& s, size_t maxLength, size_t minLength);

std::string RemovePII(const std::string& s);
  
// Create random uuid string
std::string GetUUIDString();
  
  std::vector<std::string> StringSplitStr(std::string string, const std::string& delim);

// Url encode string
std::string UrlEncodeString(const std::string& str);

// Simple way to handle comma-delimited strings
std::string StringJoin(const std::vector<std::string>& strings, char delim=',');
std::vector<std::string> StringSplit(const std::string& string, char delim=',');
  
void StringReplace( std::string& toChange, const std::string& oldStr, const std::string& newStr );

// @return bool indicating weather the epoch time was set
// posix Epoch time in struct form (time since Jan 1, 1970, midnight UTC) based on
// the passed in date string format
// If the date format is not valid, returns UINT32_MAX (distant future).
bool EpochFromDateString(const std::string& dateString, 
                         const std::string& formatString,
                         struct tm& outEpoch);

// @return posix Epoch time in seconds (time since Jan 1, 1970, midnight UTC) based on
// the specified date string in ISO8601 format: `YYYY-MM-DDTHH:MM:SSZ`.
// If the date format is not valid, returns UINT32_MAX (distant future).
uint32_t EpochSecFromIso8601UTCDateString(const std::string& dateString);

  
} // namespace Util
} // namespace Anki
#endif // __StringUtils_H__
