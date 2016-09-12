/**
 * File: navMeshQuadTreeProcessor.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Class for processing a navMeshQuadTree. It relies on the navMeshQuadTree and navMeshQuadTreeNodes to
 * share the proper information for the Processor.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_PROCESSOR_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_PROCESSOR_H

#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapTypes.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/helpers/templateHelpers.h"

#include <unordered_map>
#include <unordered_set>
#include <map>

namespace Anki {
namespace Cozmo {

class NavMeshQuadTreeNode;
using namespace NavMeshQuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeProcessor
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  NavMeshQuadTreeProcessor(VizManager* vizManager);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Notifications from nodes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set root
  void SetRoot(NavMeshQuadTreeNode* node);

  // notification when the content type changes for the given node
  void OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, ENodeContentType oldContent, ENodeContentType newContent);

  // notification when a node is going to be removed entirely
  void OnNodeDestroyed(const NavMeshQuadTreeNode* node);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // return the size of the area currently explored
  inline double GetExploredRegionAreaM2() const { return _totalExploredArea_m2; }

  // returns true if we have borders detected of the given type, or we think we do without having to actually calculate
  // them at this moment (which is slightly more costly and requires non-const access)
  bool HasBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes) const;

  // retrieve the borders for the given combination of content types
  void GetBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes, NavMemoryMapTypes::BorderRegionVector& outBorders);
  
  // returns true if the given ray collides with the given type (any quads of the given type)
  bool HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, ENodeContentTypePackedType types) const;
  
  // fills content regions of filledType that have borders with any content in fillingTypeFlags, converting the filledType
  // region to the content type given (newContent)
  void FillBorder(ENodeContentType filledType, ENodeContentTypePackedType fillingTypeFlags, const NodeContent& newContent);
  
  // replaces the given content with the new one to set
  void ReplaceContent(ENodeContentType typeToReplace, const NodeContent& newContent);
  
  // returns true if there are any nodes of the given type, false otherwise
  bool HasContentType(ENodeContentType type) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // render debug information for the processor (will only redraw if required)
  void Draw() const;
  // remove current debug information
  void ClearDraw() const;
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // each of the quad to quad waypoints of a border we find between node types
  struct BorderWaypoint {
    BorderWaypoint()
      : from(nullptr), to(nullptr), direction(EDirection::Invalid), isEnd(false), isSeed(false) {}
    BorderWaypoint(const NavMeshQuadTreeNode* f, const NavMeshQuadTreeNode* t, EDirection dir, bool end)
      : from(f), to(t), direction(dir), isEnd(end), isSeed(false) {}
    const NavMeshQuadTreeNode* from;  // inner quad
    const NavMeshQuadTreeNode* to;    // outer quad
    EDirection direction; // neighbor 4-direction between from and to
    bool isEnd; // true if this is the last waypoint of a border, for example when the obstacle finishes
    bool isSeed; // just a flag for debugging. True if this waypoint was the first in a border
  };
  
  // vector of waypoints that matched a specific combination of node types
  struct BorderCombination {
    using BorderWaypointVector = std::vector<BorderWaypoint>;
    BorderCombination() : dirty(true) {}
    bool dirty; // flag when border is dirty (needs recalculation)
    BorderWaypointVector waypoints;
  };
  
  using BorderKeyType = uint64_t;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // true if we have a need to cache the given content type, false otherwise
  static bool IsCached(ENodeContentType contentType);
  
  // returns a color used to represent the given contentType for debugging purposes
  static ColorRGBA GetDebugColor(ENodeContentType contentType);
  
  // returns a number that represents the given combination inner-outers
  static BorderKeyType GetBorderTypeKey(ENodeContentType innerType, ENodeContentTypePackedType outerTypes);

  // given a border waypoint, calculate its center in 3D
  static Vec3f CalculateBorderWaypointCenter(const BorderWaypoint& waypoint);

  // given 3d points and their neighbor directions, calculate a 3D border segment definition (line + normal)
  static NavMemoryMapTypes::BorderSegment MakeBorderSegment(const Point3f& origin, const Point3f& dest,
                                                            const NavMemoryMapTypes::BorderSegment::DataType& data,
                                                            EDirection firstEDirection,
                                                            EDirection lastEDirection);
  
  // returns true if the given contentType is contained within the set of types defined in the packedTypes
  static bool IsInENodeContentTypePackedType(ENodeContentType contentType, ENodeContentTypePackedType contentPackedTypes);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // sets all current borders are dirty/invalid, in need of being recalculated
  void InvalidateBorders();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing borders
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // adds border information
  void AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to, EDirection dir);
  
  // flags the last border waypoint as finishing a border, so that it doesn't connect with the next one
  void FinishBorder();
  
  // iterate over current nodes and finding borders between the specified types
  // note this deletes previous borders for other types (in the future we may prefer to find them all at the same time)
  void FindBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes);
  
  // checks if currently we have a node of innerType that would become a seed if we computed borders
  bool HasBorderSeed(ENodeContentType innerType, ENodeContentTypePackedType outerTypes) const;
  
  // recalculate (if dirty) the borders for the given combination of content types
  BorderCombination& RefreshBorderCombination(ENodeContentType innerType, ENodeContentTypePackedType outerTypes);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Collision checks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // checks if the given node or any of its children collides with the given ray and matches the type
  bool HasCollisionRayWithTypes(const NavMeshQuadTreeNode* node, const Point2f& rayFrom, const Point2f& rayTo,
    ENodeContentTypePackedType types) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add quads from the given contentType to the vector for rendering
  void AddQuadsToDraw(ENodeContentType contentType,
    VizManager::SimpleQuadVector& quadVector, const ColorRGBA& color, float zOffset) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using NodeSet = std::unordered_set<const NavMeshQuadTreeNode*>;
  using NodeSetPerType = std::unordered_map<ENodeContentType, NodeSet, Anki::Util::EnumHasher>;
  using BorderMap = std::map<BorderKeyType, BorderCombination>;

  // cache of nodes/quads classified per type for faster processing
  NodeSetPerType _nodeSets;
  
  // borders detected in the last search for each combination of content types (inner + outer)
  // the key is the combination of inner and outer types. See GetBorderTypeKey(...)
  BorderMap _bordersPerContentCombination;
  
  // used during process for easier/faster access to the proper combination
  BorderCombination* _currentBorderCombination;
  
  // pointer to the root of the tree
  NavMeshQuadTreeNode* _root;
  
  // true if there have been changes since last drawn
  mutable bool _contentGfxDirty;
  mutable bool _borderGfxDirty;
  
  // area of all quads that have been explored
  double _totalExploredArea_m2;
  
  VizManager* _vizManager;
  
}; // class
  
} // namespace
} // namespace

#endif //
