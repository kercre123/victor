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

bool StoreStringInFile(const std::string& fileName, const std::string& body)
{
  bool success = false;
  std::ofstream fileOut;
  fileOut.open(fileName, std::ios::out | std::ofstream::binary);
  if( fileOut.is_open() ) {
    copy(body.begin(), body.end(), std::ostreambuf_iterator<char>(fileOut));
    fileOut.close();
    success = true;
  }
  return success;
}

bool StoreStringMapInFile(const std::string& fileName, const std::map<std::string,std::string> &stringMap)
{
  std::string jsonString = StringMapToPrettyJson(stringMap);
  return StoreStringInFile(fileName, jsonString);
}

std::map<std::string, std::string> JsonFileToStringMap(const std::string& fileName)
{
  std::string jsonString = StringFromContentsOfFile(fileName);
  return JsonToStringMap(jsonString);
}

std::map<std::string,std::string> JsonToStringMap(const std::string &jsonString)
{
  std::map<std::string,std::string> stringMap;

  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(jsonString, root);
  if(parsingSuccessful) {
    for (auto const& id : root.getMemberNames()) {
      stringMap[id] = root[id].asString();
    }
  }

  return stringMap;
}


std::string StringMapToJson(const std::map<std::string,std::string> &stringMap)
{
  Json::Value root;

  for (auto const& kv : stringMap) {
    root[kv.first] = kv.second;
  }

  Json::FastWriter writer;
  writer.omitEndingLineFeed();
  std::string outputJson = writer.write(root);

  return outputJson;
}

std::string StringMapToPrettyJson(const std::map<std::string,std::string> &stringMap)
{
  Json::Value root;

  for (auto const& kv : stringMap) {
    root[kv.first] = kv.second;
  }

  Json::StyledWriter writer;
  std::string outputJson = writer.write(root);

  return outputJson;
}

void StringTrimWhitespaceFromEnd(std::string& s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

} // namespace AnkiUtil
