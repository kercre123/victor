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

#ifndef UTIL_PTREETRAVERSER_H_
#define UTIL_PTREETRAVERSER_H_

#include <stack>
#include <boost/property_tree/ptree_fwd.hpp>
#include "util/ptree/ptreeKey.h"
#include "util/ptree/includePtree.h"

namespace Anki{ namespace Util {
namespace PtreeTools {

/**
 * Helper class which will do a depth first traversal of a ptree.
 *
 * A "node" in the tree is a level which contains ptree data and its
 * children are any subgroups or lists which are defined within it.
 *
 * @author brad
 */
class PtreeTraverser {
public:

  // Constructs the traverser and sets top to be the first node.
  PtreeTraverser(const boost::property_tree::ptree& tree);

  // returns true if there are no more nodes to traverse
  bool done() const;

  // advances to next node.
  void next();

  const boost::property_tree::ptree& topTree() const;
  const PtreeKey& topKey() const;

private:

  std::stack<boost::property_tree::ptree> sVals_;
  std::stack<PtreeKey> sKeys_;
};

} // namespace
} // namespace
} // namespace

#endif
