/**
 * File: ptreeTools
 *
 * Author: raul
 * Created: 5/8/14
 * 
 * Description:
 * 
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "ptreeKey.h"
#include "util/helpers/boundedWhile.h"
#include <exception>


using namespace std;

namespace Anki{ namespace Util {
namespace PtreeTools {

void PtreeKey::SetIndex(int idx)
{
  if( empty() ) {
    throw std::runtime_error("PtreeKey.SetIndex");
  }

  back().second = idx;
}

PtreeKey::PtreeKey(const std::string& str)
{
  size_t start = 0;
  size_t pos = str.find("[");
  size_t len = str.length();

  BOUNDED_WHILE(100, pos != std::string::npos)
  {
    size_t endPos = str.find("]", pos);
    if(endPos == string::npos)
      endPos = len;

    int idx = atoi(str.substr(pos+1, endPos - pos - 1).c_str());

    string subkey = str.substr(start, pos-start);

    push_back(pair<string, int>(subkey, idx));
    start = endPos+2; // +1 for the ']' and +1 for the next '.'
    if(start < len) {
      pos = str.find("[", start);
    }
    else {
      start = pos = string::npos;
    }
  }

  if(start != string::npos) {
    // push the remaining key with no index
    push_back(pair<string, int>(str.substr(start), -1));
  }
}

ostream& operator<<(ostream& o, const PtreeKey& key)
{
  PtreeKey::const_iterator end = key.end();
  bool first = true;
  for(PtreeKey::const_iterator it = key.begin(); it != end; ++it) {
    if(first)
      first = false;
    else
      o<<'.';
    o<<it->first<<'['<<it->second<<']';
  }

  return o;
}

} // namespace
} // namespace
} // namespace
