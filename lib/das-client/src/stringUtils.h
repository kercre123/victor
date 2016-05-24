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

void StringTrimWhitespaceFromEnd(std::string& s);
} // namespace AnkiUtil
#endif // __StringUtils_H__
