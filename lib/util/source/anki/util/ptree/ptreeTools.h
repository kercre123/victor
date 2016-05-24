/**
 * File: ptreeTools
 *
 * Author: damjan
 * Created: 5/22/13
 * 
 * Description: Helper functions for managing ptrees
 * 
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef UTIL_PTREETOOLS_H_
#define UTIL_PTREETOOLS_H_

#include <string>
#include <boost/property_tree/ptree_fwd.hpp>

using namespace boost::property_tree;

namespace Json {
  class Value;
}

namespace Anki{ namespace Util {
namespace PtreeTools {

// performs a fast deep merge, iterating all nodes in 'what', overriding all nodes that are common and copying those that are not
void FastDeepOverride(boost::property_tree::ptree& where, const boost::property_tree::ptree& what);

// allows extending from 'what' into 'where', giving precedence to 'where' when common keys are found instead of 'what',
// storing the result in 'where'. The optional skipKey is removed from 'what' even if is not present in 
// this is equivalent to:
// 1) make a copy of where
// 2) make a copy of what
// 3) remove skipKey from what(copy)
// 4) FastDeepOverride where into what(copy), which ends up in where without an extra copy
// If you also make a copy of where in your code, you could optimize by making a copy of what instead
// and overriding it with where = FastDeepOverride(what, where, x)
// This version exists as opposed to FastDeepAddition because overrides allow A.B.C.D notation, whereas unprocessed
// trees do not support it with an extra step.
void OrderedDeepOverride(boost::property_tree::ptree& where, const boost::property_tree::ptree& what, const char* skipKey=nullptr);

// performs a shallow merge, plus another shallow merge of one nested node
ptree DeepMergeLimited(const ptree & first, const ptree & second, const std::string & nestedNodeKey);

// merges only first level nodes
ptree ShallowMergeWithCopy(const ptree & first, const ptree & second);

// merges only first level nodes
void ShallowMerge(ptree & first, const ptree & second);

// Pre-processes the tree allowing the use of "id" and "extends"
// anywhere in the tree. Modifies the passed in tree
void FastPreprocess( ptree& tree );

// Allows a tree that was previously preprocessed to be preprocessed once more.
// Useful when optionally merging in nodes at different stages.
void ResetPreprocessFlag(ptree& tree);
  
// Checks if string is a valid key, which for now means it contains
// some combo of alphanumeric characters and '_'.
// Here for convenience since ptrees are currently getting stomped on
// a frequent basis.
bool IsAlphaNum(const std::string &str);

// Safe function to dump json to screen
void PrintJson(const ptree& config);
  
// Print a JsonCpp (not ptree) json
void PrintJson(const Json::Value& jsonTree);

// Prints a given key to the screen
void PrintJson(const ptree& config, const std::string& key);

// Returns given json as a string
std::string StringJson(const ptree& config);


// These are old, slow versions. Keeping them in case new versions have bugs and we need to reproduce old behavior,
// but do not use them.
class PtreeKey; // TODO this is probably deprecated(unused) once we remove deprecated methods
namespace Deprecated {
ptree DeepMerge_Deprecated(const ptree & first, const ptree & second);
void Preprocess_Deprecated( ptree& tree);
ptree& GetChild_Deprecated(ptree& tree, const PtreeKey& key);
};

} // namespace
} // namespace
} // namespace

#endif
