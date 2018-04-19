/**
 * File: quadTreeProcessor.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Helper class for processing a quadTree. It performs a bunch of caching operations for 
 * quick access to important data, without having to explicitly travel the whole tree. We want to use this
 * class specifically for any operations that want to query all Data directly, but don't have constraints
 * on where it is located in the tree.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_QUAD_TREE_PROCESSOR_H
#define ANKI_COZMO_QUAD_TREE_PROCESSOR_H

#include "engine/navMap/quadTree/quadTreeTypes.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"

#include "util/helpers/templateHelpers.h"
#include "coretech/common/engine/math/fastPolygon2d.h"

#include <unordered_map>
#include <unordered_set>
#include <map>

namespace Anki {
namespace Cozmo {
  
class QuadTree;
using namespace MemoryMapTypes;
using namespace QuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class QuadTreeProcessor
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  QuadTreeProcessor();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Notifications from nodes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set root
  // NOTE: (mrw) used to set type from Invalid -> Valid so that only SetRoot and subdivide could create
  //       a validated node in the tree. Any other type of node instantiation would be flagged so that
  //       we know it was not attached to a tree. We could consider adding that safety mechanism back.
  void SetRoot(QuadTree* tree) { _quadTree = tree; };

  // notification when the content type changes for the given node
  void OnNodeContentTypeChanged(const QuadTreeNode* node, const EContentType& oldContent, const bool wasEmpty);

  // notification when a node is going to be removed entirely
  void OnNodeDestroyed(const QuadTreeNode* node);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // return the size of the area currently explored
  inline double GetExploredRegionAreaM2() const { return _totalExploredArea_m2; }
  // return the size of the area currently flagged as interesting edges
  inline double GetInterestingEdgeAreaM2() const { return _totalInterestingEdgeArea_m2; }

  // returns true if we have borders detected of the given type, or we think we do without having to actually calculate
  // them at this moment (which is slightly more costly and requires non-const access)
  bool HasBorders(EContentType innerType, EContentTypePackedType outerTypes) const;

  // retrieve the borders for the given combination of content types
  void GetBorders(EContentType innerType, EContentTypePackedType outerTypes, MemoryMapTypes::BorderRegionVector& outBorders);
  
  // fills content regions of filledType that have borders with any content in fillingTypeFlags, converting the filledType
  // region to the given data
  bool FillBorder(EContentType filledType, EContentTypePackedType fillingTypeFlags, const MemoryMapDataPtr& data);
  
  // fills inner regions satisfying innerPred( inner node ) && outerPred(neighboring node), converting
  // the inner region to the given data
  bool FillBorder(const NodePredicate& innerPred, const NodePredicate& outerPred, const MemoryMapDataPtr& data);
  
  // returns true if there are any nodes of the given type, false otherwise
  bool HasContentType(EContentType type) const;
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // each of the quad to quad waypoints of a border we find between node types
  struct BorderWaypoint {
    BorderWaypoint()
      : from(nullptr), to(nullptr), direction(EDirection::Invalid), isEnd(false), isSeed(false) {}
    BorderWaypoint(const QuadTreeNode* f, const QuadTreeNode* t, EDirection dir, bool end)
      : from(f), to(t), direction(dir), isEnd(end), isSeed(false) {}
    const QuadTreeNode* from;  // inner quad
    const QuadTreeNode* to;    // outer quad
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
  using NodeSet = std::unordered_set<const QuadTreeNode*>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // obtains a set of nodes of type typeToFill that satisfy innerPred(node of type typeToFill) && outerPred(neighboring node)
  void GetNodesToFill(const NodePredicate& innerPred, const NodePredicate& outerPred, NodeSet& output);
  
  // true if we have a need to cache the given content type, false otherwise
  static bool IsCached(EContentType contentType);
  
  // returns a number that represents the given combination inner-outers
  static BorderKeyType GetBorderTypeKey(EContentType innerType, EContentTypePackedType outerTypes);

  // given a border waypoint, calculate its center in 3D
  static Vec3f CalculateBorderWaypointCenter(const BorderWaypoint& waypoint);

  // given 3d points and their neighbor directions, calculate a 3D border segment definition (line + normal)
  static MemoryMapTypes::BorderSegment MakeBorderSegment(const Point3f& origin, const Point3f& dest,
                                                            const MemoryMapTypes::BorderSegment::DataType& data,
                                                            EDirection firstEDirection,
                                                            EDirection lastEDirection);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // sets all current borders are dirty/invalid, in need of being recalculated
  void InvalidateBorders();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing borders
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // adds border information
  void AddBorderWaypoint(const QuadTreeNode* from, const QuadTreeNode* to, EDirection dir);
  
  // flags the last border waypoint as finishing a border, so that it doesn't connect with the next one
  void FinishBorder();
  
  // iterate over current nodes and finding borders between the specified types
  // note this deletes previous borders for other types (in the future we may prefer to find them all at the same time)
  void FindBorders(EContentType innerType, EContentTypePackedType outerTypes);
  
  // checks if currently we have a node of innerType that would become a seed if we computed borders
  bool HasBorderSeed(EContentType innerType, EContentTypePackedType outerTypes) const;
  
  // recalculate (if dirty) the borders for the given combination of content types
  BorderCombination& RefreshBorderCombination(EContentType innerType, EContentTypePackedType outerTypes);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using NodeSetPerType = std::unordered_map<EContentType, NodeSet, Anki::Util::EnumHasher>;
  using BorderMap = std::map<BorderKeyType, BorderCombination>;

  // cache of nodes/quads classified per type for faster processing
  NodeSetPerType _nodeSets;
  
  // borders detected in the last search for each combination of content types (inner + outer)
  // the key is the combination of inner and outer types. See GetBorderTypeKey(...)
  BorderMap _bordersPerContentCombination;
  
  // used during process for easier/faster access to the proper combination
  BorderCombination* _currentBorderCombination;
  
  // pointer to the root of the tree
  QuadTree* _quadTree;
  
  // area of all quads that have been explored
  double _totalExploredArea_m2;
  
  // area of all quads that are currently interesting edges
  double _totalInterestingEdgeArea_m2;
}; // class
  
} // namespace
} // namespace

#endif //
