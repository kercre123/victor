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


}; // namespace
}; // namespace

