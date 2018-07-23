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


#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "json/json.h"
#include <assert.h>


namespace Anki {
namespace Util {

  
bool GraphEvaluator2d::Node::operator==(const GraphEvaluator2d::Node& rhs) const
{
  const bool isEqual = FLT_NEAR(_x, rhs._x) && FLT_NEAR(_y, rhs._y);
  return isEqual;
}
  
  
GraphEvaluator2d::GraphEvaluator2d(size_t numNodesToReserve)
{
  Reserve(numNodesToReserve);
}


GraphEvaluator2d::GraphEvaluator2d(const std::vector<Node>& inNodes, bool errorOnFailure)
{
  AddNodes(inNodes, errorOnFailure);
}

  
void GraphEvaluator2d::Clear()
{
  _nodes.clear();
}
  
  
void GraphEvaluator2d::Reserve(size_t numNodesToReserve)
{
  if (numNodesToReserve > 0)
  {
    _nodes.reserve(numNodesToReserve);
  }
}
  

bool GraphEvaluator2d::AddNodes(const std::vector<Node>& inNodes, bool errorOnFailure)
{
  const size_t numNodesToAdd = inNodes.size();
  const size_t postNumNodesExpected = _nodes.size() + numNodesToAdd;
  if (numNodesToAdd > 0)
  {
    _nodes.reserve(postNumNodesExpected);
    
    for (const Node& inNode : inNodes)
    {
      AddNode(inNode._x, inNode._y, errorOnFailure);
    }
  }
  
  bool addedAllNodesOK = (postNumNodesExpected == _nodes.size());
  
  if (errorOnFailure && !addedAllNodesOK)
  {
    PRINT_NAMED_ERROR("GraphEvaluator2d.AddNodes.ErrorAdding",
                      "Expected to have %zu nodes after adding %zu, but has %zu nodes instead!",
                      postNumNodesExpected, numNodesToAdd, _nodes.size());
  }
  
  if (ANKI_DEVELOPER_CODE && errorOnFailure && (_nodes.size() >= 3))
  {
    // allow up to two nodes with the same x value
    float previous2 = _nodes[0]._x;
    float previous  = _nodes[1]._x;
    for (size_t i = 2; i < _nodes.size(); ++i)
    {
      if ((previous == previous2) && (previous == _nodes[i]._x))
      {
        PRINT_NAMED_ERROR("GraphEvaluator2d.AddNodes.TooManyDuplicates",
                          "Three or more overlapping x values (x = %f)",
                          previous);
        addedAllNodesOK = false;
      }
      previous2 = previous;
      previous = _nodes[i]._x;
    }
  }

  return addedAllNodesOK;
}


bool GraphEvaluator2d::AddNode(float x, float y, bool errorOnFailure)
{
  const size_t numNodes = _nodes.size();
  if (numNodes > 0)
  {
    const size_t lastNodeIdx = numNodes-1;
    if (x < _nodes[lastNodeIdx]._x)
    {
      if (errorOnFailure)
      {
        PRINT_NAMED_ERROR("GraphEvaluator2d.AddNode.OORange",
                          "new node (%f, %f) has x < than last node %zu = (%f, %f)",
                          x, y, lastNodeIdx, _nodes[lastNodeIdx]._x, _nodes[lastNodeIdx]._y);
      }
      return false;
    }
  }
  
  _nodes.emplace_back(x, y);
  
  return true;
}
  
  
float GraphEvaluator2d::EvaluateY(float x) const
{
  const size_t numNodes = _nodes.size();
  assert(numNodes > 0);
  
  const Node* prevNode = &_nodes[0];
  
  if (x < prevNode->_x)
  {
    // x is < than all nodes - clamp to 1st node
    return prevNode->_y;
  }
  
  for (size_t i=1; i < numNodes; ++i)
  {
    const Node* nextNode = &_nodes[i];
    
    if (x <= nextNode->_x)
    {
      // found the span - interpolate between prev and next nodes
      const float xRange = (nextNode->_x - prevNode->_x);
      if (FLT_GT(xRange, 0.0f))
      {
        assert(x >= prevNode->_x);
        const float xRatio = (x - prevNode->_x) / xRange;
        assert(xRatio >= 0.0f);
        assert(xRatio <= 1.0f);
        const float yRange = (nextNode->_y - prevNode->_y);
        const float yVal   = prevNode->_y + (xRatio * yRange);
        return yVal;
      }
      else
      {
        // x range is too small to lerp -> just use prevNode
        return prevNode->_y;
      }
    }
    else
    {
      // keep looking
      prevNode = nextNode;
    }
  }
  
  // x is > than all nodes - clamp to last node
  
  return prevNode->_y;
}
  
float GraphEvaluator2d::GetSlopeAt(float x) const
{
  const size_t numNodes = _nodes.size();
  assert(numNodes > 0);
  
  if (numNodes == 1)
  {
    // only one node. define slope to be 0
    return 0.0f;
  }
  if (FLT_LT(x, _nodes[0]._x) || FLT_GT(x, _nodes.back()._x))
  {
    // define slope to be 0 outside range
    return 0.0f;
  }
  
  const Node* prevNode = &_nodes[0];
  
  // calc and cache the second to last slope, in case the last segment is vertical.
  // If there are only two nodes sharing the same x value, this returns FLT_MAX.
  float secondToLastSlope = std::numeric_limits<float>::max();
  
  for (size_t i = 1; i < numNodes; ++i)
  {
    const Node* nextNode = &_nodes[i];
    const float xRange = (nextNode->_x - prevNode->_x);
    const bool notVertical = FLT_GT(xRange, 0.0f);
    const bool lastSegment = (i == numNodes - 1);
    const bool secondToLastSegment = (i == numNodes - 2);
    
    if (notVertical && ((x <= nextNode->_x) || lastSegment))
    {
      const float yRange = (nextNode->_y - prevNode->_y);
      return (yRange / xRange);
    }
    else if (secondToLastSegment)
    {
      const float yRange = (nextNode->_y - prevNode->_y);
      secondToLastSlope = notVertical ? (yRange / xRange) : 0.0f;
      // this slope will not get returned unless the last segment is vertical, which means that the
      // second to last segment should not be vertical (because 3 nodes with the same x value is prohibited)
    }
    
    // keep looking
    prevNode = nextNode;
  }
  
  // the last segment was vertical, so either there's only one vertical segment and this returns FLT_MAX, or
  // there are multiple segments and this returns the second to last segment's slope.
  return secondToLastSlope;
}


bool GraphEvaluator2d::FindFirstX(float y, float& outX) const
{
  const size_t numNodes = _nodes.size();
  assert(numNodes > 0);
  
  const Node* prevNode = &_nodes[0];
  
  // See if almost exact match on 1st node (span won't find if it's single node graph or only a NEAR match outside of span's range)
  if (FLT_NEAR(y, prevNode->_y))
  {
    outX = prevNode->_x;
    return true;
  }
  
  for (size_t i=1; i < numNodes; ++i)
  {
    const Node* nextNode = &_nodes[i];
    
    if (((y >= prevNode->_y) && (y <= nextNode->_y)) ||
        ((y <= prevNode->_y) && (y >= nextNode->_y)))
    {
      // X result exists in this span, but if there's no x separation then technically that result isn't possible
      // (there's an immediate step up) so we skip that case
    
      const float xRange = (nextNode->_x - prevNode->_x);
      if (FLT_GT(Anki::Util::Abs(xRange), 0.0f))
      {
        // found the span - interpolate between prev and next nodes
        
        const float yRange = (nextNode->_y - prevNode->_y);
        const float yRangeAbs = Anki::Util::Abs(yRange);

        if (FLT_GT(yRangeAbs, 0.0f))
        {
          const float xRatio = (y - prevNode->_y) / yRange;
          assert(xRatio >= 0.0f);
          assert(xRatio <= 1.0f);
          const float xVal   = prevNode->_x + (xRatio * xRange);
          outX = xVal;
        }
        else
        {
          // y range is too small to lerp -> just use prevNode
          outX = prevNode->_x;
        }
      
        return true;
      }
      else
      {
        // only if an exact match for either
        if (FLT_NEAR(y, prevNode->_y))
        {
          outX = prevNode->_x;
          return true;
        }
        else if (FLT_NEAR(y, nextNode->_y))
        {
          outX = nextNode->_x;
          return true;
        }
      }
    }
    
    // keep looking
    prevNode = nextNode;
  }
  
  // Failed to find span, but could be an almost exact match on last node
  if (FLT_NEAR(y, prevNode->_y))
  {
    outX = prevNode->_x;
    return true;
  }
  
  // failed to find an X value that would Evaluate to Y
  
  return false;
}
  
  
static const char* kNodesKey = "nodes";
static const char* kNodeXKey = "x";
static const char* kNodeYKey = "y";
  

bool GraphEvaluator2d::ReadFromJson(const Json::Value& inJson)
{
  const Json::Value& nodes = inJson[kNodesKey];
  
  if (nodes.isNull())
  {
    PRINT_NAMED_WARNING("GraphEvaluator2d.ReadFromJson.NoNodes", "Missing entry for '%s' key", kNodesKey);
    return false;
  }

  const uint32_t numNodes = nodes.size();
 
  Clear();
  Reserve(numNodes);
  
  const Json::Value kNullNodeValue;
  
  for (uint32_t i = 0; i < numNodes; ++i)
  {
    const Json::Value& node = nodes.get(i, kNullNodeValue);
    
    if (node.isNull())
    {
      PRINT_NAMED_WARNING("GraphEvaluator2d.ReadFromJson.BadNode", "Node %u failed to read", i);
      return false;
    }
    
    const Json::Value& nodeX = node[kNodeXKey];
    const Json::Value& nodeY = node[kNodeYKey];

    if (nodeX.isNull())
    {
      PRINT_NAMED_WARNING("GraphEvaluator2d.ReadFromJson.BadX", "Node %u failed to read '%s'", i, kNodeXKey);
      return false;
    }
    if (nodeY.isNull())
    {
      PRINT_NAMED_WARNING("GraphEvaluator2d.ReadFromJson.BadY", "Node %u failed to read '%s'", i, kNodeYKey);
      return false;
    }
    
    const float x = nodeX.asFloat();
    const float y = nodeY.asFloat();
    
    AddNode(x, y);
  }
  
  return true;
}


bool GraphEvaluator2d::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  const size_t numNodes = GetNumNodes();
  
  Json::Value outNodes(Json::arrayValue);
  
  for (size_t i = 0; i < numNodes; ++i)
  {
    const GraphEvaluator2d::Node& node = GetNode(i);
    
    Json::Value nodeValue;
    nodeValue[kNodeXKey] = node._x;
    nodeValue[kNodeYKey] = node._y;
    
    outNodes.append(nodeValue);
  }
  
  outJson[kNodesKey] = outNodes;
  
  return true;
}
  
  
bool GraphEvaluator2d::operator==(const GraphEvaluator2d& rhs) const
{
  const size_t numNodes = GetNumNodes();
  
  if (numNodes != rhs.GetNumNodes())
  {
    return false;
  }
  
  for (size_t i = 0; i < numNodes; ++i)
  {
    if (GetNode(i) != rhs.GetNode(i))
    {
      return false;
    }
  }
  
  return true;
}

  
} // end namespace Util
} // end namespace Anki

