/**
 * File: memoryMap.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: QuadTree map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_H
#define ANKI_COZMO_MEMORY_MAP_H

#include "engine/navMap/iNavMap.h"
#include "engine/navMap/quadTree/quadTree.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MemoryMap : public INavMap
{
public:

  using EContentType = MemoryMapTypes::EContentType;
  using FullContentArray = MemoryMapTypes::FullContentArray;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  MemoryMap();
  virtual ~MemoryMap() {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  
  // add data to the memory map defined by poly
  virtual bool Insert(const Poly2f& poly, const MemoryMapData& data) override;
  
  // merge the given map into this map by applying to the other's information the given transform
  // although this methods allows merging any INavMemoryMap into any INavMemoryMap, subclasses are not
  // expected to provide support for merging other subclasses, but only other instances from the same
  // subclass
  virtual bool Merge(const INavMap* other, const Pose3d& transform) override;
  
  // change the content type from typeToReplace into newTypeSet if there's a border from any of the typesToFillFrom towards typeToReplace
  virtual bool FillBorder(EContentType typeToReplace, const FullContentArray& neighborsToFillFrom, EContentType newTypeSet, TimeStamp_t timeMeasured) override;
  
  // attempt to apply a transformation function to all nodes in the tree
  virtual bool TransformContent(NodeTransformFunction transform) override;
  
  // attempt to apply a transformation function to any node intersecting the poly
  virtual bool TransformContent(const Poly2f& poly, NodeTransformFunction transform) override;

  // populate a list of all data that matches the predicate
  virtual void FindContentIf(NodePredicate pred, MemoryMapDataConstList& output) const override;

  
  // return the size of the area currently explored
  virtual double GetExploredRegionAreaM2() const override;
  // return the size of the area currently flagged as interesting edges
  virtual double GetInterestingEdgeAreaM2() const override;
  
  // returns the precision of content data in the memory map. For example, if you add a point, and later query for it,
  // the region that the point generated to store the point could have an error of up to this length.
  virtual float GetContentPrecisionMM() const override;
  
  // check whether the given content types would have any borders at the moment. This method is expected to
  // be faster than CalculateBorders for the same innerType/outerType combination, since it only queries
  // whether a border exists, without requiring calculating all of them
  virtual bool HasBorders(EContentType innerType, const FullContentArray& outerTypes) const override;
  
  // retrieve the borders currently found in the map between the given types. This query is not const
  // so that the memory map can calculate and cache values upon being requested, rather than when
  // the map is modified. Function is expected to clear the vector before returning the new borders
  virtual void CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderRegionVector& outBorders) override;
  
  // checks if the given ray collides with the given type (any quad with that type)
  virtual bool HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, const FullContentArray& types) const override;
  
  // returns true if there are any nodes of the given type, false otherwise
  virtual bool HasContentType(EContentType type) const override;
  
  // Broadcast the memory map
  virtual void GetBroadcastInfo(MemoryMapTypes::MapBroadcastData& info) const override;

  // get the timestamp the QT was last measured (we can update the QT with a new timestamp even if the content
  // does not change)
  virtual TimeStamp_t GetLastChangedTimeStamp() const override {return _quadTree.GetRootNodeData()->GetLastObservedTime();}

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // underlaying data container
  QuadTree _quadTree;
  
}; // class
  
} // namespace
} // namespace

#endif // ANKI_COZMO_MEMORY_MAP_H
