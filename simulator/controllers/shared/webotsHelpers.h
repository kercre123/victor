/**
 * File: webotsHelpers.h
 *
 * Author: Matt Michini
 * Date:  01/22/2018
 *
 * Description: A few helper functions for common webots node queries
 *
 * Copyright: Anki, Inc. 2018
**/

#ifndef __Simulator_WebotsCtrlShared_WebotsHelpers_H__
#define __Simulator_WebotsCtrlShared_WebotsHelpers_H__

#include <webots/Node.hpp>

namespace webots {
  class Supervisor;
}

namespace Anki {
namespace WebotsHelpers {

struct RootNodeInfo {
  RootNodeInfo(webots::Node* nodePtr = nullptr,
               int type = webots::Node::NO_NODE,
               const std::string& typeName = "")
  : nodePtr(nodePtr)
  , type(type)
  , typeName(typeName)
  {};
  
  webots::Node* nodePtr;  // pointer to the node
  int type;               // node type (e.g. SUPERVISOR)
  std::string typeName;   // type name. For protos, it's just the proto name e.g. "CozmoBot"
};
  
// Returns a vector containing information about all of the nodes in the Webots scene tree
std::vector<RootNodeInfo> GetAllSceneTreeNodes(const webots::Supervisor* const);

// Returns information about the first node in the scene tree whose TypeName matches the input string.
// Note: As long as the input string exists in the node's TypeName, it's considered a match (e.g. if
// input string is "LightCube", then "LightCubeNew" and "LightCubeOld" would both match)
RootNodeInfo GetFirstMatchingSceneTreeNode(const webots::Supervisor* const, const std::string& typeNameToMatch);

// Return all scene tree nodes whose typeNames match the input string.
std::vector<RootNodeInfo> GetMatchingSceneTreeNodes(const webots::Supervisor* const, const std::string& typeNameToMatch);

// Populates outputStr with the string contents of the specified field. Returns true on success.
// Does not modify outputStr if not successful.
bool GetFieldAsString(const webots::Node* const rootNode,
                      const std::string& fieldName,
                      std::string& outputStr,
                      const bool failOnEmptyString = true);
  
}; // namespace
}; // namespace

#endif // __Simulator_WebotsCtrlShared_WebotsHelpers_H__
