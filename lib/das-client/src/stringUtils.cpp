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

#include "stringUtils.h"

#include "json/json.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>

namespace AnkiUtil
{

bool StringCaseInsensitiveEquals(const std::string& s1, const std::string& s2)
{
  size_t sz = s1.size();

  if (s2.size() != sz) {
    return false;
  }

  for (unsigned int i = 0 ; i < sz; i++) {
    if (std::tolower(s1[i]) != std::tolower(s2[i])) {
      return false;
    }
  }

  return true;
}

std::string StringFromContentsOfFile(const std::string &filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize((size_t) in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return contents;
  }
  return "";
}

std::string StringMapToJson(const std::map<std::string,std::string> &stringMap)
{
  DASClient::Json::Value root;

  for (auto const& kv : stringMap) {
    root[kv.first] = kv.second;
  }

  DASClient::Json::FastWriter writer;
  writer.omitEndingLineFeed();
  std::string outputJson = writer.write(root);

  return outputJson;
}

void StringTrimWhitespaceFromEnd(std::string& s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

} // namespace AnkiUtil
