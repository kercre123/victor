/**
 * File: ptreeMandatory.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-09-29
 *
 * Description: a mandatory getter (as opposed to optional)
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "util/ptree/ptreeMandatory.h"


const boost::property_tree::ptree& Anki::Util::PtreeTools::GetChildMandatory(const boost::property_tree::ptree& tree,
                                                                           const std::string& key,
                                                                           const std::string& file,
                                                                           int line)
{
  boost::optional<const boost::property_tree::ptree&> opt = tree.get_child_optional(key);
  if(opt) {
    return *opt;
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
