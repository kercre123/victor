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
#include "ptreeTraverser.h"
#include "util/helpers/boundedWhile.h"
#include "util/ptree/includePtree.h"

using namespace boost::property_tree;

namespace Anki{ namespace Util {
namespace PtreeTools {

PtreeTraverser::PtreeTraverser(const ptree& tree)
{
  sVals_.push(tree);
  PtreeKey k;
  sKeys_.push(k);
}

bool PtreeTraverser::done() const
{
  return sVals_.empty();
}

void PtreeTraverser::next()
{
  ptree node = sVals_.top();
  PtreeKey key = sKeys_.top();
  
  sVals_.pop();
  sKeys_.pop();

  unsigned int idx = 0;

  for(const ptree::value_type& v : node) {
    if(v.second.size() > 0) {
      PtreeKey newKey(key);

      // if this is a list, increment index, otherwise keep it at 0
      if(v.first == "") {
        newKey.SetIndex(idx);
        idx++;
      }
      else{
        newKey.push_back(std::pair<std::string, int>(v.first, -1));
        idx = 0;
      }

      sKeys_.push(newKey);
      sVals_.push(v.second);
    }
  }
}

const ptree& PtreeTraverser::topTree() const
{
  return sVals_.top();
}

const PtreeKey& PtreeTraverser::topKey() const
{
  return sKeys_.top();
}


} // namespace
} // namespace
} // namespace