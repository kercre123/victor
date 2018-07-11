/**
* File: common.h
*
* Author: ross
* Created: Jun 24 2018
*
* Description: shared types and methods
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <vector>
#include <string>
#include <unordered_map>

namespace Anki {

using TagList = std::vector<std::pair<std::string, int>>; // field name and field num

using TagsInfo = std::unordered_map<std::string, TagList>; // message name and tag info

struct CtorInfo {
  std::string type;
  std::string varName;
  bool isMessageType;
};

inline void FindAndReplaceAll( std::string& str, const std::string& searchStr, const std::string& replaceStr )
{
  size_t pos = str.find( searchStr );
  while( pos != std::string::npos ) {
    str.replace( pos, searchStr.size(), replaceStr );
    pos = str.find( searchStr, pos + searchStr.size() );
  }
}

} // namespace

#endif // __COMMON_H__
