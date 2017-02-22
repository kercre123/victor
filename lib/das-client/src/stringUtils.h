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

namespace AnkiUtil
{
bool StringCaseInsensitiveEquals(const std::string& s1, const std::string& s2);

std::string StringFromContentsOfFile(const std::string &filename);

std::string StringMapToJson(const std::map<std::string,std::string> &stringMap);

std::string StringMapToPrettyJson(const std::map<std::string,std::string> &stringMap);

bool StoreStringInFile(const std::string& fileName, const std::string& body);

bool StoreStringMapInFile(const std::string& fileName, const std::map<std::string,std::string> &stringMap);

std::map<std::string, std::string> JsonFileToStringMap(const std::string& fileName);

std::map<std::string,std::string> JsonToStringMap(const std::string &jsonString);

void StringTrimWhitespaceFromEnd(std::string& s);
} // namespace AnkiUtil
#endif // __StringUtils_H__
