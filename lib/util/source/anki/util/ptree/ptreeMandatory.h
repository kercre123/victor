/**
 * File: ptreeMandatory.h
 *
 * Author: Brad Neuman
 * Created: 2014-09-29
 *
 * Description: a mandatory getter (as opposed to optional)
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __ANKI_UTIL_PTREE_PTREEMANDATORY_H__
#define __ANKI_UTIL_PTREE_PTREEMANDATORY_H__

#include "util/logging/logging.h"
#include "util/ptree/includePtree.h"
#include <exception>

// Macro to use instead of the normal get call. The normal get throws
// a nasty boost exception and is impossible to debug on android. This
// one throws a basestation exception, and prints a readable DAS error
// with the missing key, file, and line number. Stores result in val,
// which must be the correct type
#define PTREE_GET_CHILD_MANDATORY(tree, key)                            \
  Anki::Util::PtreeTools::GetChildMandatory(                              \
    tree,                                                               \
    key,                                                                \
    __FILE__,                                                           \
    __LINE__)
#define PTREE_GET_MANDATORY(tree, key, val)                             \
  Anki::Util::PtreeTools::GetMandatory(                                   \
    tree,                                                               \
    key,                                                                \
    val,                                                                \
    __FILE__,                                                           \
    __LINE__)

namespace Anki{ namespace Util {
namespace PtreeTools {

// helper for mandatory child getter
const boost::property_tree::ptree& GetChildMandatory(const boost::property_tree::ptree& tree,
                                                     const std::string& key,
                                                     const std::string& file,
                                                     int line);

// helper for mandatory child getter
template<class T>
void GetMandatory(const boost::property_tree::ptree& tree,
                  const std::string& key,
                  T& value,
                  const std::string& file,
                  int line)
{
  boost::optional<T> opt = tree.get_optional<T>(key);
  if(opt) {
    value = *opt;
    return;
  }
  else {
    PRINT_NAMED_ERROR("Ptree.MissingKey",
                      "error: %s:%d: key '%s' not found",
                      file.c_str(),
                      line,
                      key.c_str());
    throw std::runtime_error("Ptree.MissingKey");
  }
}

}
}
} // namespace


#endif
