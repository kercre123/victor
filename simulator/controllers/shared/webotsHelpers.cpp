/**
 * File: webotsHelpers.cpp
 *
 * Author: Matt Michini
 * Date:  01/22/2018
 *
 * Description: A few helper functions for common webots node queries
 *
 * Copyright: Anki, Inc. 2018
**/

#include "simulator/controllers/shared/webotsHelpers.h"
#include "util/logging/logging.h"

#include <webots/Supervisor.hpp>
#include <webots/Field.hpp>

namespace Anki {
namespace WebotsHelpers {

std::vector<RootNodeInfo> GetAllSceneTreeNodes(const webots::Supervisor* const supervisor)
{
  DEV_ASSERT(supervisor != nullptr, "WebotsHelpers.GetAllSceneTreeNodes.NullSupervisor");

  const auto* rootNode = supervisor->getRoot();
  DEV_ASSERT(rootNode != nullptr, "WebotsHelpers.GetAllSceneTreeNodes.NullSupervisorRoot");
  
  const auto* rootChildren = rootNode->getField("children");
  DEV_ASSERT(rootChildren != nullptr, "WebotsHelpers.GetAllSceneTreeNodes.NullRootChildren");
  
  std::vector<RootNodeInfo> sceneTreeNodes;
  
  int numRootChildren = rootChildren->getCount();
  for (int n = 0 ; n < numRootChildren; ++n) {
    webots::Node* nd = rootChildren->getMFNode(n);
    DEV_ASSERT(nd != nullptr, "WebotsHelpers.GetAllSceneTreeNodes.NullNode");
    
    sceneTreeNodes.emplace_back(nd, nd->getType(), nd->getTypeName());
  }
  return sceneTreeNodes;
}


RootNodeInfo GetFirstMatchingSceneTreeNode(const webots::Supervisor* const supervisor, const std::string& typeNameToMatch)
{
  RootNodeInfo foundNode;
  for (const auto& node : GetAllSceneTreeNodes(supervisor)) {
    if (node.typeName.find(typeNameToMatch) != std::string::npos) {
      foundNode = node;
      break;
    }
  }
  return foundNode;
}


std::vector<RootNodeInfo> GetMatchingSceneTreeNodes(const webots::Supervisor* const supervisor, const std::string& typeNameToMatch)
{
  std::vector<RootNodeInfo> foundNodes;
  for (const auto& node : GetAllSceneTreeNodes(supervisor)) {
    if (node.typeName.find(typeNameToMatch) != std::string::npos) {
      foundNodes.push_back(node);
    }
  }
  return foundNodes;
}


bool GetFieldAsString(const webots::Node* const rootNode,
                      const std::string& fieldName,
                      std::string& outputStr,
                      const bool failOnEmptyString)
{
  DEV_ASSERT_MSG(rootNode != nullptr,
                 "WebotsHelpers.GetFieldAsString.NullRootNode",
                 "Null root node (field name %s)",
                 fieldName.c_str());

  const auto* field = rootNode->getField(fieldName);
  if (field == nullptr) {
    PRINT_NAMED_ERROR("WebotsHelpers.GetFieldAsString.NullField",
                      "Field named %s does not exist (root node type %s)",
                      fieldName.c_str(),
                      rootNode->getTypeName().c_str());
    return false;
  } else if (field->getType() != webots::Field::SF_STRING) {
    PRINT_NAMED_ERROR("WebotsHelpers.GetFieldAsString.WrongFieldType",
                      "Wrong field type '%s' for field %s (should be string)",
                      field->getTypeName().c_str(),
                      fieldName.c_str());
    return false;
  }

  outputStr = field->getSFString();
  
  if (failOnEmptyString && outputStr.empty()) {
    PRINT_NAMED_WARNING("WebotsHelpers.GetFieldAsString.EmptyString",
                        "Empty string for field name %s",
                        fieldName.c_str());
    return false;
  }

  return true;
}


}; // namespace
}; // namespace

