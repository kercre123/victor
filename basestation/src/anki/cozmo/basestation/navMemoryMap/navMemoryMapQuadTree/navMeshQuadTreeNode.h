/**
 * File: navMeshQuadTreeNode.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Nodes in the nav mesh, represented as quad tree nodes.
 * Note nodes can work with a processor to speed up algorithms and searches, however this implementation supports
 * working with one processor only for any given node. Do not use more than one processor instance for nodes, or
 * otherwise leaks and bad pointer references will happen.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H

#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapTypes.h"

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/triangle.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/helpers/noncopyable.h"

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

class NavMeshQuadTreeProcessor;
using namespace NavMeshQuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeNode : private Util::noncopyable
{
public:
  friend class NavMeshQuadTreeProcessor;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using NodeCPtrVector = std::vector<const NavMeshQuadTreeNode*>;

  struct SegmentLineEquation {
    const Point2f& from;
    const Point2f& to;
    bool isXAligned; // m and 1/m are not useful in this case
    bool isYAligned; // m and 1/m are not useful in this case
    float segM;      // m (segment-line slope)
    float oneOverM;  // 1/m
    float segB;      // y-intercept
    
    inline void CalculateMB()
    {
      const float xInc = from.x() - to.x();
      const float yInc = from.y() - to.y();
      isXAligned = FLT_NEAR(xInc, 0.0f);
      isYAligned = FLT_NEAR(yInc, 0.0f);
      const bool computeLineEq = !isXAligned && !isYAligned;
      if ( computeLineEq )
      {
        // m = (y-y1) / (x-x1)
        segM = yInc / xInc;
        oneOverM = 1.0f / segM;
        // y = mx + b   -->   b = y - mx
        segB = from.y() - (segM * from.x());
      }
      else {
        segM = oneOverM = segB = 0.0f; // they are not 0, I just set to 0 because I don't use them for X or Y aligned segments
      }
    }
    
    inline SegmentLineEquation(const Point2f& f, const Point2f& t) : from(f), to(t) { CalculateMB(); }
  };
  using QuadSegmentArray = SegmentLineEquation[4];
  using TriangleSegmentArray = SegmentLineEquation[3];
  using QuadInfoVector = std::vector<ExternalInterface::MemoryMapQuadInfo>;
  using QuadInfoDebugVizVector = std::vector<ExternalInterface::MemoryMapQuadInfoDebugViz>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Crete node
  // it will allow subdivision as long as level is greater than 0
  NavMeshQuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, NavMeshQuadTreeNode* parent);
  
  // Note: Destructor should call processor.OnNodeDestroyed for any processor the node has been registered to.
  // However, by design, we don't do this (no need to store processor pointers, etc). We can do it because of the
  // assumption that the processor(s) will be destroyed at the same time than nodes are, except in the case of
  // nodes that are merged into their parents, or when we shift the root, in which cases we do notify the processor.
  // Alternatively processors would store weak_ptr, but no need for the moment given the above assumption.
  // ~NavMeshQuadTreeNode();
  
  // with noncopyable this is not needed, but xcode insist on showing static_asserts in cpp as errors for a while,
  // which is annoying
  NavMeshQuadTreeNode(const NavMeshQuadTreeNode&&) = delete;
  NavMeshQuadTreeNode& operator=(const NavMeshQuadTreeNode&&) = delete;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t GetLevel() const { return _level; }
  float GetSideLen() const { return _sideLen; }
  const Point3f& GetCenter() const { return _center; }
  const NodeContent& GetContent() const { return _content; }

  // consider using the concrete checks if you are going to do GetContentType() == X, in case the meaning of that
  // comparison changes
  ENodeContentType GetContentType() const { return _content.type; }
  bool IsContentTypeUnknown() const { return _content.type == ENodeContentType::Unknown; }
  
  // returns true if this node FULLY contains the given quad, false if any corner is not within this node's quad
  bool Contains(const Quad2f& quad) const;
  // returns true if this node contains the given point
  bool Contains(const Point2f& point) const;
  // returns true if this node FULLY contains the given triangle
  bool Contains(const Triangle2f& tri) const;

  // Builds a quad from our coordinates
  Quad3f MakeQuad() const;
  Quad2f MakeQuadXY(const float padding_mm=0.0f) const;
  
  // return the unique_ptr for our child at the given index. If no child is present at the given index, it returns
  // a null pointer
  const std::unique_ptr<NavMeshQuadTreeNode>& GetChildAt(size_t index) const;
  
  // return number of current children
  size_t GetNumChildren() const { return _childrenPtr.size(); }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // processes the given quad within the tree and appropriately stores the content in the quad tree where the quad overlaps
  bool AddContentQuad(const Quad2f& quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
  // processes the given line within the tree and appropriately stores the content in the quad tree where the line collides
  bool AddContentLine(const Point2f& from, const Point2f& to, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
  // processes the given triangle within the tree and appropriately stores the content in the quad tree where the line collides
  bool AddContentTriangle(const Triangle2f& tri, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
  // processes the given point within the tree and appropriately stores the content in the quad tree where the point resides
  bool AddContentPoint(const Point2f& point, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);

  // moves this node's center towards the required points, so that they can be included in this node
  // returns true if the root shifts, false if it can't shift to accomodate all points or the points are already contained
  bool ShiftRoot(const std::vector<Point2f>& requiredPoints, NavMeshQuadTreeProcessor& processor);

  // Convert this node into a parent of its level, delegating its children to the new child that substitutes it
  // In order for a quadtree to be valid, the only way this could work without further operations is calling this
  // on a root node. Such responsibility lies in the caller, not in this node
  // Returns true if successfully expanded, false otherwise
  // maxRootLevel: it won't upgrade if the root is already higher level than the specified
  bool UpgradeRootLevel(const Point2f& direction, uint8_t maxRootLevel, NavMeshQuadTreeProcessor& processor);
 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // find the neighbor of the same or higher level in the given direction
  const NavMeshQuadTreeNode* FindSingleNeighbor(EDirection direction) const;

  // find the group of smallest neighbors with whom this node shares a border.
  // they would be children of the same level neighbor. This is normally useful when our neighbor is subdivided but
  // we are not.
  // direction: direction in which we move to find the neighbors (4 cardinals)
  // iterationDirection: when there're more than one neighbor in that direction, which one comes first in the list
  // NOTE: this method is expected to NOT clear the vector before adding neighbors
  void AddSmallestNeighbors(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& neighbors) const;
 
  // adds to the given vector the smallest descendants of every branch. If a node is a leaf, it adds itself,
  // otherwise it recursively asks children to do the same
  void AddSmallestDescendantsDepthFirst(NodeCPtrVector& descendants) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds the necessary quads to the given vector to be rendered
  void AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Send
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds the necessary quad infos to the given vector to be sent
  void AddQuadsToSend(QuadInfoVector& quadInfoVector) const;
  void AddQuadsToSendDebugViz(QuadInfoDebugVizVector& quadInfoVector) const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info about moving towards a neighbor
  struct MoveInfo {
    EQuadrant neighborQuadrant;  // destination quadrant
    bool sharesParent;           // whether destination quadrant is in the same parent
  };
  
  // type of overlap for quads
  enum class EContentOverlap { Partial, Total };
  
  // container for each node's children
  using ChildrenVector = std::vector< std::unique_ptr<NavMeshQuadTreeNode> >;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Collision checks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // returns true if the quad contains this node, this node contains the quad, or if they overlap
  bool ContainsOrOverlapsQuad(const Quad2f& inQuad) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // checks if the given point is contained in the quad, and properly acts, delegating on children if needed
  bool AddPoint_Recursive(const Point2f& point, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);

  // setup precomputes common variables that the recursive method is going to use
  bool AddTriangle_Setup(const Triangle2f& quad,
                         const NodeContent& detectedContent,
                         NavMeshQuadTreeProcessor& processor);

  // checks how the given triangle affects this node, and properly acts, delegating on children if needed
  bool AddTriangle_Recursive(const Triangle2f& triangle,
                             const TriangleSegmentArray& triangleSegments,
                             const NodeContent& detectedContent,
                             NavMeshQuadTreeProcessor& processor);
  
  // checks how the given line affects this node, and properly acts, delegating on children if needed
  bool AddLine_Recursive(const SegmentLineEquation& segmentLine, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
 
  // old implementation (slow) of AddQuad
  // checks how the given quad affects this node, and properly acts, delegating on children if needed
  bool AddQuad_OldRecursive(const Quad2f& quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
  // new implementation (fast) of AddQuad
  // setup precomputes common variables that the recursive method is going to use
  bool AddQuad_NewSetup(const Quad2f &quad,
                        const NodeContent& detectedContent,
                        NavMeshQuadTreeProcessor& processor);
  
  // new implementation (fast) of AddQuad
  // checks how the given quad affects this node, and properly acts, delegating on children if needed
  bool AddQuad_NewRecursive(const Quad2f& quad,
                            const QuadSegmentArray& nonAAQuadSegments,
                            const NodeContent& detectedContent,
                            NavMeshQuadTreeProcessor& processor);
  
  // return true if this quad can subdivide
  bool CanSubdivide() const { return _level > 0; }
  
  // return true if this quad is already subdivided
  bool IsSubdivided() const { return !_childrenPtr.empty(); }
  
  // returns true if this node can override children with the given content type (some changes in content
  // type are not allowed to preserve information). This is a necessity now to prevent Cliffs from being
  // removed by Clear. Note that eventually we have to support that since it's possible that the player
  // actually covers the cliff with something transitable
  bool CanOverrideSelfWithContent(ENodeContentType newContentType, EContentOverlap overlap ) const;
  bool CanOverrideSelfAndChildrenWithContent(ENodeContentType newContentType, EContentOverlap overlap) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // subdivide/merge children
  void Subdivide(NavMeshQuadTreeProcessor& processor);
  void Merge(const NodeContent& newContent, NavMeshQuadTreeProcessor& processor);
  void ClearDescendants(NavMeshQuadTreeProcessor& processor);

  // checks if all children are the same type, if so it removes the children and merges back to a single parent
  void TryAutoMerge(NavMeshQuadTreeProcessor& processor);
  
  // sets the content type to the detected one.
  // try checks por priority first, then calls force
  void TrySetDetectedContentType(const NodeContent& detectedContent, EContentOverlap overlap, NavMeshQuadTreeProcessor& processor);
  // force sets the type and updates shared container
  void ForceSetDetectedContentType(const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
  // sets a new parent to this node. Used on expansions
  void ChangeParent(const NavMeshQuadTreeNode* newParent) { _parent = newParent; }
  
  // swaps children and content with 'otherNode', updating the children's parent pointer
  void SwapChildrenAndContent(NavMeshQuadTreeNode* otherNode, NavMeshQuadTreeProcessor& processor);
  
  // read the note in destructor on why we manually destroy nodes when they are removed
  static void DestroyNodes(ChildrenVector& nodes, NavMeshQuadTreeProcessor& processor);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   
  // calculate where we would land from a quadrant if we moved in the given direction
  static const MoveInfo* GetDestination(EQuadrant from, EDirection direction);
  
  // get the child in the given quadrant, or null if this node is not subdivided
  const NavMeshQuadTreeNode* GetChild(EQuadrant quadrant) const;

  // iterate until we reach the nodes that have a border in the given direction, and add them to the vector
  // NOTE: this method is expected to NOT clear the vector before adding descendants
  void AddSmallestDescendants(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& descendants) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // NOTE: try to minimize padding in these attributes

  // children when subdivided. Can be empty or have 4 nodes
  std::vector< std::unique_ptr<NavMeshQuadTreeNode> > _childrenPtr;

  // coordinates of this quad
  Point3f _center;
  float   _sideLen;

  // parent node
  const NavMeshQuadTreeNode* _parent;

  // our level
  uint8_t _level;

  // quadrant within the parent
  EQuadrant _quadrant;
  
  // information about what's in this quad
  NodeContent _content;
    
}; // class
  
namespace QTOptimizations{
  bool OverlapsOrContains(const Quad2f& axisAlignedQuad,
                          const NavMeshQuadTreeNode::SegmentLineEquation& line,
                          bool& doesLineCrossQuad);
  
  
} // namespace QTOptimizations
  
} // namespace
} // namespace

#endif //
