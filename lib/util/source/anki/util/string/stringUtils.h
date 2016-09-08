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

namespace Anki {
namespace Util {
  
bool StringCaseInsensitiveEquals(const std::string& s1, const std::string& s2);

std::string StringToLower(const std::string& source);

std::string StringToUpper(const std::string& source);

std::string StringFromContentsOfFile(const std::string &filename);

std::string StringMapToJson(const std::map<std::string,std::string> &stringMap);
  
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

bool StringEndsWith(const std::string& fullString, const std::string& ending);

// Checks if a byte array is valid UTF-8
// Warning:  This is homegrown code for use by PlayerAdvertisement::GetAdvertisingString.
// It has not been broadly tested.  For wider use, unit tests are strongly recommended.
bool IsValidUTF8(const uint8_t* b, size_t length);

bool IsValidUTF8(const std::string& s);

std::string TruncateUTF8String(const std::string& s, size_t maxLength, size_t minLength);

std::string RemovePII(const std::string& s);
  
// Create random uuid string
std::string GetUUIDString();

// Url encode string
std::string UrlEncodeString(const std::string& str);
  
} // namespace Util
} // namespace Anki
#endif // __StringUtils_H__
