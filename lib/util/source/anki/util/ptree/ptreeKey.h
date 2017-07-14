/**
 * File: ptreeTools
 *
 * Author: raul
 * Created: 5/8/14
 * 
 * Description: Check class description below.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_PTREEKEY_H_
#define UTIL_PTREEKEY_H_

#include "util/helpers/includeOstream.h"
#include <utility>
#include <vector>

namespace Anki{ namespace Util {
namespace PtreeTools {

// Helper class which defines a ptree key that works with lists /
// duplicate entries. The strings are paths into the ptree (using dot
// notation) and the int is the index into the list pointed at by the
// string. If the int is -1, then the whole list is returned
class PtreeKey : public std::vector< std::pair<std::string, int> >
{
  friend std::ostream& operator<<(std::ostream& o, const PtreeKey& key);
public:
  PtreeKey() {};

  // parses a string of the form a.b.c[X].d.e[Y] where X and Y are
  // 0-based indices into arrays c and e
  PtreeKey(const std::string& str);

  void SetIndex(int idx);
};

} // namespace
} // namespace
} // namespace

#endif
