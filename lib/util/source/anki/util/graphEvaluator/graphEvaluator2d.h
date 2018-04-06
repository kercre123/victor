/**
 * File: graphEvaluator2d
 *
 * Author: Mark Wesley
 * Created: 10/13/15
 *
 * Description: A 2d graph of nodes, lerps values between nodes, clamps values at ends of the range
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Util_GraphEvaluator_GraphEvaluator2d_H__
#define __Util_GraphEvaluator_GraphEvaluator2d_H__


#include <vector>

namespace Json {
  class Value;
}

namespace Anki {
namespace Util {


class GraphEvaluator2d
{
public:
  
  // ==================== Inner Node ====================
  
  struct Node
  {
    Node(float newX, float newY) : _x(newX), _y(newY) {}
    float _x;
    float _y;
    
    bool operator==(const Node& rhs) const;
    bool operator!=(const Node& rhs) const { return !(*this == rhs); }
  };
  
  // ==================== Public Members ====================
  
  explicit GraphEvaluator2d(size_t numNodesToReserve = 0);
  explicit GraphEvaluator2d(const std::vector<Node>& inNodes, bool errorOnFailure=true);
  
  // ensure noexcept default move and copy assignment-operators/constructors are created
  GraphEvaluator2d(const GraphEvaluator2d& other) = default;
  GraphEvaluator2d& operator=(const GraphEvaluator2d& other) = default;
  GraphEvaluator2d(GraphEvaluator2d&& rhs) noexcept = default;
  GraphEvaluator2d& operator=(GraphEvaluator2d&& rhs) noexcept = default;
  
  void Clear();
  void Reserve(size_t numNodes);
  
  bool AddNodes(const std::vector<Node>& inNodes, bool errorOnFailure=true);
  bool AddNode(float x, float y, bool errorOnFailure=true);
  
  float EvaluateY(float x) const;
  
  // Reverse lookup to try and find an X value that would produce Y, returns success (on success outX has the x result)
  bool  FindFirstX(float y, float& outX) const;
  
  bool operator==(const GraphEvaluator2d& rhs) const;
  bool operator!=(const GraphEvaluator2d& rhs) const { return !(*this == rhs); }
  
  // ===== Json =====
  
  bool WriteToJson(Json::Value& outJson) const;
  bool ReadFromJson(const Json::Value& inJson);
  
  // ==================== Inlined accessors ====================
  
  size_t      GetNumNodes()     const { return _nodes.size(); }
  const Node& GetNode(size_t i) const { return _nodes[i]; }
  bool        Empty()           const { return _nodes.empty(); }

private:
  
  std::vector<Node>    _nodes;
};
  

} // end namespace Util
} // end namespace Anki

#endif // __Util_GraphEvaluator_GraphEvaluator2d_H__
